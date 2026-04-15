#pragma once

#include "generator/collector_interface.hpp"
#include "indexer/route_relation.hpp"

#include "coding/file_writer.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class RelationElement;

namespace generator
{

class RouteRelationsBuilder : public CollectorInterface
{
public:
  explicit RouteRelationsBuilder(std::string const & filename);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;
  void CollectRelation(RelationElement const & e) override;
  void Finish() override;

  IMPLEMENT_COLLECTOR_IFACE(RouteRelationsBuilder);
  void MergeInto(RouteRelationsBuilder & other) const;

  /// Reads the data file produced by Save() and returns wayOsmId -> sorted relation indices.
  static std::unordered_map<uint64_t, std::vector<uint32_t>> LoadWayToRelations(std::string const & dataFilePath);

  // Read an intermediate file from RouteRelationsBuilder and convert it to an mwm section.
  static bool WriteRelationsSections(std::string const & mwmPath, std::string const & dataFilePath);

protected:
  void Save() override;

private:
  std::unique_ptr<FileWriter> m_writer;
};

}  // namespace generator
