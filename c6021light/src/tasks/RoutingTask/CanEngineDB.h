#ifndef __TASKS__ROUTINGTASK__CANENGINEDB_H__
#define __TASKS__ROUTINGTASK__CANENGINEDB_H__

#include <array>
#include "RR32Can/Locomotive.h"
#include "RR32CanEngineDb/LocoConsumer.h"
#include "RR32CanEngineDb/LocoListConsumer.h"
#include "RR32CanEngineDb/util/ConfigDataConsumer.h"
#include "RR32CanEngineDb/util/ConfigDataEndStreamCallback.h"
#include "RR32CanEngineDb/util/ConfigDataStreamParser.h"

namespace tasks {
namespace RoutingTask {

/*
 * \brief Class CanEngineDB
 */
class CanEngineDB : public RR32Can::ConfigDataEndStreamCallback {
 public:
  constexpr static const uint8_t kMaxNumDbEntries = 40;

  using DB_t = std::array<RR32Can::Locomotive, kMaxNumDbEntries>;

  void fetchEngineDB();

  void streamComplete(RR32Can::ConfigDataConsumer*) override;
  void streamAborted(RR32Can::ConfigDataConsumer*) override {}

  void dump() const;

 private:
  RR32Can::ConfigDataStreamParser streamParser_;
  RR32Can::LocoListConsumer listConsumer_;
  RR32Can::LocoConsumer locoConsumer_;
  DB_t db_;

  void fetchEnginesFromOffset(uint8_t offset);

  void fetchEngine(RR32Can::Locomotive& loco);
  void fetchNextEngine();
};

}  // namespace RoutingTask
}  // namespace tasks

#endif  // __TASKS__ROUTINGTASK__CANENGINEDB_H__