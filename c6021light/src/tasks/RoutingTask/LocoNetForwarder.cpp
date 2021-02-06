#include "tasks/RoutingTask/LocoNetForwarder.h"

#include "RR32Can/Constants.h"
#include "RR32Can/messages/S88Event.h"
#include "RR32Can/messages/SystemMessage.h"
#include "RR32Can/messages/TurnoutPacket.h"

#include "tasks/RoutingTask/LocoNetHelpers.h"

namespace tasks {
namespace RoutingTask {

void LocoNetForwarder::forward(const RR32Can::CanFrame& frame) {
  switch (frame.id.getCommand()) {
    case RR32Can::Command::ACCESSORY_SWITCH: {
      const RR32Can::TurnoutPacket turnoutPacket(const_cast<RR32Can::Data&>(frame.data));
      if (!frame.id.isResponse()) {
        // Send to LocoNet
        auto msg = Ln_Turnout(turnoutPacket.getLocid(), turnoutPacket.getDirection(),
                              turnoutPacket.getPower());
        tx_->AsyncSend(msg);
      }
      break;
    }

    case RR32Can::Command::SYSTEM_COMMAND: {
      const RR32Can::SystemMessage systemMessage(const_cast<RR32Can::Data&>(frame.data));
      switch (systemMessage.getSubcommand()) {
        case RR32Can::SystemSubcommand::SYSTEM_STOP:
          if (!frame.id.isResponse()) {
            auto msg = Ln_Off();
            tx_->AsyncSend(msg);
          }
          break;
        case RR32Can::SystemSubcommand::SYSTEM_GO:
          if (!frame.id.isResponse()) {
            auto msg = Ln_On();
            tx_->AsyncSend(msg);
          }
          break;
        default:
          // Other messages not forwarded.
          break;
      }
      break;
    }
    case RR32Can::Command::S88_EVENT: {
      const RR32Can::S88Event s88Event(const_cast<RR32Can::Data&>(frame.data));
      if (s88Event.getSubtype() == RR32Can::S88Event::Subtype::RESPONSE) {
        auto msg = Ln_Sensor(s88Event.getContactId(), s88Event.getNewState());
        tx_->AsyncSend(msg);
      }
      break;
    }
    default:
      // Other messages not forwarded.
      break;
  }
}

void LocoNetForwarder::forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) {
  // In passive mode:

  // Get a hold of the slot server
  // Find if there is a slot known for the loco
  // If so, send messages for that slot.

  // Future TODO: If loco is not known, remember the request in some queue and send out a request
  // for the Address. Associate a timeout and clean the entry from the queue if the timeout is
  // exceeded.

  // In active mode:
  // Allocate a slot for the loco and start sending packets immediately.

  if (slotServer_->isPassive()) {
    const auto slotIt = slotServer_->findSlotForAddress(loco.getAddress());
    const uint8_t slotIdx = slotServer_->findSlotIndex(slotIt);
    if (slotServer_->isSlotInBounds(slotIt)) {
      if (diff.velocity) {
        lnMsg msg;
        locoSpdMsg& speedMessage = msg.lsp;
        speedMessage.command = OPC_LOCO_SPD;
        speedMessage.slot = slotIdx;
        speedMessage.spd = canVelocityToLnSpeed(loco.getVelocity());
        tx_->AsyncSend(msg);
        diff.velocity = false;
      }

      if (diff.direction || ((diff.functions & 0x1F) != 0)) {
        lnMsg msg;
        locoDirfMsg& dirfMessage = msg.ldf;
        dirfMessage.command = OPC_LOCO_DIRF;
        dirfMessage.slot = slotIdx;
        dirfMessage.dirf = locoToDirf(loco);
        tx_->AsyncSend(msg);
        diff.direction = false;
      }

      if (((diff.functions & 0x1E0) != 0)) {
        lnMsg msg;
        locoSndMsg& sndMessage = msg.ls;
        sndMessage.command = OPC_LOCO_SND;
        sndMessage.slot = slotIdx;
        sndMessage.snd = locoToSnd(loco);
        tx_->AsyncSend(msg);
      }

      diff.functions = 0;
    }
  }
}

bool LocoNetForwarder::MakeRR32CanMsg(const lnMsg& LnPacket, RR32Can::CanFrame& frame) {
  // Decode the opcode
  switch (LnPacket.data[0]) {
    case OPC_SW_REQ: {
      frame.id.setCommand(RR32Can::Command::ACCESSORY_SWITCH);
      frame.id.setResponse(false);
      RR32Can::TurnoutPacket turnoutPacket(frame.data);
      turnoutPacket.initData();

      // Extract the switch address
      RR32Can::MachineTurnoutAddress lnAddr{static_cast<RR32Can::MachineTurnoutAddress::value_type>(
          ((LnPacket.srq.sw2 & 0x0F) << 7) | LnPacket.srq.sw1)};
      lnAddr.setProtocol(dataModel_->accessoryRailProtocol);
      turnoutPacket.setLocid(lnAddr);

      RR32Can::TurnoutDirection direction =
          ((LnPacket.srq.sw2 & OPC_SW_REQ_DIR) == 0 ? RR32Can::TurnoutDirection::RED
                                                    : RR32Can::TurnoutDirection::GREEN);
      turnoutPacket.setDirection(direction);
      uint8_t power = LnPacket.srq.sw2 & OPC_SW_REQ_OUT;
      turnoutPacket.setPower(power);

      return true;
      break;
    }
    case OPC_GPON:
    case OPC_GPOFF: {
      frame.id.setCommand(RR32Can::Command::SYSTEM_COMMAND);
      frame.id.setResponse(false);
      RR32Can::SystemMessage systemMessage(frame.data);
      systemMessage.initData();

      if (LnPacket.data[0] == OPC_GPON) {
        systemMessage.setSubcommand(RR32Can::SystemSubcommand::SYSTEM_GO);
      } else {
        systemMessage.setSubcommand(RR32Can::SystemSubcommand::SYSTEM_STOP);
      }

      return true;
      break;
    }
    case OPC_INPUT_REP: {
      frame.id.setCommand(RR32Can::Command::S88_EVENT);
      frame.id.setResponse(true);
      RR32Can::S88Event message(frame.data);
      message.initData();
      message.setSubtype(RR32Can::S88Event::Subtype::RESPONSE);
      message.setDeviceId(RR32Can::MachineTurnoutAddress(0));

      {
        uint16_t addr = LnPacket.ir.in1 << 1;
        addr |= (LnPacket.ir.in2 & 0x0F) << 8;
        addr |= (LnPacket.ir.in2 & 0x20) >> 5;
        RR32Can::MachineTurnoutAddress lnAddr(addr);

        message.setContactId(lnAddr);
      }
      {
        RR32Can::SensorState newState;
        RR32Can::SensorState oldState;
        if (((LnPacket.ir.in2 & 0x10) >> 4) == 0) {
          newState = RR32Can::SensorState::OPEN;
          oldState = RR32Can::SensorState::CLOSED;
        } else {
          newState = RR32Can::SensorState::CLOSED;
          oldState = RR32Can::SensorState::OPEN;
        }

        message.setStates(oldState, newState);
      }
      message.setTime(0);
      return true;
      break;
    }
    default:
      // Other packet types not handled.
      return false;
      break;
  }
}

}  // namespace RoutingTask
}  // namespace tasks
