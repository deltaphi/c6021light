#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include <algorithm>

#include "RR32Can/RR32Can.h"

#include "ConsoleManager.h"
#include "DataModel.h"

#include "RoutingForwarder.h"

namespace tasks {
namespace RoutingTask {

const LocoNetSlotServer::SlotIdx_t LocoNetSlotServer::kNumSlots;

bool LocoNetSlotServer::isDisabled() const {
  return dataModel_->lnSlotServerState == SlotServerState::DISABLED;
}

bool LocoNetSlotServer::isPassive() const {
  return dataModel_->lnSlotServerState == SlotServerState::PASSIVE;
}

bool LocoNetSlotServer::isActive() const {
  return dataModel_->lnSlotServerState == SlotServerState::ACTIVE;
}

void LocoNetSlotServer::processSlotMove(const slotMoveMsg& msg) {
  if (isDispatchGet(msg)) {
    if (isActive()) {
      if (dispatchSlotAvailable()) {
        sendSlotDataRead(slotInDispatch_);
      } else {
        sendNoDispatch();
      }
    }
  } else if (isDispatchPut(msg)) {
    slotInDispatch_ = slotDB_.begin();
    std::advance(slotInDispatch_, msg.src);
  } else if (isNullMove(msg)) {
    // Self-Moves currently ignored.
  } else {
    auto fromSlot = std::next(slotDB_.begin(), msg.src);
    auto toSlot = std::next(slotDB_.begin(), msg.dest);

    if (isSlotInBounds(toSlot)) {
      if (isSlotInBounds(fromSlot)) {
        *toSlot = *fromSlot;
        clearSlot(fromSlot);
      } else {
        clearSlot(toSlot);
      }
    }
  }
}

void LocoNetSlotServer::processLocoRequest(const LocoAddr_t locoAddr) {
  if (isActive()) {
    SlotDB_t::iterator slot = findOrAllocateSlotForAddress(locoAddr);
    if (slot != slotDB_.end()) {
      sendSlotDataRead(slot);
    }
  }
}

LocoNetSlotServer::SlotDB_t::iterator LocoNetSlotServer::findSlot(const uint8_t lnMsgSlot) {
  SlotDB_t::iterator slotIt = slotDB_.begin();
  std::advance(slotIt, std::min(lnMsgSlot, kNumSlots));
  return slotIt;
}

LocoNetSlotServer::SlotDB_t::iterator LocoNetSlotServer::findOrRequestSlot(
    const uint8_t lnMsgSlot) {
  const SlotDB_t::iterator slotIt = findSlot(lnMsgSlot);
  if (!slotIt->inUse) {
    requestSlotDataRead(slotIt);
  }
  return slotIt;
}

void LocoNetSlotServer::requestSlotDataRead(
    const LocoNetSlotServer::SlotDB_t::const_iterator slot) const {
  if (!isDisabled()) {
    lnMsg msg;
    slotReqMsg& reqMsg{msg.sr};
    reqMsg.command = OPC_RQ_SL_DATA;
    reqMsg.slot = findSlotIndex(slot);
    reqMsg.pad = 0;
    LocoNet.send(&msg);
  }
}

void LocoNetSlotServer::processSlotRead(const rwSlotDataMsg& msg) {
  // engine information learned about a slot
  SlotDB_t::iterator slotIt = findSlot(msg.slot);

  if (slotIt != slotDB_.end()) {
    slotIt->inUse = true;
    slotIt->loco.reset();
    slotIt->loco.setAddress(getLocoAddress(msg));
    slotIt->loco.setUid(getLocoAddress(msg).value());  // Foce the engine to be MM2.
    slotIt->loco.setVelocity(lnSpeedToCanVelocity(msg.spd));
    dirfToLoco(msg.dirf, slotIt->loco);
    sndToLoco(msg.snd, slotIt->loco);
  }
}

void LocoNetSlotServer::processLocoSpeed(const locoSpdMsg& msg) {
  const SlotDB_t::iterator slotIt = findOrRequestSlot(msg.slot);

  if (isSlotInBounds(slotIt)) {
    slotIt->loco.setVelocity(lnSpeedToCanVelocity(msg.spd));

    if (shouldSendEngineUpdateForSlot(slotIt)) {
      forwarder_.forwardLocoChange(slotIt->loco, true, false, 0);
    }
  }
}

void LocoNetSlotServer::processLocoDirF(const locoDirfMsg& msg) {
  const SlotDB_t::iterator slotIt = findOrRequestSlot(msg.slot);

  if (isSlotInBounds(slotIt)) {
    const auto oldDirf = locoToDirf(slotIt->loco);
    const auto delta = oldDirf ^ msg.dirf;
    const bool hasChanged = delta != 0;

    if (hasChanged) {
      dirfToLoco(msg.dirf, slotIt->loco);

      if (shouldSendEngineUpdateForSlot(slotIt)) {
        const bool directionChanged = (delta & kDirfDirMask) != 0;
        const uint16_t f1to4Bits = (hasChanged & 0x0F) << 1;
        const uint16_t f0bit = (hasChanged >> 4) & 0x01;
        const uint16_t functionChangeMask = f0bit | f1to4Bits;

        forwarder_.forwardLocoChange(slotIt->loco, false, directionChanged, functionChangeMask);
      }
    }
  }
}

void LocoNetSlotServer::processLocoSnd(const locoSndMsg& msg) {
  const SlotDB_t::iterator slotIt = findOrRequestSlot(msg.slot);

  if (isSlotInBounds(slotIt)) {
    const uint8_t oldDirf = locoToSnd(slotIt->loco);
    const auto delta = oldDirf ^ msg.snd;
    const bool hasChanged = delta != 0;

    if (hasChanged) {
      sndToLoco(msg.snd, slotIt->loco);

      if (shouldSendEngineUpdateForSlot(slotIt)) {
        uint8_t functionMask = 1;
        for (uint8_t i = kFunctionsInDirfMessage;
             i < kFunctionsInDirfMessage + kFunctionsInSndMessage; ++i) {
          uint8_t functionIdx = i + 1;
          if (functionIdx == kFunctionsInDirfMessage) {
            functionIdx = 0;
          }
          if ((delta & functionMask) != 0) {
            RR32Can::RR32Can.SendEngineFunction(slotIt->loco, functionIdx,
                                                slotIt->loco.getFunction(functionIdx));
          }
          functionMask <<= 1;
        }
      }
    }
  }
}

void LocoNetSlotServer::process(const lnMsg& LnPacket) {
  switch (LnPacket.data[0]) {
    case OPC_MOVE_SLOTS:
      processSlotMove(LnPacket.sm);
      break;
    case OPC_LOCO_ADR:
      processLocoRequest(extractLocoAddress(LnPacket));
      break;
    case OPC_SL_RD_DATA:
    case OPC_WR_SL_DATA:
      processSlotRead(LnPacket.sd);
      break;
    case OPC_LOCO_SPD:
      processLocoSpeed(LnPacket.lsp);
      break;
    case OPC_LOCO_DIRF:
      processLocoDirF(LnPacket.ldf);
      break;
    case OPC_LOCO_SND:
      processLocoSnd(LnPacket.ls);
      break;
    default:
      break;
  }
}

void LocoNetSlotServer::sendSlotDataRead(SlotDB_t::const_iterator slot) const {
  lnMsg txMsg;
  rwSlotDataMsg& slotRead = txMsg.sd;

  // See https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en for message definition.

  SlotIdx_t slotIdx = std::distance(slotDB_.begin(), slot);

  slotRead.command = OPC_SL_RD_DATA;
  slotRead.slot = slotIdx;  // Slot Number
  slotRead.stat = 0;        // Status1, speed steps

  if (slot->inUse) {
    slotRead.stat |= 0b00110000;  // Busy & Active
    slotRead.stat |= 0b00000011;  // 128 steps mode, no advanced consisting
  } else {
    slotRead.stat &= ~0b00110000;  // FREE Slot
  }

  putLocoAddress(slotRead, slot->loco.getAddress().getNumericAddress());

  slotRead.spd = canVelocityToLnSpeed(slot->loco.getVelocity());  // Speed
  slotRead.dirf = locoToDirf(slot->loco);                         // Direction & Functions 0-4
  slotRead.trk = 0;                                               //
  slotRead.ss2 = 0;                                               // Status2
  slotRead.snd = locoToSnd(slot->loco);                           // F5-8
  slotRead.id1 = 0;                                               // Throttle ID (low)
  slotRead.id2 = 0;                                               // Throttle ID (high)

  LocoNet.send(&txMsg);
}

void LocoNetSlotServer::sendNoDispatch() const { LocoNet.sendLongAck(0); }

void LocoNetSlotServer::dirfToLoco(const uint8_t dirf, LocoData_t& loco) {
  RR32Can::EngineDirection direction;
  if ((dirf & kDirfDirMask) == kDirfDirMask) {
    direction = RR32Can::EngineDirection::REVERSE;
  } else {
    direction = RR32Can::EngineDirection::FORWARD;
  }
  loco.setDirection(direction);

  uint8_t functionMask = 1;
  for (uint8_t i = 0; i < kFunctionsInDirfMessage; ++i) {
    uint8_t functionIdx = i + 1;
    if (functionIdx == kFunctionsInDirfMessage) {
      functionIdx = 0;
    }
    loco.setFunction(functionIdx, ((dirf & functionMask) != 0));
    functionMask <<= 1;
  }
}

uint8_t LocoNetSlotServer::locoToDirf(const LocoData_t& loco) {
  uint8_t dirf = 0;

  if (loco.getDirection() == RR32Can::EngineDirection::FORWARD) {
    dirf &= ~kDirfDirMask;
  } else {
    dirf |= kDirfDirMask;
  }

  uint8_t functionMask = 1;
  for (uint8_t i = 0; i < kFunctionsInDirfMessage; ++i) {
    uint8_t functionIdx = i + 1;
    if (functionIdx == kFunctionsInDirfMessage) {
      functionIdx = 0;
    }
    if (loco.getFunction(functionIdx)) {
      dirf |= functionMask;
    } else {
      dirf &= ~functionMask;
    }
    functionMask <<= 1;
  }

  return dirf;
}

void LocoNetSlotServer::sndToLoco(const uint8_t snd, LocoData_t& loco) {
  uint8_t functionMask = 1;
  for (uint8_t functionIdx = kFunctionsInSndMessage;
       functionIdx < kLowestFunctionInSndMessage + kFunctionsInSndMessage; ++functionIdx) {
    loco.setFunction(functionIdx, ((snd & functionMask) != 0));
    functionMask <<= 1;
  }
}

uint8_t LocoNetSlotServer::locoToSnd(const LocoData_t& loco) {
  uint8_t snd = 0;

  uint8_t functionMask = 1;
  for (uint8_t i = kFunctionsInSndMessage; i < kLowestFunctionInSndMessage + kFunctionsInSndMessage;
       ++i) {
    uint8_t functionIdx = i;
    if (loco.getFunction(functionIdx)) {
      snd |= functionMask;
    } else {
      snd &= ~functionMask;
    }
    functionMask <<= 1;
  }
  return snd;
}

void LocoNetSlotServer::dump() const {
  puts("LocoNet Slot Server Status:");
  SlotIdx_t slotIdx = 0;
  for (auto& slot : slotDB_) {
    if (slot.inUse) {
      printf("Slot %i: InUse: %s Loco:", slotIdx, (slot.inUse ? "true" : "false"));
      slot.loco.print();
    }
    ++slotIdx;
  }
  puts("-- LocoNet Slot Server Status.");
}

}  // namespace RoutingTask
}  // namespace tasks
