#ifndef __TASKS__ROUTINGTASK__LOCONETHELPERS_H__
#define __TASKS__ROUTINGTASK__LOCONETHELPERS_H__

#include "RR32Can/Constants.h"
#include "RR32Can/Locomotive.h"
#include "RR32Can/Types.h"
#include "ln_opc.h"

namespace tasks {
namespace RoutingTask {

constexpr static const RR32Can::Velocity_t kLocoNetMaxVeloctiy = 127;
constexpr static const uint8_t kDirfDirMask = 0b00100000;  // mask set == reverse
constexpr static const uint8_t kFunctionsInDirfMessage = 5;

constexpr static const uint8_t kFunctionsInSndMessage = 4;
constexpr static const uint8_t kLowestFunctionInSndMessage = 5;

constexpr RR32Can::Velocity_t lnSpeedToCanVelocity(RR32Can::Velocity_t speed) {
  return (speed * RR32Can::kMaxEngineVelocity / kLocoNetMaxVeloctiy);
}

constexpr RR32Can::Velocity_t canVelocityToLnSpeed(RR32Can::Velocity_t velocity) {
  return (velocity * kLocoNetMaxVeloctiy) / RR32Can::kMaxEngineVelocity;
}

void dirfToLoco(const uint8_t dirf, RR32Can::LocomotiveData& loco);
uint8_t locoToDirf(const RR32Can::LocomotiveData& loco);

void sndToLoco(const uint8_t snd, RR32Can::LocomotiveData& loco);
uint8_t locoToSnd(const RR32Can::LocomotiveData& loco);

constexpr RR32Can::Locomotive::Address_t getLocoAddress(const rwSlotDataMsg& slotRead) {
  RR32Can::Locomotive::Address_t::value_type address = slotRead.adr2 << 7;
  address |= slotRead.adr & 0x7F;
  return RR32Can::Locomotive::Address_t{address};
}

constexpr void putLocoAddress(rwSlotDataMsg& slotRead,
                              const RR32Can::Locomotive::Address_t address) {
  slotRead.adr = address.value() & 0x7F;  // Loco Address low bits
  slotRead.adr2 = address.value() >> 7;   // Loco Address high bits
}

constexpr bool isDispatchGet(const slotMoveMsg& msg) { return msg.src == 0; }
constexpr bool isDispatchPut(const slotMoveMsg& msg) { return msg.dest == 0; }
constexpr bool isNullMove(const slotMoveMsg& msg) { return msg.src == msg.dest; }

constexpr lnMsg Ln_Turnout(RR32Can::MachineTurnoutAddress address,
                           RR32Can::TurnoutDirection direction, bool power) {
  const RR32Can::MachineTurnoutAddress addr = address.getNumericAddress();
  lnMsg LnPacket{};
  LnPacket.srq.command = OPC_SW_REQ;

  LnPacket.srq.sw1 = addr.value() & 0x7F;
  LnPacket.srq.sw2 = (addr.value() >> 7) & 0x0F;

  if (power) {
    LnPacket.srq.sw2 |= OPC_SW_REQ_OUT;
  }
  if (direction == RR32Can::TurnoutDirection::GREEN) {
    LnPacket.srq.sw2 |= OPC_SW_REQ_DIR;
  }

  return LnPacket;
}

inline lnMsg Ln_Sensor(RR32Can::MachineTurnoutAddress address, RR32Can::SensorState state) {
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

  return LnPacket;
}

inline lnMsg Ln_On() {
  lnMsg LnPacket{};
  LnPacket.ir.command = OPC_GPON;

  return LnPacket;
}

inline lnMsg Ln_Off() {
  lnMsg LnPacket{};
  LnPacket.ir.command = OPC_GPOFF;

  return LnPacket;
}

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETHELPERS_H__
