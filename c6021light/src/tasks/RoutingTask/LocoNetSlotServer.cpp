#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include "ConsoleManager.h"
#include "DataModel.h"

#include <algorithm>

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

void LocoNetSlotServer::processLocoRequest(LocoAddr_t locoAddr) {
  SlotDB_t::iterator slot = findOrAllocateSlotForAddress(locoAddr);

  if (slot != slotDB_.end()) {
    sendSlotDataRead(slot);
  }
}

void LocoNetSlotServer::processSlotRead(const rwSlotDataMsg& msg) {
  // engine information learned about a slot
  SlotDB_t::iterator slotIt = slotDB_.begin();
  std::advance(slotIt, std::min(msg.slot, kNumSlots));

  if (slotIt != slotDB_.end()) {
    slotIt->locoAddress = getLocoAddress(msg);
    slotIt->inUse = true;
  }
}

void LocoNetSlotServer::process(const lnMsg& LnPacket) {
  switch (LnPacket.data[0]) {
    case OPC_MOVE_SLOTS:
      processSlotMove(LnPacket.sm);
      break;
    case OPC_LOCO_ADR:
      processLocoRequest(extractAddress(LnPacket));
      break;
    case OPC_SL_RD_DATA:
    case OPC_WR_SL_DATA:
      processSlotRead(LnPacket.sd);
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

  putLocoAddress(slotRead, slot->locoAddress);

  slotRead.spd = 0;   // Speed
  slotRead.dirf = 0;  // Direction & Functions 0-4
  slotRead.trk = 0;   //
  slotRead.ss2 = 0;   // Status2
  slotRead.snd = 0;   // Sound
  slotRead.id1 = 0;   // Throttle ID (low)
  slotRead.id2 = 0;   // Throttle ID (high)

  LocoNet.send(&txMsg);
}

void LocoNetSlotServer::sendNoDispatch() const { LocoNet.sendLongAck(0); }

void LocoNetSlotServer::dump() const {
  puts("LocoNet Slot Server Status");
  SlotIdx_t slotIdx = 0;
  for (auto& slot : slotDB_) {
    printf("Slot %i:\n  InUse: %s\n  LocoAddr: %lu\n", slotIdx, (slot.inUse ? "true" : "false"),
           slot.locoAddress);
    ++slotIdx;
  }
}

}  // namespace RoutingTask
}  // namespace tasks
