#include "tasks/RoutingTask/LocoNetHelpers.h"

namespace tasks {
namespace RoutingTask {

void dirfToLoco(const uint8_t dirf, RR32Can::LocomotiveData& loco) {
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

uint8_t locoToDirf(const RR32Can::LocomotiveData& loco) {
  uint8_t dirf = 0;

  if (loco.getDirection() == RR32Can::EngineDirection::REVERSE) {
    dirf |= kDirfDirMask;
  } else {
    dirf &= ~kDirfDirMask;
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

void sndToLoco(const uint8_t snd, RR32Can::LocomotiveData& loco, const uint8_t functionOffset) {
  uint8_t functionMask = 1;
  for (uint8_t functionIdx = functionOffset; functionIdx < functionOffset + kFunctionsInSndMessage;
       ++functionIdx) {
    loco.setFunction(functionIdx, ((snd & functionMask) != 0));
    functionMask <<= 1;
  }
}

uint8_t locoToSnd(const RR32Can::LocomotiveData& loco, const uint8_t functionOffset) {
  uint8_t snd = 0;

  uint8_t functionMask = 1;
  for (uint8_t functionIdx = functionOffset; functionIdx < functionOffset + kFunctionsInSndMessage;
       ++functionIdx) {
    if (loco.getFunction(functionIdx)) {
      snd |= functionMask;
    } else {
      snd &= ~functionMask;
    }
    functionMask <<= 1;
  }
  return snd;
}

namespace {

void putLocoAddress(rwSlotDataMsg& slotRead, const RR32Can::Locomotive::Address_t address) {
  slotRead.adr = address.value() & 0x7F;  // Loco Address low bits
  slotRead.adr2 = address.value() >> 7;   // Loco Address high bits
}

void putTurnoutAddress(lnMsg& LnPacket, RR32Can::MachineTurnoutAddress address,
                       RR32Can::TurnoutDirection direction, bool power) {
  const RR32Can::MachineTurnoutAddress addr = address.getNumericAddress();

  LnPacket.srq.sw1 = addr.value() & 0x7F;
  LnPacket.srq.sw2 = (addr.value() >> 7) & 0x0F;

  if (power) {
    LnPacket.srq.sw2 |= OPC_SW_REQ_OUT;
  }
  if (direction == RR32Can::TurnoutDirection::GREEN) {
    LnPacket.srq.sw2 |= OPC_SW_REQ_DIR;
  }
}

}  // namespace

lnMsg Ln_Turnout(RR32Can::MachineTurnoutAddress address, RR32Can::TurnoutDirection direction,
                 bool power) {
  lnMsg LnPacket{};
  putTurnoutAddress(LnPacket, address, direction, power);
  LnPacket.srq.command = OPC_SW_REQ;
  LnPacket.srq.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_TurnoutStatusRequest(RR32Can::MachineTurnoutAddress address) {
  lnMsg LnPacket{};
  putTurnoutAddress(LnPacket, address, RR32Can::TurnoutDirection::RED, false);
  LnPacket.srq.command = OPC_SW_STATE;
  LnPacket.srq.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_TurnoutStatusResponse(RR32Can::MachineTurnoutAddress address,
                               RR32Can::TurnoutDirection direction, bool power) {
  lnMsg LnPacket{};
  putTurnoutAddress(LnPacket, address, direction, power);
  LnPacket.srq.command = OPC_SW_REP;
  LnPacket.srq.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_Sensor(RR32Can::MachineTurnoutAddress address, RR32Can::SensorState state) {
  lnMsg LnPacket{};
  LnPacket.ir.command = OPC_INPUT_REP;

  const uint8_t bit0 = address.value() & 1;
  const uint8_t bits1to7 = (address.value() >> 1) & 0x7F;
  const uint8_t bits8to11 = (address.value() >> 8) & 0x0F;
  LnPacket.ir.in1 = bits1to7;
  LnPacket.ir.in2 = (bit0 << 5) | bits8to11;

  if (state == RR32Can::SensorState::CLOSED) {
    LnPacket.ir.in2 |= OPC_INPUT_REP_HI;
  }
  LnPacket.ir.chksum = 0U;

  return LnPacket;
}

lnMsg Ln_LocoAddr(const RR32Can::MachineLocomotiveAddress& address) {
  lnMsg LnPacket{};
  const uint16_t addr = address.getNumericAddress().value();
  LnPacket.la.command = OPC_LOCO_ADR;
  LnPacket.la.adr_hi = addr >> 7;
  LnPacket.la.adr_lo = addr & 0x7F;
  LnPacket.la.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_SlotMove(uint8_t src, uint8_t dest) {
  lnMsg LnPacket{};
  LnPacket.sm.command = OPC_MOVE_SLOTS;
  LnPacket.sm.src = src;
  LnPacket.sm.dest = dest;
  LnPacket.sm.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_RequestSlotData(uint8_t slot) {
  lnMsg LnPacket;
  LnPacket.sr.command = OPC_RQ_SL_DATA;
  LnPacket.sr.slot = slot;
  LnPacket.sr.pad = 0U;
  LnPacket.sr.chksum = 0U;
  return LnPacket;
}

namespace {

rwSlotDataMsg Ln_SlotReadWriteMessage(uint8_t slot, uint8_t stat,
                                      const RR32Can::LocomotiveData& engine) {
  rwSlotDataMsg msg;
  msg.mesg_size = 0x0Eu;
  msg.slot = slot;  // Slot Number
  msg.stat = stat;  // Status1, speed steps
  putLocoAddress(msg, engine.getAddress());
  msg.spd = static_cast<uint8_t>(canVelocityToLnSpeed(engine.getVelocity()));  // Speed
  msg.dirf = locoToDirf(engine);                             // Direction & Functions 0-4
  msg.trk = 0;                                               //
  msg.ss2 = 0;                                               // Status2
  msg.snd = locoToSnd(engine, kLowestFunctionInSndMessage);  // F5-8
  msg.id1 = 0;                                               // Throttle ID (low)
  msg.id2 = 0;                                               // Throttle ID (high)
  msg.chksum = 0;
  return msg;
}

}  // namespace

lnMsg Ln_SlotDataRead(uint8_t slot, uint8_t stat, const RR32Can::LocomotiveData& engine) {
  // See https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en for message definition.
  lnMsg LnPacket{};
  LnPacket.sd = Ln_SlotReadWriteMessage(slot, stat, engine);
  LnPacket.sd.command = OPC_SL_RD_DATA;

  return LnPacket;
}

lnMsg Ln_SlotDataWrite(uint8_t slot, uint8_t stat, const RR32Can::LocomotiveData& engine) {
  // See https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en for message definition.
  lnMsg LnPacket{};
  LnPacket.sd = Ln_SlotReadWriteMessage(slot, stat, engine);
  LnPacket.sd.command = OPC_WR_SL_DATA;
  return LnPacket;
}

lnMsg Ln_LocoSpeed(uint8_t slotIdx, RR32Can::Velocity_t velocity) {
  lnMsg msg;
  locoSpdMsg& speedMessage = msg.lsp;
  speedMessage.command = OPC_LOCO_SPD;
  speedMessage.slot = slotIdx;
  speedMessage.spd = canVelocityToLnSpeed(velocity);
  speedMessage.chksum = 0U;
  return msg;
}

lnMsg Ln_LocoDirf(uint8_t slotIdx, const RR32Can::LocomotiveData& loco) {
  lnMsg msg;
  locoDirfMsg& dirfMessage = msg.ldf;
  dirfMessage.command = OPC_LOCO_DIRF;
  dirfMessage.slot = slotIdx;
  dirfMessage.dirf = locoToDirf(loco);
  dirfMessage.chksum = 0U;
  return msg;
}

lnMsg Ln_LocoSnd(uint8_t slotIdx, const RR32Can::LocomotiveData& loco) {
  lnMsg msg;
  locoSndMsg& sndMessage = msg.ls;
  sndMessage.command = OPC_LOCO_SND;
  sndMessage.slot = slotIdx;
  sndMessage.snd = locoToSnd(loco, kLowestFunctionInSndMessage);
  sndMessage.chksum = 0U;
  return msg;
}

lnMsg Ln_LocoSnd2(uint8_t slotIdx, const RR32Can::LocomotiveData& loco) {
  lnMsg msg;
  locoSndMsg& sndMessage = msg.ls;
  sndMessage.command = OPC_LOCO_SND + 1U;
  sndMessage.slot = slotIdx;
  sndMessage.snd = locoToSnd(loco, kLowestFunctionInSndMessage + kFunctionsInSndMessage);
  sndMessage.chksum = 0U;
  return msg;
}

namespace {

uint8_t locoToFunExtByte(const LocoFunExtBlockId blockId, const RR32Can::LocomotiveData& loco) {
  uint8_t result{0};

  if (blockId == LocoFunExtBlockId::THIRD) {
    if (loco.getFunction(20)) {
      result |= kFunExt20Mask;
    }
    if (loco.getFunction(28)) {
      result |= kFunExt28Mask;
    }
  } else {
    const auto startIdx =
        (blockId == LocoFunExtBlockId::FIRST ? kFunExtFirstOffset : kFunExtSecondOffset);
    uint8_t mask{1U};
    for (uint8_t i = startIdx; i < startIdx + kFunExtFunctionPerMessage; ++i) {
      if (loco.getFunction(i)) {
        result |= mask;
      }
      mask <<= 1;
    }
  }

  return result;
}

}  // namespace

lnMsg Ln_LocoFunExt(const uint8_t slotIdx, const LocoFunExtBlockId blockId,
                    const RR32Can::LocomotiveData& loco) {
  lnMsg msg;
  msg.msdi.command = OPC_LOCO_FUNEXT;
  msg.msdi.arg1 = kFunExtMagicByte;
  msg.msdi.arg2 = slotIdx;
  msg.msdi.arg3 = static_cast<uint8_t>(blockId);
  msg.msdi.arg4 = locoToFunExtByte(blockId, loco);
  msg.msdi.chksum = 0;
  return msg;
}

}  // namespace RoutingTask
}  // namespace tasks
