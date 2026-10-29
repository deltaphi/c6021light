#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <algorithm>
#include <string>

#include "RR32Can/messages/PingResponsePacket.h"
#include "RR32Can/util/constexpr.h"
#include "mocks/RoutingTaskFixture.h"
#include "mocks/SequenceMaker.h"

namespace {
using Protocol = tasks::RoutingTask::EngineDownloadProtocol;
using StateMachine = tasks::RoutingTask::CanEngineDBStateMachine;

RR32Can::CanFrame pingResponse(uint8_t major, uint8_t minor, uint16_t device = 0x0032) {
  RR32Can::CanFrame frame{{RR32Can::Command::PING, 0}, {}};
  frame.id.setResponse(true);
  RR32Can::PingResponsePacket packet(frame.data);
  packet.initData();
  packet.setSoftwareVersionMajor(major);
  packet.setSoftwareVersionMinor(minor);
  packet.setDeviceInformation(device);
  return frame;
}

class CanEngineDBDiscovery : public mocks::RoutingTaskFixture {
 protected:
  void SetUp() override {
    mocks::RoutingTaskFixture::SetUp();
    EXPECT_CALL(statusIndicator, setCanDbDownload()).Times(AnyNumber());
    EXPECT_CALL(statusIndicator, clearCanDbDownload()).Times(AnyNumber());
    EXPECT_CALL(canEngineDBTimer, Start()).Times(AnyNumber());
    EXPECT_CALL(canEngineDBTimer, Stop()).Times(AnyNumber());
    EXPECT_CALL(routingTask, notify()).Times(AnyNumber());
  }

  void loop(RR32Can::CanFrame* frame = nullptr) {
    mocks::makeSequence(i2cHal);
    EXPECT_CALL(i2cHal, getStopGoRequest()).WillOnce(Return(hal::StopGoRequest{}));
    mocks::makeSequence(lnHal);
    if (frame != nullptr) {
      mocks::makeSequence(canHal, *frame);
    } else {
      mocks::makeSequence(canHal);
    }
    routingTask.loop();
  }

  void tick() {
    routingTask.canEngineDBStateM_.TimerCallback(0);
    loop();
  }

  void expectPings(int count) {
    RR32Can::CanFrame ping{{RR32Can::Command::PING, 0}, {}};
    EXPECT_CALL(canTx, SendPacket(ping)).Times(count);
  }

  void expectDownload(Protocol protocol, int count = 1) {
    if (protocol == Protocol::LOKLISTE) {
      EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("lokliste", 8)))
          .Times(count);
    } else {
      EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data(
                             RR32Can::Filenames::kEngineNames, RR32Can::CanDataMaxLength)))
          .Times(count);
      EXPECT_CALL(canTx, SendPacket(RR32Can::util::Request_Config_Data("0 2", 3))).Times(count);
    }
  }

  void startDownload() {
    routingTask.getCANEngineDB().setDownloadProtocol(Protocol::LOKLISTE);
    routingTask.canEngineDBStateM_.startRequesting();
    loop();
  }

  void receiveHeader() {
    auto header = RR32Can::util::Config_Data_Stream(80, 0xAFFE);
    loop(&header);
    ASSERT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::DOWNLOADING);
  }

  void receiveText(const std::string& text) {
    RR32Can::Crc crc;
    crc.reset();
    for (unsigned char c : text) {
      crc.updateCrc(c);
    }
    auto header = RR32Can::util::Config_Data_Stream(text.size(), crc.getCrc());
    loop(&header);
    for (size_t offset = 0; offset < text.size(); offset += 8) {
      char chunk[9]{};
      memcpy(chunk, text.data() + offset, std::min(size_t(8), text.size() - offset));
      auto data = RR32Can::util::Config_Data_Stream(chunk);
      loop(&data);
    }
  }
};

struct VersionCase {
  uint8_t major;
  uint8_t minor;
  Protocol protocol;
};

class CanEngineDBDiscoveryVersion : public CanEngineDBDiscovery,
                                    public WithParamInterface<VersionCase> {};

