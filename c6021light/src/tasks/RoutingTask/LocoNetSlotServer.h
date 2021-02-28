#ifndef __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
#define __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__

#include <algorithm>
#include <array>
#include <cstdint>

#include "RR32Can/Locomotive.h"
#include "RR32Can/Types.h"

#include "LocoNetTx.h"
#include "ln_opc.h"

#include "RoutingForwarder.h"

struct DataModel;

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
    LocoDiff_t diff;
    bool needsMatchToCAN = false;

    bool hasUpdate() const { return diff.hasDiff(); }
  };

  constexpr static const SlotIdx_t kNumSlots = 127;
  using SlotDB_t = std::array<SlotEntry, kNumSlots>;  // note that Slot 0 is not used

  void init(DataModel& dataModel, LocoNetTx& tx) {
    this->dataModel_ = &dataModel;
    this->tx_ = &tx;
  }

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
    initializeSlot(freeIt, addr);
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
    const uint16_t addr = (LnPacket.data[1] << 7) | (LnPacket.data[2] & 0x7F);
    return addr;
  }

  /**
   * \brief Write the current slot status to STDOUT.
   */
  void dump() const;

  bool isSlotInBounds(const SlotDB_t::const_iterator& it) const { return it != slotDB_.end(); }

  bool isDisabled() const;
  bool isPassive() const;
  bool isActive() const;

  auto begin() { return slotDB_.begin(); }
  auto end() { return slotDB_.end(); }

  bool dispatchSlotAvailable() const { return slotInDispatch_ != slotDB_.end(); }

 private:
  void processSlotMove(const slotMoveMsg& msg);

  SlotDB_t::iterator findOrRequestSlot(const uint8_t lnMsgSlot);

  SlotDB_t::iterator findSlot(const uint8_t lnMsgSlot) {
    return std::next(slotDB_.begin(), std::min(lnMsgSlot, kNumSlots));
  }

  SlotDB_t::const_iterator findSlot(const uint8_t lnMsgSlot) const {
    return std::next(slotDB_.begin(), std::min(lnMsgSlot, kNumSlots));
  }

  void requestSlotDataRead(SlotDB_t::iterator slot);
  void sendSlotDataRead(const SlotDB_t::const_iterator slot) const;
  void sendNoDispatch() const;

  void processLocoRequest(const LocoAddr_t locoAddr);
  void processSlotRead(const rwSlotDataMsg& msg);
  void processRequestSlotRead(slotReqMsg slotReq) const;

  void processLocoSpeed(const locoSpdMsg& msg);
  void processLocoDirF(const locoDirfMsg& msg);
  void processLocoSnd(const locoSndMsg& msg, const uint8_t functionOffset);
  void processLocoFunExt(const multiSenseDeviceInfoMsg& msg);

  static void clearSlot(SlotDB_t::iterator& slot) { *slot = SlotEntry(); }

  static void initializeSlot(SlotDB_t::iterator& slot, LocoAddr_t address) {
    clearSlot(slot);
    slot->loco.setAddress(address);
    slot->inUse = true;
    slot->needsMatchToCAN = true;
  }

  auto shouldSendEngineUpdateForSlot(const SlotDB_t::const_iterator slot) {
    return slot->inUse && !isDisabled();
  }

  DataModel* dataModel_;
  LocoNetTx* tx_;
  SlotDB_t slotDB_;
  SlotDB_t::iterator slotInDispatch_{slotDB_.end()};  // slotDB_.end() means no slot.
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__LOCONETSLOTSERVER_H__
