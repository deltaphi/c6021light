#ifndef __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
#define __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__

#include <algorithm>
#include <array>
#include <cstdint>

#include "RR32Can/Types.h"

#include "LocoNet.h"

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

  using LocoAddr_t = RR32Can::LocId_t;
  using SlotIdx_t = uint8_t;

  struct SlotEntry {
    bool inUse = false;
    LocoAddr_t locoAddress = 0;
  };

  constexpr static const SlotIdx_t kNumSlots = 127;
  using SlotDB_t = std::array<SlotEntry, kNumSlots>;  // note that Slot 0 is not used

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
      return (entry.inUse && entry.locoAddress == addr);
    });
  }

  SlotDB_t::iterator findOrAllocateSlotForAddress(LocoAddr_t addr) {
    SlotDB_t::iterator freeIt = slotDB_.end();
    for (SlotDB_t::iterator addrIt = slotDB_.begin() + 1; addrIt != slotDB_.end(); ++addrIt) {
      if (addrIt->locoAddress == addr) {
        return addrIt;
      } else if (freeIt == slotDB_.end() && !(addrIt->inUse)) {
        freeIt = addrIt;
      }
    }
    return freeIt;
  }

  void process(const lnMsg& LnPacket);

  static uint16_t extractAddress(const lnMsg& LnPacket) {
    uint16_t addr = (LnPacket.data[1] | ((LnPacket.data[2] & 0x0F) << 7));
    return addr;
  }

  /**
   * \brief Write the current slot status to STDOUT.
   */
  void dump() const;

 private:
  bool dispatchSlotAvailable() const { return slotInDispatch_ != slotDB_.end(); }

  void processSlotMove(const slotMoveMsg& msg);
  static bool isDispatchGet(const slotMoveMsg& msg) { return msg.src == 0; }
  static bool isDispatchPut(const slotMoveMsg& msg) { return msg.dest == 0; }
  static bool isNullMove(const slotMoveMsg& msg) { return msg.src == msg.dest; }

  void sendSlotDataRead(SlotDB_t::const_iterator slot) const;
  void sendNoDispatch() const;

  void processLocoRequest(LocoAddr_t locoAddr);
  void processSlotRead(const rwSlotDataMsg& msg);

  bool isDisabled() const;
  bool isPassive() const;
  bool isActive() const;

  bool isSlotInBounds(const SlotDB_t::const_iterator& it) const { return it != slotDB_.end(); }

  static void clearSlot(SlotDB_t::iterator& slot) { *slot = SlotEntry(); }

  static LocoAddr_t getLocoAddress(const rwSlotDataMsg& slotRead) {
    LocoAddr_t address = slotRead.adr2 << 7;
    address |= slotRead.adr & 0x7F;
    return address;
  }

  static void putLocoAddress(rwSlotDataMsg& slotRead, LocoAddr_t address) {
    slotRead.adr = address & 0x7F;  // Loco Address low bits
    slotRead.adr2 = address >> 7;   // Loco Address high bits
  }

  DataModel* dataModel_;
  SlotDB_t slotDB_;
  SlotDB_t::iterator slotInDispatch_;  // slotDB_.end() means no slot.
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
