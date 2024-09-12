#include "tasks/RoutingTask/XpressNetForwarder.h"

#include "XpressNetMaster.h"

#include "RR32Can/Constants.h"
#include "RR32Can/messages/S88Event.h"
#include "RR32Can/messages/SystemMessage.h"
#include "RR32Can/messages/TurnoutPacket.h"
#include "RR32Can/util/constexpr.h"

namespace tasks {
namespace RoutingTask {

void XpressNetForwarder::forward(const RR32Can::CanFrame& frame) {
  switch (frame.id.getCommand()) {
    case RR32Can::Command::ACCESSORY_SWITCH: {
      const RR32Can::TurnoutPacket turnoutPacket(const_cast<RR32Can::Data&>(frame.data));
      if (!frame.id.isResponse()) {
        // Send to XpressNet
        // convert CAN turnout direction
        uint8_t xn_direction;
        if (turnoutPacket.getDirection() == RR32Can::TurnoutDirection::GREEN) {
          xn_direction = 1;
        } else {
          xn_direction = 0;
        }

        XpressNet.SetTrntPos(turnoutPacket.getLocid().getNumericAddress().value() + 4, xn_direction,
                             turnoutPacket.getPower());
      }
      break;
    }

    case RR32Can::Command::SYSTEM_COMMAND: {
      const RR32Can::SystemMessage systemMessage(const_cast<RR32Can::Data&>(frame.data));
      switch (systemMessage.getSubcommand()) {
        case RR32Can::SystemSubcommand::SYSTEM_STOP: {
          if (!frame.id.isResponse()) {
            XpressNet.setPower(csTrackVoltageOff);
          }
          break;
        }
        case RR32Can::SystemSubcommand::SYSTEM_GO: {
          if (!frame.id.isResponse()) {
            XpressNet.setPower(csNormal);
          }
          break;
        }
        default:
          // Other messages not forwarded.
          break;
      }
      break;
    }

    default:
      // Other messages not forwarded.
      break;
  }
}

void XpressNetForwarder::forwardLocoChange(const RR32Can::LocomotiveData& loco, LocoDiff_t& diff) {
  // nothing here yet..
}

bool XpressNetForwarder::MakeRR32CanMsg(const XpressNetMsg::XN_Msg_t& XnPacket,
                                        RR32Can::CanFrame& frame) {
  // Decode the message type
  switch (XnPacket.header) {
    case XpressNetMsg::POWER: {
      if (XnPacket.data.powerData == csNormal) {
        frame = RR32Can::util::System_Go(false);
      } else if (XnPacket.data.powerData == csTrackVoltageOff) {
        frame = RR32Can::util::System_Stop(false);
      } else {
        return false;
        break;
      }

      return true;
      break;
    }

    default:
      // Other packet types not handled for now.
      return false;
      break;
  }
}

}  // namespace RoutingTask
}  // namespace tasks
