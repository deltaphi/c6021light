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

constexpr static const uint8_t kFunExtMagicByte{0x20U};
constexpr static const uint8_t kFunExtFunctionPerMessage{7};
constexpr static const uint8_t kFunExtFirstOffset{13};
constexpr static const uint8_t kFunExtSecondOffset{21};

constexpr static const uint8_t kFunExt20Mask{0x20};
constexpr static const uint8_t kFunExt28Mask{0x40};

enum class LocoFunExtBlockId : uint8_t { FIRST = 8, SECOND = 9, THIRD = 5 };

constexpr static const decltype(OPC_LOCO_SND) OPC_LOCO_SND2{OPC_LOCO_SND + 1U};
constexpr static const decltype(OPC_LOCO_SND) OPC_LOCO_FUNEXT{0xD4U};

constexpr RR32Can::Velocity_t lnSpeedToCanVelocity(const uint8_t speed) {
  const uint16_t adjustedSpeed = speed * 10;
  const auto velocity = (adjustedSpeed * RR32Can::kMaxEngineVelocity / kLocoNetMaxVeloctiy);
  const auto roundedVelocity = (velocity + 5) / 10;
  return roundedVelocity;
}

constexpr uint8_t canVelocityToLnSpeed(const RR32Can::Velocity_t velocity) {
  const RR32Can::Velocity_t adjustedVelocity = velocity * 10;
  const auto speed = (adjustedVelocity * kLocoNetMaxVeloctiy) / RR32Can::kMaxEngineVelocity;
  const auto roundedSpeed = (speed + 5) / 10;
  return roundedSpeed;
}

void dirfToLoco(const uint8_t dirf, RR32Can::LocomotiveData& loco);
uint8_t locoToDirf(const RR32Can::LocomotiveData& loco);

void sndToLoco(const uint8_t snd, RR32Can::LocomotiveData& loco, const uint8_t i);
uint8_t locoToSnd(const RR32Can::LocomotiveData& loco, const uint8_t functionOffset);

constexpr RR32Can::Locomotive::Address_t getLocoAddress(const rwSlotDataMsg& slotRead) {
  RR32Can::Locomotive::Address_t::value_type address = slotRead.adr2 << 7;
  address |= slotRead.adr & 0x7F;
  return RR32Can::Locomotive::Address_t{address};
}

constexpr bool isDispatchGet(const slotMoveMsg& msg) { return msg.src == 0; }
constexpr bool isDispatchPut(const slotMoveMsg& msg) { return msg.dest == 0; }
constexpr bool isNullMove(const slotMoveMsg& msg) { return msg.src == msg.dest; }

constexpr RR32Can::MachineTurnoutAddress getTurnoutAddress(const lnMsg& LnPacket) {
  const uint16_t numericAddr = ((LnPacket.srq.sw2 & 0x0F) << 7) | LnPacket.srq.sw1;
  const RR32Can::MachineTurnoutAddress addr{numericAddr};
  return addr;
}

lnMsg Ln_Turnout(RR32Can::MachineTurnoutAddress address, RR32Can::TurnoutDirection direction,
                 bool power);

lnMsg Ln_TurnoutStatusRequest(RR32Can::MachineTurnoutAddress address);
lnMsg Ln_TurnoutStatusResponse(RR32Can::MachineTurnoutAddress address,
                               RR32Can::TurnoutDirection direction, bool power);
lnMsg Ln_Sensor(RR32Can::MachineTurnoutAddress address, RR32Can::SensorState state);

inline lnMsg Ln_On() {
  lnMsg LnPacket{};
  LnPacket.ir.command = OPC_GPON;
  LnPacket.ir.in1 = 0U;
  LnPacket.ir.in2 = 0U;
  LnPacket.ir.chksum = 0U;

  return LnPacket;
}

inline lnMsg Ln_Off() {
  lnMsg LnPacket{};
  LnPacket.ir.command = OPC_GPOFF;
  LnPacket.ir.in1 = 0U;
  LnPacket.ir.in2 = 0U;
  LnPacket.ir.chksum = 0U;

  return LnPacket;
}

/**
 * Construct a OPC_LONG_ACK message.
 *
 * @param lopc The Opcode that is ACKed.
 * @param success Whether the Acknowledgemet is positive (true) or negative (false).
 * @return
 */
inline lnMsg Ln_LongAck(uint8_t lopc, bool success) {
  lnMsg LnPacket{};
  LnPacket.lack.command = OPC_LONG_ACK;
  LnPacket.lack.opcode = lopc & 0x7fU;
  LnPacket.lack.ack1 = (success ? 0x7fU : 0U);
  LnPacket.lack.chksum = 0U;
  return LnPacket;
}

lnMsg Ln_LocoAddr(const RR32Can::MachineLocomotiveAddress& address);
lnMsg Ln_SlotMove(uint8_t src, uint8_t dest);
lnMsg Ln_RequestSlotData(uint8_t slot);
lnMsg Ln_SlotDataRead(uint8_t slot, uint8_t stat, const RR32Can::LocomotiveData& engine);
lnMsg Ln_SlotDataWrite(uint8_t slot, uint8_t stat, const RR32Can::LocomotiveData& engine);
lnMsg Ln_LocoSpeed(uint8_t slotIdx, RR32Can::Velocity_t velocity);
lnMsg Ln_LocoDirf(uint8_t slotIdx, const RR32Can::LocomotiveData& loco);
lnMsg Ln_LocoSnd(uint8_t slotIdx, const RR32Can::LocomotiveData& loco);
lnMsg Ln_LocoSnd2(uint8_t slotIdx, const RR32Can::LocomotiveData& loco);

lnMsg Ln_LocoFunExt(const uint8_t slotIdx, const LocoFunExtBlockId blockId,
                    const RR32Can::LocomotiveData& loco);

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETHELPERS_H__
