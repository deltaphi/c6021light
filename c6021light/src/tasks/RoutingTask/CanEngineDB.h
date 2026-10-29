#ifndef __TASKS__ROUTINGTASK__CANENGINEDB_H__
#define __TASKS__ROUTINGTASK__CANENGINEDB_H__

#include <array>
#include "RR32Can/Locomotive.h"
#include "RR32Can/callback/EngineCbk.h"
#include "RR32Can/callback/PingCbk.h"
#include "RR32CanEngineDb/LocoConsumer.h"
#include "RR32CanEngineDb/LocoListConsumer.h"
#include "RR32CanEngineDb/LoklisteConsumer.h"
#include "RR32CanEngineDb/LoklisteConsumerCallback.h"
#include "RR32CanEngineDb/util/ConfigDataConsumer.h"
#include "RR32CanEngineDb/util/ConfigDataEndStreamCallback.h"
#include "RR32CanEngineDb/util/ConfigDataStreamParser.h"

#include "EngineDownloadProtocol.h"
#include "IStatusIndicator.h"
#include "RoutingForwarder.h"

namespace tasks {
namespace RoutingTask {

/**
 * \brief CAN locomotive database and download/consumer callbacks.
 *
 * Downloading is coordinated by several cooperating state machines:
 *
 * 1. CanEngineDBStateMachine: discovery, request retries and supervision
 * ------------------------------------------------------------------
 * Startup and manual downloads enter through startRequesting(), which aborts
 * any current configuration request and clears this database (including its
 * previous completion state). Its RequestState transitions are:
 *
 *   IDLE -- start, protocol UNKNOWN --> REQUESTING_PROTOCOL
 *   IDLE -- start, protocol known   --> REQUESTING
 *   REQUESTING_PROTOCOL -- protocol known --> REQUESTING
 *   REQUESTING -- configuration stream header (DLC 6 or 7) --> DOWNLOADING
 *   DOWNLOADING -- abort or inactivity timeout --> REQUESTING
 *   Any active state -- database COMPLETE --> IDLE
 *   REQUESTING_PROTOCOL / REQUESTING -- attempts exhausted --> IDLE
 *
 * startRequesting() can also restart an active operation. In
 * REQUESTING_PROTOCOL, an initial ping is sent immediately and repeated on
 * the one-second request timer, up to kMaxDownloadTries total pings.
 * Station dispatches valid ping responses to HandlePingResponse(); only MS2
 * devices (device information 0x0032) select a protocol:
 *   - version < 3.55: LOKNAMEN
 *   - version >= 3.55: LOKLISTE (including later major versions)
 * A known protocol skips discovery. It is retained across downloads and can
 * be set explicitly; setting UNKNOWN requests discovery on the next start.
 * Later MS2 ping responses can update the selected protocol as well.
 *
 * Entering REQUESTING starts a separate download-attempt budget. The first
 * fetch is scheduled immediately; unanswered initial requests are retried on
 * each timer tick. Once DOWNLOADING, configuration-stream traffic resets an
 * inactivity counter. kDownloadTimeoutTicks (currently five) processed timer
 * ticks without such traffic, or a consumer/parser abort, trigger a retry.
 * Every retry aborts the old stream and restarts the whole database download,
 * not just the failed page or locomotive. At most kMaxDownloadTries database
 * fetches are attempted; exhaustion aborts the stream, clears partial results,
 * stops the timer and clears the download indicator. Discovery exhaustion
 * also stops the timer/indicator. A new startRequesting() resets the budgets.
 *
 * 2. CanEngineDB: database lifecycle and download sequencing
 * -------------------------------------------------------
 * DBState is independent of RequestState: EMPTY after construction/clear(),
 * DOWNLOADING once a fetch starts (even before its first response), FAILED on
 * streamAborted(), and COMPLETE when no name-only entries remain.
 * fetchEngineDB() itself does not discover a protocol or supervise retries;
 * with UNKNOWN it only clears the database. Use the supervisor entry point
 * above for a complete operation.
 *
 * LOKNAMEN downloads name-list pages through LocoListConsumer, starting at
 * offset zero. streamComplete() copies populated, nonduplicate entries and
 * requests further pages until the master's reported count is covered.
 * LOKLISTE instead uses one LoklisteConsumer stream, which writes names into
 * database slots by index through getLokInfoShort().
 *
 * Both paths then call fetchNextEngine(): each name-only locomotive is fetched
 * by name through LocoConsumer. Its completion advances to the next incomplete
 * entry. When none remain (also for an empty list), DBState becomes COMPLETE.
 * RoutingTask observes isComplete() and calls notifyEngineDBComplete() on the
 * supervisor, returning it to IDLE and stopping its timer/indicator.
 *
 * 3. Station and ConfigDataStreamParser: one configuration stream at a time
 * ---------------------------------------------------------------------
 * Station holds the active configuration callback. Each list/page/locomotive
 * request uses the shared streamParser_ with the corresponding consumer:
 *
 *   IDLE -- startStream --> WAITING_FIRST_PACKET
 *   WAITING_FIRST_PACKET -- nonempty header --> WAITING_DATA_PACKET
 *   WAITING_FIRST_PACKET -- empty header --> STREAM_DONE
 *   WAITING_DATA_PACKET -- last bytes, valid CRC --> STREAM_DONE
 *
 * The parser feeds text to the consumer and validates framing and CRC.
 * Malformed framing, CRC failure or rejected consumer content invokes the
 * abort path, setting DBState::FAILED for supervisor recovery. Successful
 * completion calls streamComplete(), which first releases Station's current
 * request before starting the next stream. AbortCurrentConfigRequest() also
 * resets/releases the parser; partial database entries are cleared on retry.
 *
 * RoutingTask processes received CAN packets through Station before passing
 * them to the supervisor's handlePacket(), then checks completion and calls
 * the supervisor's loop(). StateMachineBase's timer callback only records an
 * expiry and notifies RoutingTask; timeout/retry work happens in loop().
 */
class CanEngineDB : public RR32Can::ConfigDataEndStreamCallback,
                    public RR32Can::callback::EngineCbk,
                    public RR32Can::callback::PingCbk,
                    public RR32Can::LokListeConsumerCallback {
 public:
  constexpr static const uint8_t kMaxNumDbEntries = 40;

  struct DbEntry_t {
    RR32Can::Locomotive loco;
    LocoDiff_t diff;

    void reset() {
      loco.reset();
      diff = LocoDiff_t();
    }

    bool hasUpdate() const { return diff.hasDiff(); }
  };

  void init(IStatusIndicator& si) { this->statusIndicator = &si; }

  using DB_t = std::array<DbEntry_t, kMaxNumDbEntries>;

  enum class DBState { EMPTY, DOWNLOADING, COMPLETE, FAILED };

  void fetchEngineDB();

  void HandlePingResponse(const RR32Can::PingResponsePacket& packet) override;
  void HandlePing(const RR32Can::CanFrame&) override {}
  void setDownloadProtocol(EngineDownloadProtocol downloadProtocol) {
    this->downloadProtocol_ = downloadProtocol;
  }
  bool setDownloadProtocol(const char* downloadProtocolAsText);
  EngineDownloadProtocol getDownloadProtocol() const { return downloadProtocol_; };

  void streamComplete(RR32Can::ConfigDataConsumer*) override;
  void streamAborted(RR32Can::ConfigDataConsumer*) override { dbState_ = DBState::FAILED; }

  void dump() const;

  // Overrides for EngineCbk
  void setLocoVelocity(RR32Can::Locomotive::Uid_t engineUid, RR32Can::Velocity_t velocity) override;
  void setLocoVelocity(RR32Can::Velocity_t velocity) override;
  void setLocoFunction(const RR32Can::Uid_t engineUid, uint8_t functionIdx,
                       bool functionOn) override;
  void setLocoDirection(const RR32Can::Uid_t engineUid,
                        const RR32Can::EngineDirection direction) override;
  void changeLocoDirection(const RR32Can::Uid_t engineUid) override;

  auto begin() { return db_.begin(); }
  auto end() { return db_.end(); }

  const RR32Can::Locomotive* findEngine(const RR32Can::Locomotive::Address_t address) const {
    for (auto& entry : db_) {
      if (entry.loco.isFullDetailsKnown() && (address == entry.loco.getAddress())) {
        return &entry.loco;
      }
    }
    return nullptr;
  }

  bool isEmpty() const { return dbState_ == DBState::EMPTY; }
  bool isDownloading() const { return dbState_ == DBState::DOWNLOADING; }
  bool isComplete() const { return dbState_ == DBState::COMPLETE; }
  bool isFailed() const { return dbState_ == DBState::FAILED; }

  void clear() {
    dbState_ = DBState::EMPTY;
    for (auto it = db_.begin(); it != db_.end(); ++it) {
      it->reset();
    }
  }

  RR32Can::LocomotiveShortInfo* getLokInfoShort(uint32_t index) override {
    if (index >= db_.size()) {
      return nullptr;
    } else {
      return &db_[index].loco;
    }
  }

 private:
  RR32Can::ConfigDataStreamParser streamParser_;
  RR32Can::LocoListConsumer listConsumer_;
  RR32Can::LoklisteConsumer loklisteConsumer_{*this};
  RR32Can::LocoConsumer locoConsumer_;
  DB_t db_;
  DBState dbState_{DBState::EMPTY};
  EngineDownloadProtocol downloadProtocol_{EngineDownloadProtocol::UNKNOWN};
  IStatusIndicator* statusIndicator{nullptr};

  DB_t::iterator getEntry(const RR32Can::Locomotive::Uid_t uid);
  RR32Can::Locomotive* findFirstIncompleteEngine();

  void fetchEnginesFromOffset(uint8_t offset);
  void fetchLokliste();

  void fetchEngine(RR32Can::Locomotive& loco);
  void fetchNextEngine();

  bool hasEngine(const char* const engineName) const;
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANENGINEDB_H__
