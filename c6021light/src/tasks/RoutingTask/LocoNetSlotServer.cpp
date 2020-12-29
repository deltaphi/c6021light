#include "tasks/RoutingTask/LocoNetSlotServer.h"

#include <algorithm>

namespace tasks {
namespace RoutingTask {

void LocoNetSlotServer::processSlotMove(const slotMoveMsg& msg) {
  if (isDispatchGet(msg)) {
    if (dispatchSlotAvailable()) {
      sendSlotDataRead(slotInDispatch_);
    } else {
      sendNoDispatch();
    }
  } else if (isDispatchPut(msg)) {
    slotInDispatch_ = slotDB_.begin();
    std::advance(slotInDispatch_, msg.src);
  }
  // No processing for other slot moves needed.
}

void LocoNetSlotServer::processLocoRequest(LocoAddr_t locoAddr) {
  SlotDB_t::iterator slot = findOrAllocateSlotForAddress(locoAddr);

  if (slot != slotDB_.end()) {
    sendSlotDataRead(slot);
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

  slotRead.adr = slot->locoAddress & 0x7F;  // Loco Address low bits
  slotRead.spd = 0;                         // Speed
  slotRead.dirf = 0;                        // Direction & Functions 0-4
  slotRead.trk = 0;                         //
  slotRead.ss2 = 0;                         // Status2
  slotRead.adr2 = slot->locoAddress >> 7;   // Loco Address high bits
  slotRead.snd = 0;                         // Sound
  slotRead.id1 = 0;                         // Throttle ID (low)
  slotRead.id2 = 0;                         // Throttle ID (high)

  LocoNet.send(&txMsg);
}

void LocoNetSlotServer::sendNoDispatch() const { LocoNet.sendLongAck(0); }

}  // namespace RoutingTask
}  // namespace tasks
