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

void CanEngineDB::setLocoFunction(const RR32Can::Uid_t engineUid, uint8_t functionIdx,
                                  bool functionOn) {
  auto entry = getEntry(engineUid);
  if (entry != db_.end()) {
    const bool oldFunctionOn = entry->loco.getFunction(functionIdx);
    if (oldFunctionOn != functionOn) {
      entry->diff.functions |= (1U << functionIdx);
      entry->loco.setFunction(functionIdx, functionOn);
    }
  }
}

void CanEngineDB::setLocoDirection(const RR32Can::Uid_t engineUid,
                                   const RR32Can::EngineDirection direction) {
  auto entry = getEntry(engineUid);
  if (entry != db_.end()) {
    const auto oldDirection = entry->loco.getDirection();
    if (oldDirection != direction) {
      entry->diff.direction = true;
      entry->loco.setDirection(direction);
    }
  }
}

void CanEngineDB::changeLocoDirection(const RR32Can::Uid_t engineUid) {
  auto entry = getEntry(engineUid);
  if (entry != db_.end()) {
    entry->diff.direction = true;
    entry->loco.changeDirection();
  }
}

void CanEngineDB::fetchEnginesFromOffset(uint8_t offset) {
  dbState_ = DBState::DOWNLOADING;
  listConsumer_.reset();
  listConsumer_.setStreamOffset(offset);
  streamParser_.reset();
  listConsumer_.setStreamEndCallback(this);
  streamParser_.startStream(&listConsumer_);
  RR32Can::RR32Can.RequestEngineList(offset, &streamParser_);
}

void CanEngineDB::fetchEngineDB() { fetchEnginesFromOffset(0); }

RR32Can::Locomotive* CanEngineDB::findFirstIncompleteEngine() {
  const auto db_end = std::next(db_.begin(), listConsumer_.getNumEnginesKnownByMaster());
  const auto incompleteEntry = std::find_if(
      db_.begin(), db_end, [](const auto& entry) { return entry.loco.isNameOnlyKnown(); });
  if (incompleteEntry != db_end) {
    return &(incompleteEntry->loco);
  } else {
    return nullptr;
  }
}

void CanEngineDB::fetchNextEngine() {
  auto* const loco = findFirstIncompleteEngine();
  if (loco != nullptr) {
    dbState_ = DBState::DOWNLOADING;
    fetchEngine(*loco);
  } else {
    dbState_ = DBState::COMPLETE;
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
      if (!hasEngine(engineShortInfo.getName())) {
        db_[engineOffset].reset();
        db_[engineOffset].loco.setName(engineShortInfo.getName());
        ++engineOffset;
      }
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

bool CanEngineDB::hasEngine(const char* const engineName) const {
  for (auto it = db_.cbegin(); it != db_.cend() && !it->loco.isFree(); ++it) {
    if (strncmp(engineName, it->loco.getName(), RR32Can::kEngineNameLength) == 0) {
      return true;
    }
  }
  return false;
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
