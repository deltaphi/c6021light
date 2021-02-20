#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include <algorithm>

#include "RR32Can/RR32Can.h"

#include "ConsoleManager.h"
#include "DataModel.h"

#include "RoutingForwarder.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"

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
  SlotDB_t::iterator slotIt = findSlot(lnMsgSlot);
  if (!slotIt->inUse) {
    requestSlotDataRead(slotIt);
  }
  return slotIt;
}

void LocoNetSlotServer::requestSlotDataRead(LocoNetSlotServer::SlotDB_t::iterator slot) const {
  if (!this->isDisabled()) {
    slot->diff = LocoDiff_t{};
    lnMsg msg;
    slotReqMsg& reqMsg{msg.sr};
    reqMsg.command = OPC_RQ_SL_DATA;
    reqMsg.slot = this->findSlotIndex(slot);
    reqMsg.pad = 0;
    tx_->AsyncSend(msg);
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
    slotIt->needsMatchToCAN = true;
  }
}

void LocoNetSlotServer::processLocoSpeed(const locoSpdMsg& msg) {
  const SlotDB_t::iterator slotIt = findOrRequestSlot(msg.slot);

  if (isSlotInBounds(slotIt)) {
    slotIt->loco.setVelocity(lnSpeedToCanVelocity(msg.spd));
    slotIt->diff.velocity = true;
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
      slotIt->diff.direction |= (delta & kDirfDirMask) != 0;
      slotIt->diff.functions |= (delta & 0x10) >> 4;
      slotIt->diff.functions |= (delta & 0x0F) << 1;
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
      slotIt->diff.functions |= (delta & 0x0F << 5);
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

void LocoNetSlotServer::sendSlotDataRead(const SlotDB_t::const_iterator slot) const {
  lnMsg txMsg;
  rwSlotDataMsg& slotRead = txMsg.sd;

  // See https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en for message definition.

  SlotIdx_t slotIdx = this->findSlotIndex(slot);

  slotRead.command = OPC_SL_RD_DATA;
  slotRead.mesg_size = 0x0Eu;
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
  slotRead.chksum = 0;

  tx_->AsyncSend(txMsg);
}

void LocoNetSlotServer::sendNoDispatch() const {
  lnMsg msg = Ln_LongAck(0);
  tx_->AsyncSend(msg);
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
