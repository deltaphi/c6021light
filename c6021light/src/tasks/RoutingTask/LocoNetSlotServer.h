#ifndef __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
#define __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__

#include <algorithm>
#include <array>
#include <cstdint>

#include "RR32Can/Locomotive.h"
#include "RR32Can/Types.h"

#include "ln_opc.h"

#include "RoutingForwarder.h"

class DataModel;

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class LocoNetSlotServer
 *
 * See https://wiki.rocrail.net/doku.php?id=loconet:ln-pe-en for protocol details
 */
class LocoNetSlotServer {
 public:
  enum class SlotServerState : uint8_t { DISABLED = 0, PASSIVE = 1, ACTIVE = 2 };

  using LocoAddr_t = RR32Can::MachineLocomotiveAddress;
  using LocoData_t = RR32Can::LocomotiveData;
  using SlotIdx_t = uint8_t;

  struct SlotEntry {
    bool inUse = false;
    LocoData_t loco;
  };

  constexpr static const SlotIdx_t kNumSlots = 127;
  using SlotDB_t = std::array<SlotEntry, kNumSlots>;  // note that Slot 0 is not used

  constexpr static const RR32Can::Velocity_t kLocoNetMaxVeloctiy = 127;
  constexpr static const uint8_t kDirfDirMask = 0b00100000;  // mask set == reverse
  constexpr static const uint8_t kFunctionsInDirfMessage = 5;

  constexpr static const uint8_t kFunctionsInSndMessage = 4;
  constexpr static const uint8_t kLowestFunctionInSndMessage = 5;

  LocoNetSlotServer(RoutingForwarder& forwarder) : forwarder_(forwarder) {}

  void init(DataModel& dataModel) { this->dataModel_ = &dataModel; }

  bool markAddressForDispatch(LocoAddr_t addr) {
    SlotDB_t::iterator it = findOrAllocateSlotForAddress(addr);
    if (it == slotDB_.end()) {
      return false;
    } else {
      slotInDispatch_ = it;
      return true;
    }
  }

  SlotDB_t::iterator findSlotForAddress(LocoAddr_t addr) {
    return std::find_if(slotDB_.begin(), slotDB_.end(), [addr](const auto& entry) {
      return (entry.inUse && entry.loco.getAddress() == addr);
    });
  }

  SlotDB_t::iterator findOrAllocateSlotForAddress(LocoAddr_t addr) {
    addr = addr.getNumericAddress();
    SlotDB_t::iterator freeIt = slotDB_.end();
    for (SlotDB_t::iterator addrIt = slotDB_.begin() + 1; addrIt != slotDB_.end(); ++addrIt) {
      if (addrIt->loco.getAddress().getNumericAddress() == addr) {
        return addrIt;
      } else if (freeIt == slotDB_.end() && !(addrIt->inUse)) {
        freeIt = addrIt;
      }
    }
    return freeIt;
  }

  uint8_t findSlotIndex(const SlotDB_t::const_iterator& slotIt) const {
    return std::distance(slotDB_.begin(), slotIt);
  }

  void process(const lnMsg& LnPacket);

  static RR32Can::MachineLocomotiveAddress extractLocoAddress(const lnMsg& LnPacket) {
    return RR32Can::MachineLocomotiveAddress(extractAddress(LnPacket));
  }

  static uint16_t extractAddress(const lnMsg& LnPacket) {
    uint16_t addr = (LnPacket.data[1] | ((LnPacket.data[2] & 0x0F) << 7));
    return addr;
  }

  /**
   * \brief Write the current slot status to STDOUT.
   */
  void dump() const;

  bool isSlotInBounds(const SlotDB_t::const_iterator& it) const { return it != slotDB_.end(); }

  static RR32Can::Velocity_t lnSpeedToCanVelocity(RR32Can::Velocity_t speed) {
    return (speed * RR32Can::kMaxEngineVelocity / kLocoNetMaxVeloctiy);
  }

  static RR32Can::Velocity_t canVelocityToLnSpeed(RR32Can::Velocity_t velocity) {
    return (velocity * kLocoNetMaxVeloctiy) / RR32Can::kMaxEngineVelocity;
  }

  static void dirfToLoco(const uint8_t dirf, LocoData_t& loco);
  static uint8_t locoToDirf(const LocoData_t& loco);

  static void sndToLoco(const uint8_t snd, LocoData_t& loco);
  static uint8_t locoToSnd(const LocoData_t& loco);

  bool isDisabled() const;
  bool isPassive() const;
  bool isActive() const;

 private:
  bool dispatchSlotAvailable() const { return slotInDispatch_ != slotDB_.end(); }

  void processSlotMove(const slotMoveMsg& msg);
  static bool isDispatchGet(const slotMoveMsg& msg) { return msg.src == 0; }
  static bool isDispatchPut(const slotMoveMsg& msg) { return msg.dest == 0; }
  static bool isNullMove(const slotMoveMsg& msg) { return msg.src == msg.dest; }

  SlotDB_t::iterator findOrRequestSlot(const uint8_t lnMsgSlot);
  SlotDB_t::iterator findSlot(const uint8_t lnMsgSlot);
  void requestSlotDataRead(const SlotDB_t::const_iterator slot) const;
  void sendSlotDataRead(const SlotDB_t::const_iterator slot) const;
  void sendNoDispatch() const;

  void processLocoRequest(const LocoAddr_t locoAddr);
  void processSlotRead(const rwSlotDataMsg& msg);

  void processLocoSpeed(const locoSpdMsg& msg);
  void processLocoDirF(const locoDirfMsg& msg);
  void processLocoSnd(const locoSndMsg& msg);

  static void clearSlot(SlotDB_t::iterator& slot) { *slot = SlotEntry(); }

  static LocoAddr_t getLocoAddress(const rwSlotDataMsg& slotRead) {
    LocoAddr_t::value_type address = slotRead.adr2 << 7;
    address |= slotRead.adr & 0x7F;
    return LocoAddr_t{address};
  }

  static void putLocoAddress(rwSlotDataMsg& slotRead, const LocoAddr_t address) {
    slotRead.adr = address.value() & 0x7F;  // Loco Address low bits
    slotRead.adr2 = address.value() >> 7;   // Loco Address high bits
  }

  auto shouldSendEngineUpdateForSlot(const SlotDB_t::const_iterator slot) {
    return slot->inUse && !isDisabled();
  }

  DataModel* dataModel_;
  SlotDB_t slotDB_;
  SlotDB_t::iterator slotInDispatch_;  // slotDB_.end() means no slot.
  RoutingForwarder& forwarder_;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
