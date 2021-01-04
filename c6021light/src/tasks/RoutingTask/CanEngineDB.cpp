#include "tasks/RoutingTask/CanEngineDB.h"

#include <algorithm>

#include "RR32Can/RR32Can.h"

namespace tasks {
namespace RoutingTask {

CanEngineDB::DB_t::iterator CanEngineDB::getEntry(const RR32Can::Locomotive::Uid_t uid) {
  auto it = db_.begin();
  for (; it != db_.end(); ++it) {
    if (it->loco.isFullDetailsKnown() && it->loco.getUid() == uid) {
      return it;
    }
  }
  return it;
}

RR32Can::Locomotive* CanEngineDB::getLoco(RR32Can::Locomotive::Uid_t uid) {
  const auto it = getEntry(uid);
  if (it == db_.end()) {
    return nullptr;
  } else {
    return &it->loco;
  }
}

void CanEngineDB::setLocoVelocity(RR32Can::Locomotive::Uid_t engineUid,
                                  RR32Can::Velocity_t velocity) {
  auto entry = getEntry(engineUid);
  if (entry != db_.end()) {
    const auto oldVelocity = entry->loco.getVelocity();
    if (oldVelocity != velocity) {
      entry->diff.velocity = true;
      entry->loco.setVelocity(velocity);
    }
  }
}

void CanEngineDB::setLocoVelocity(RR32Can::Velocity_t velocity) {
  for (auto& entry : db_) {
    if (entry.loco.isFullDetailsKnown()) {
      const auto oldVelocity = entry.loco.getVelocity();
      if (oldVelocity != velocity) {
        entry.diff.velocity = true;
        entry.loco.setVelocity(velocity);
      }
    }
  }
}

void CanEngineDB::fetchEnginesFromOffset(uint8_t offset) {
  listConsumer_.reset();
  streamParser_.reset();
  listConsumer_.setStreamEndCallback(this);
  streamParser_.startStream(&listConsumer_);
  RR32Can::RR32Can.RequestEngineList(offset, &streamParser_);
}

void CanEngineDB::fetchEngineDB() { fetchEnginesFromOffset(0); }

void CanEngineDB::fetchNextEngine() {
  auto db_end = std::next(db_.begin(), listConsumer_.getNumEnginesKnownByMaster());
  const auto incompleteEntry = std::find_if(
      db_.begin(), db_end, [](const auto& entry) { return entry.loco.isNameOnlyKnown(); });
  if (incompleteEntry != db_end) {
    fetchEngine(incompleteEntry->loco);
  }
}

void CanEngineDB::fetchEngine(RR32Can::Locomotive& loco) {
  printf("Fetching Engine '%s'.\n", loco.getName());
  streamParser_.reset();
  locoConsumer_.setEngine(&loco);
  locoConsumer_.setStreamEndCallback(this);
  streamParser_.startStream(&locoConsumer_);
  RR32Can::RR32Can.RequestEngine(loco, &streamParser_);
}

void CanEngineDB::streamComplete(RR32Can::ConfigDataConsumer* consumer) {
  RR32Can::RR32Can.FinishCurrentConfigRequest();

  if (consumer == &listConsumer_) {
    // Copy received engines to database
    const auto& engineShortInfos = listConsumer_.getEngineInfos();
    auto engineOffset = listConsumer_.getStreamOffset();
    const auto nextRequestedStreamOffset = engineOffset + RR32Can::kEngineBrowserEntries;

    for (const auto& engineShortInfo : engineShortInfos) {
      db_[engineOffset].reset();
      db_[engineOffset].loco.setName(engineShortInfo.getName());
      ++engineOffset;
    }

    const bool allEnginesDownloaded =
        nextRequestedStreamOffset >= listConsumer_.getNumEnginesKnownByMaster();
    if (allEnginesDownloaded) {
      fetchNextEngine();
    } else {
      fetchEnginesFromOffset(nextRequestedStreamOffset);
    }
  } else if (consumer == &locoConsumer_) {
    fetchNextEngine();
  }
}

void CanEngineDB::dump() const {
  puts("CAN Engine DB Status:");
  auto engineIdx = 0;
  for (auto& entry : db_) {
    if (!entry.loco.isFree()) {
      printf("Engine %i: ", engineIdx);
      entry.loco.print();
      puts("");
    }
    ++engineIdx;
  }
  puts("--- CAN Engine DB Status.");
}

}  // namespace RoutingTask
}  // namespace tasks
