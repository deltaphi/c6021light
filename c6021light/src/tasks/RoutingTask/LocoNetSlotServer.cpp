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
        slotInDispatch_ = end();
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
    const SlotDB_t::const_iterator slot = findOrAllocateSlotForAddress(locoAddr);
    if (slot != slotDB_.end()) {
      sendSlotDataRead(slot);
    }
  }
}

LocoNetSlotServer::SlotDB_t::iterator LocoNetSlotServer::findOrRequestSlot(
    const uint8_t lnMsgSlot) {
  SlotDB_t::iterator slotIt = findSlot(lnMsgSlot);
  if (!slotIt->inUse) {
    requestSlotDataRead(slotIt);
  }
  return slotIt;
}

void LocoNetSlotServer::requestSlotDataRead(LocoNetSlotServer::SlotDB_t::iterator slot) {
  if (!this->isDisabled()) {
    slot->diff = LocoDiff_t{};
    const auto slotIdx = this->findSlotIndex(slot);
    const auto msg = Ln_RequestSlotData(slotIdx);
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
    slotIt->loco.setUid(getLocoAddress(msg).value());  // Force the engine to be MM2.
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
    const auto originalFunctionBits = slotIt->loco.getFunctionBits();

    if (hasChanged) {
      dirfToLoco(msg.dirf, slotIt->loco);
      slotIt->diff.direction |= (delta & kDirfDirMask) != 0;
      const auto updatedFunctionBits = slotIt->loco.getFunctionBits();
      const auto functionBitsDelta = updatedFunctionBits ^ originalFunctionBits;
      slotIt->diff.functions |= functionBitsDelta;
    }
  }
}

void LocoNetSlotServer::processLocoSnd(const locoSndMsg& msg) {
  const SlotDB_t::iterator slotIt = findOrRequestSlot(msg.slot);

  if (isSlotInBounds(slotIt)) {
    const auto oldDirf = locoToSnd(slotIt->loco);
    const auto delta = oldDirf ^ msg.snd;
    const bool hasChanged = delta != 0;

    const auto originalFunctionBits = slotIt->loco.getFunctionBits();

    if (hasChanged) {
      sndToLoco(msg.snd, slotIt->loco);
      const auto updatedFunctionBits = slotIt->loco.getFunctionBits();
      const auto functionBitsDelta = updatedFunctionBits ^ originalFunctionBits;
      slotIt->diff.functions |= functionBitsDelta;
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
    case OPC_RQ_SL_DATA:
      processRequestSlotRead(LnPacket.sr);
      break;
    case OPC_SL_RD_DATA:
      processSlotRead(LnPacket.sd);
      break;
    case OPC_WR_SL_DATA:
      processSlotRead(LnPacket.sd);
      if (isActive()) {
        tx_->AsyncSend(Ln_LongAck(OPC_WR_SL_DATA, true));
      }
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
  SlotIdx_t slotIdx = this->findSlotIndex(slot);
  uint8_t stat = 0b00000011;  // 128 steps mode, no advanced consisting

  if (slot->inUse) {
    stat |= 0b00110000;  // Busy & Active
  } else {
    stat &= ~0b00110000;  // FREE Slot
  }

  tx_->AsyncSend(Ln_SlotDataRead(slotIdx, stat, slot->loco));
}

void LocoNetSlotServer::sendNoDispatch() const {
  tx_->AsyncSend(Ln_LongAck(OPC_MOVE_SLOTS, false));
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

void LocoNetSlotServer::processRequestSlotRead(const slotReqMsg slotReq) const {
  const SlotDB_t::const_iterator it = findSlot(slotReq.slot);
  sendSlotDataRead(it);
}

}  // namespace RoutingTask
}  // namespace tasks