TEST_P(CanEngineDBDiscoveryVersion, PingSelectsProtocolAndStartsDownload) {
  const auto version = GetParam();
  expectPings(1);
  routingTask.canEngineDBStateM_.startRequesting();
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(),
            StateMachine::RequestState::REQUESTING_PROTOCOL);
  loop();  // No database request before a response, or extra ping without a timeout.
  EXPECT_EQ(routingTask.getCANEngineDB().getDownloadProtocol(), Protocol::UNKNOWN);

  expectDownload(version.protocol);
  auto response = pingResponse(version.major, version.minor);
  loop(&response);
  EXPECT_EQ(routingTask.getCANEngineDB().getDownloadProtocol(), version.protocol);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

INSTANTIATE_TEST_SUITE_P(Versions, CanEngineDBDiscoveryVersion,
                         Values(VersionCase{2, 99, Protocol::LOKNAMEN},
                                VersionCase{3, 54, Protocol::LOKNAMEN},
                                VersionCase{3, 55, Protocol::LOKLISTE},
                                VersionCase{4, 0, Protocol::LOKLISTE}));

TEST_F(CanEngineDBDiscovery, IgnoresOtherDevicesAndMalformedResponses) {
  expectPings(1);
  routingTask.canEngineDBStateM_.startRequesting();
  auto otherDevice = pingResponse(4, 0, 0x0010);
  loop(&otherDevice);
  auto truncated = pingResponse(4, 0);
  truncated.data.dlc = 7;
  loop(&truncated);
  auto request = pingResponse(4, 0);
  request.id.setResponse(false);
  loop(&request);
  EXPECT_EQ(routingTask.getCANEngineDB().getDownloadProtocol(), Protocol::UNKNOWN);
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(),
            StateMachine::RequestState::REQUESTING_PROTOCOL);
}

TEST_F(CanEngineDBDiscovery, RetriesLostPingOrResponseThenDownloads) {
  expectPings(3);
  routingTask.canEngineDBStateM_.startRequesting();
  tick();
  loop();
  tick();
  expectDownload(Protocol::LOKLISTE);
  auto response = pingResponse(3, 55);
  loop(&response);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBDiscovery, DiscoveryTimeoutStopsAndCanBeManuallyRestarted) {
  expectPings(StateMachine::kMaxDownloadTries + 1);
  EXPECT_CALL(canEngineDBTimer, Stop()).Times(1);
  EXPECT_CALL(statusIndicator, clearCanDbDownload()).Times(1);
  routingTask.canEngineDBStateM_.startRequesting();
  for (int i = 0; i < StateMachine::kMaxDownloadTries; ++i) {
    tick();
  }
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  tick();
  routingTask.canEngineDBStateM_.startRequesting();
  expectDownload(Protocol::LOKLISTE);
  auto response = pingResponse(3, 55);
  loop(&response);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBDiscovery, UnrelatedStreamDoesNotInterruptDiscovery) {
  expectPings(1);
  routingTask.canEngineDBStateM_.startRequesting();
  auto header = RR32Can::util::Config_Data_Stream(80, 0xAFFE);
  loop(&header);
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(),
            StateMachine::RequestState::REQUESTING_PROTOCOL);
  expectDownload(Protocol::LOKLISTE);
  auto response = pingResponse(3, 55);
  loop(&response);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

// Recovery is shared by both protocols; do not repeat protocol-specific parsing tests here.
TEST_F(CanEngineDBDiscovery, StalledDownloadRetriesAfterInactivity) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  receiveHeader();
  for (int i = 1; i < StateMachine::kDownloadTimeoutTicks; ++i) {
    tick();
    EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::DOWNLOADING);
  }
  tick();
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::REQUESTING);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBDiscovery, StreamActivityResetsInactivityTimeout) {
  expectDownload(Protocol::LOKLISTE);
  startDownload();
  receiveHeader();
  for (int i = 1; i < StateMachine::kDownloadTimeoutTicks; ++i) {
    tick();
  }
  RR32Can::CanFrame data{{RR32Can::Command::CONFIG_DATA_STREAM, 0}, {}};
  data.data.dlc = 8;
  loop(&data);
  tick();
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::DOWNLOADING);
}

