#include "tasks/RoutingTask/LocoNetPrinter.h"

#include <cinttypes>
#include <cstdio>

#include "LocoNetSlotServer.h"

namespace tasks {
namespace RoutingTask {

void printLnPacket(const lnMsg& LnPacket, RxTxDirection rxTx) {
  printf("LN %sX: ", (rxTx == RxTxDirection::RX ? "R" : "T"));
  for (int i = 0; i < getLnMsgSize(const_cast<lnMsg*>(&LnPacket)); ++i) {
    printf(" %x", LnPacket.data[i]);
  }

  // OpCode
  switch (LnPacket.data[0]) {
    case OPC_BUSY:
      printf(" [Busy]");
      break;
    case OPC_GPOFF:
      printf(" [Power Off]");
      break;
    case OPC_GPON:
      printf(" [Power On]");
      break;
    case OPC_IDLE:
      printf(" [Idle]");
      break;
    case OPC_LOCO_SPD:
      printf(" [OPC_LOCO_SPD] Slot: %u Speed: %u", LnPacket.lsp.slot, LnPacket.lsp.spd);
      break;
    case OPC_LOCO_DIRF:
      printf(" [OPC_LOCO_DIRF] Slot: %u DirF: %#02x", LnPacket.ldf.slot, LnPacket.ldf.dirf);
      break;
    case OPC_LOCO_SND:
      printf(" [OPC_LOCO_SND] Slot: %u Sound: %#02x", LnPacket.ls.slot, LnPacket.ls.snd);
      break;
    case OPC_SW_REQ:
      printf(" [Switch Req] %u", LocoNetSlotServer::extractAddress(LnPacket));
      break;
    case OPC_SW_REP:
      printf(" [Switch Rep] %u", LocoNetSlotServer::extractAddress(LnPacket));
      break;
    case OPC_INPUT_REP:
      printf(" [Input Rep]");
      break;
    case OPC_UNKNOWN:
      printf(" [Unknown]");
      break;
    case OPC_LONG_ACK:
      printf(" [OPC_LONG_ACK]");
      break;
    case OPC_SLOT_STAT1:
      printf(" [OPC_SLOT_STAT1] Slot: %u Stat1: %#02x", LnPacket.ss.slot, LnPacket.ss.stat);
      break;
    case OPC_CONSIST_FUNC:
      printf(" [OPC_CONSIST_FUNC] Slot: %u Dirf: %#02x", LnPacket.cf.slot, LnPacket.cf.dirf);
      break;
    case OPC_UNLINK_SLOTS:
      printf(" [OPC_UNLINK_SLOTS] Master: %u Slave: %u", LnPacket.sm.dest, LnPacket.sm.src);
      break;
    case OPC_LINK_SLOTS:
      printf(" [OPC_LINK_SLOTS] Master: %u Slave: %u", LnPacket.sm.dest, LnPacket.sm.src);
      break;
    case OPC_MOVE_SLOTS:
      printf(" [OPC_MOVE_SLOTS] %u -> %u", LnPacket.sm.src, LnPacket.sm.dest);
      break;
    case OPC_RQ_SL_DATA:
      printf(" [OPC_RQ_SL_DATA] Slot %u", LnPacket.sr.slot);
      break;
    case OPC_SW_STATE:
      printf(" [OPC_SW_STATE] Addr %u", LocoNetSlotServer::extractAddress(LnPacket));
      break;
    case OPC_SW_ACK:
      printf(" [OPC_SW_ACK] Addr %u", LocoNetSlotServer::extractAddress(LnPacket));
      break;
    case OPC_LOCO_ADR:
      printf(" [OPC_LOCO_ADR] Addr %u", LnPacket.la.adr_lo);
      break;
    case OPC_MULTI_SENSE:
      printf(" [OPC_MULTI_SENSE]");
      break;
    case OPC_PEER_XFER:
      printf(" [OPC_PEER_XFER]");
      break;
    case OPC_SL_RD_DATA:
      printf(" [OPC_SL_RD_DATA] Slot: %u Stat: %u Addr: %u Speed: %u Dirf: %#02x ...",
             LnPacket.sd.slot, LnPacket.sd.stat, LnPacket.sd.adr, LnPacket.sd.spd,
             LnPacket.sd.dirf);
      break;
    case OPC_IMM_PACKET:
      printf(" [OPC_IMM_PACKET]");
      break;
    case OPC_IMM_PACKET_2:
      printf(" [OPC_IMM_PACKET_2]");
      break;
    case OPC_WR_SL_DATA:
      printf(" [OPC_WR_SL_DATA] Slot: %u Stat: %u Addr: %u Speed: %u Dirf: %#02x ...",
             LnPacket.sd.slot, LnPacket.sd.stat, LnPacket.sd.adr, LnPacket.sd.spd,
             LnPacket.sd.dirf);
      break;
  }

  printf("\n");
}

}  // namespace RoutingTask
}  // namespace tasks
