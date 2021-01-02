#include "tasks/RoutingTask/CanEngineDB.h"

#include <algorithm>

#include "RR32Can/RR32Can.h"

namespace tasks {
namespace RoutingTask {

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
  RR32Can::Locomotive* incompleteEngine = std::find_if(
      db_.begin(), db_end, [](const auto& engine) { return engine.isNameOnlyKnown(); });
  fetchEngine(*incompleteEngine);
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
      db_[engineOffset].setName(engineShortInfo.getName());
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
  for (auto& engine : db_) {
    if (!engine.isFree()) {
      printf("Engine %i: ", engineIdx);
      engine.print();
      puts("");
    }
    ++engineIdx;
  }
  puts("--- CAN Engine DB Status.");
}

}  // namespace RoutingTask
}  // namespace tasks