TEST_F(CanEngineDBDiscovery, InvalidStreamImmediatelyRetries) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  receiveHeader();
  RR32Can::CanFrame invalid{{RR32Can::Command::CONFIG_DATA_STREAM, 0}, {}};
  invalid.data.dlc = 5;  // Data packets must contain eight bytes.
  loop(&invalid);
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::REQUESTING);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBDiscovery, CrcFailureImmediatelyRetries) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  auto header = RR32Can::util::Config_Data_Stream(8, 0xAFFE);
  loop(&header);
  RR32Can::CanFrame data{{RR32Can::Command::CONFIG_DATA_STREAM, 0}, {}};
  data.data.dlc = 8;
  loop(&data);
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::REQUESTING);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}

TEST_F(CanEngineDBDiscovery, RepeatedStallsExhaustDownloadRetryBudget) {
  expectDownload(Protocol::LOKLISTE, StateMachine::kMaxDownloadTries);
  EXPECT_CALL(canEngineDBTimer, Stop()).Times(1);
  EXPECT_CALL(statusIndicator, clearCanDbDownload()).Times(1);
  startDownload();
  for (int attempt = 0; attempt < StateMachine::kMaxDownloadTries; ++attempt) {
    receiveHeader();
    for (int i = 0; i < StateMachine::kDownloadTimeoutTicks; ++i) {
      tick();
    }
  }
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  EXPECT_TRUE(routingTask.getCANEngineDB().isEmpty());
  tick();
}

TEST_F(CanEngineDBDiscovery, RepeatedAbortsExhaustDownloadRetryBudget) {
  expectDownload(Protocol::LOKLISTE, StateMachine::kMaxDownloadTries);
  EXPECT_CALL(canEngineDBTimer, Stop()).Times(1);
  EXPECT_CALL(statusIndicator, clearCanDbDownload()).Times(1);
  startDownload();
  for (int attempt = 0; attempt < StateMachine::kMaxDownloadTries; ++attempt) {
    receiveHeader();
    RR32Can::CanFrame invalid{{RR32Can::Command::CONFIG_DATA_STREAM, 0}, {}};
    invalid.data.dlc = 5;
    loop(&invalid);
  }
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  EXPECT_TRUE(routingTask.getCANEngineDB().isEmpty());
  tick();
}

TEST_F(CanEngineDBDiscovery, ConsumerRejectionRetriesAndCanComplete) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  receiveText("[lokliste]\n.idx=-1\n.name=Invalid\n");
  EXPECT_EQ(routingTask.canEngineDBStateM_.GetState(), StateMachine::RequestState::REQUESTING);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
  receiveText("[lokliste]\n.dv=0\n");
  EXPECT_TRUE(routingTask.getCANEngineDB().isComplete());
  EXPECT_TRUE(routingTask.canEngineDBStateM_.isIdle());
}

TEST_F(CanEngineDBDiscovery, CompletedDownloadCanBeManuallyRestartedWithCachedProtocol) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  auto empty = RR32Can::util::Config_Data_Stream(0, 0xAFFE);
  loop(&empty);
  ASSERT_TRUE(routingTask.getCANEngineDB().isComplete());
  ASSERT_TRUE(routingTask.canEngineDBStateM_.isIdle());
  routingTask.canEngineDBStateM_.startRequesting();
  loop();
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
}

TEST_F(CanEngineDBDiscovery, CompletedDownloadCanBeManuallyRestartedWithDiscovery) {
  expectDownload(Protocol::LOKLISTE, 2);
  startDownload();
  auto empty = RR32Can::util::Config_Data_Stream(0, 0xAFFE);
  loop(&empty);
  ASSERT_TRUE(routingTask.getCANEngineDB().isComplete());
  ASSERT_TRUE(routingTask.canEngineDBStateM_.isIdle());

  routingTask.getCANEngineDB().setDownloadProtocol(Protocol::UNKNOWN);
  expectPings(1);
  routingTask.canEngineDBStateM_.startRequesting();
  loop();
  EXPECT_FALSE(routingTask.canEngineDBStateM_.isIdle());
  auto response = pingResponse(3, 55);
  loop(&response);
  EXPECT_TRUE(routingTask.getCANEngineDB().isDownloading());
}
}  // namespace
