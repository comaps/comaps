#include "generator/route_relations_builder.hpp"

#include "generator/intermediate_elements.hpp"

#include "indexer/features_offsets_table.hpp"
#include "indexer/route_relation.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace generator
{
namespace
{

std::optional<uint32_t> ParseOsmColorName(std::string_view name)
{
  static constexpr std::pair<std::string_view, uint32_t> kColors[] = {
      {"black", 0x000000FF}, {"blue", 0x0A55A3FF},   {"brown", 0xA52A2AFF},
      {"green", 0x00824FFF}, {"orange", 0xFFA500FF}, {"purple", 0x800080FF},
      {"red", 0xB71B1BFF},   {"white", 0xFFFFFFFF},  {"yellow", 0xFFCC00FF},
  };
  for (auto const & [n, rgba] : kColors)
    if (name == n)
      return rgba;
  return {};
}

std::optional<uint32_t> ExtractColor(RelationElement const & e)
{
  // osmc:symbol=waycolor:background:foreground:foreground2:text:textcolor
  // Example value is osmc:symbol=green:black_circle:green_dot
  // We are extracting the first color (waycolor)
  // https://wiki.openstreetmap.org/wiki/Key:osmc:symbol
  std::string_view osmc = e.GetTagValue("osmc:symbol");
  if (!osmc.empty())
  {
    std::string::size_type colon = osmc.find(':');
    if (colon != std::string_view::npos)
      return ParseOsmColorName(osmc.substr(0, colon));
  }
  return {};
}

std::optional<feature::RouteRelationBase::Type> GetRouteType(RelationElement const & e)
{
  if (e.GetType() != "route")
    return {};
  auto const route = e.GetTagValue("route");
  if (route == "hiking")
    return feature::RouteRelationBase::Type::Hiking;
  if (route == "foot")
    return feature::RouteRelationBase::Type::Foot;
  if (route == "bicycle")
    return feature::RouteRelationBase::Type::Bicycle;
  if (route == "mtb")
    return feature::RouteRelationBase::Type::MTB;
  return {};
}

bool ReadRecord(ReaderSource<FileReader> & src, feature::RouteRelationBase::Type & type, uint32_t & colorRGBA,
                std::vector<uint64_t> & sortedWays, std::string & network)
{
  if (src.Size() == 0)
    return false;
  type = static_cast<feature::RouteRelationBase::Type>(ReadPrimitiveFromSource<uint8_t>(src));
  colorRGBA = ReadPrimitiveFromSource<uint32_t>(src);
  uint32_t const numWays = ReadVarUint<uint32_t>(src);
  sortedWays.resize(numWays);
  for (auto & w : sortedWays)
    w = ReadPrimitiveFromSource<uint64_t>(src);
  rw::Read(src, network);
  return true;
}

void ReadFinalRelation(ReaderSource<FileReader> & src, feature::RouteRelationBase::Type & type, uint32_t & colorRGBA,
                       std::string & network)
{
  type = static_cast<feature::RouteRelationBase::Type>(ReadPrimitiveFromSource<uint8_t>(src));
  colorRGBA = ReadPrimitiveFromSource<uint32_t>(src);
  rw::Read(src, network);
}

}  // namespace

// RouteRelationsBuilder -----------------------------------------------------------------------

RouteRelationsBuilder::RouteRelationsBuilder(std::string const & filename)
  : CollectorInterface(filename)
  , m_writer(std::make_unique<FileWriter>(GetTmpFilename()))
{}

std::shared_ptr<generator::CollectorInterface> RouteRelationsBuilder::Clone(IDRInterfacePtr const &) const
{
  return std::make_shared<RouteRelationsBuilder>(GetFilename());
}

void RouteRelationsBuilder::CollectRelation(RelationElement const & e)
{
  auto const type = GetRouteType(e);
  if (!type)
    return;
  if (e.m_ways.empty())
    return;

  auto const colorRGBA = ExtractColor(e);
  if (!colorRGBA)
    return;
  std::string const network(e.GetTagValue("network"));

  // Build sorted way list (used as canonical identity of this relation).
  std::vector<uint64_t> sortedWays;
  sortedWays.reserve(e.m_ways.size());
  for (auto const & [wayId, role] : e.m_ways)
    sortedWays.push_back(wayId);
  std::sort(sortedWays.begin(), sortedWays.end());

  WriteToSink(*m_writer, static_cast<uint8_t>(type.value()));
  WriteToSink(*m_writer, colorRGBA.value());
  WriteVarUint(*m_writer, static_cast<uint32_t>(sortedWays.size()));
  for (uint64_t w : sortedWays)
    WriteToSink(*m_writer, w);
  rw::Write(*m_writer, network);
}

void RouteRelationsBuilder::Finish()
{
  m_writer.reset();
}

void RouteRelationsBuilder::MergeInto(RouteRelationsBuilder & other) const
{
  CHECK(!m_writer && !other.m_writer, ("Finish() must be called before MergeInto()"));
  base::AppendFileToFile(GetTmpFilename(), other.GetTmpFilename());
}

void RouteRelationsBuilder::Save()
{
  // Read all records from the tmp file; deduplicate by sorted-ways key.
  // Assign each unique relation an index; build wayOsmId -> [relIdx] map.

  struct RelData
  {
    feature::RouteRelationBase::Type type;
    uint32_t colorRGBA;
    std::string network;
  };

  // canonical key (sorted way IDs) -> relation index
  std::map<std::vector<uint64_t>, uint32_t> keyToIdx;
  std::vector<RelData> relations;
  std::unordered_map<uint64_t, std::vector<uint32_t>> wayToRelations;

  {
    FileReader reader(GetTmpFilename());
    ReaderSource<FileReader> src(reader);
    feature::RouteRelationBase::Type type;
    uint32_t colorRGBA;
    std::vector<uint64_t> sortedWays;
    std::string network;

    while (ReadRecord(src, type, colorRGBA, sortedWays, network))
    {
      auto [it, inserted] = keyToIdx.emplace(sortedWays, static_cast<uint32_t>(relations.size()));
      uint32_t const relIdx = it->second;

      if (inserted)
        relations.push_back({type, colorRGBA, network});

      for (uint64_t wayId : sortedWays)
      {
        auto & relIds = wayToRelations[wayId];
        if (std::find(relIds.begin(), relIds.end(), relIdx) == relIds.end())
          relIds.push_back(relIdx);
      }
    }
  }

  if (relations.empty())
  {
    LOG(LINFO, ("RouteRelationsBuilder: no route relations found."));
    FileWriter writer(GetFilename());
    WriteToSink(writer, uint32_t(0));  // numRelations
    WriteToSink(writer, uint32_t(0));  // numWayMappings
    return;
  }

  LOG(LINFO, ("RouteRelationsBuilder: collected", relations.size(), "unique route relations covering",
              wayToRelations.size(), "ways."));

  FileWriter writer(GetFilename());

  // Write relations.
  WriteToSink(writer, static_cast<uint32_t>(relations.size()));
  for (auto const & rel : relations)
  {
    WriteToSink(writer, static_cast<uint8_t>(rel.type));
    WriteToSink(writer, rel.colorRGBA);
    rw::Write(writer, rel.network);
  }

  // Write way mappings.
  WriteToSink(writer, static_cast<uint32_t>(wayToRelations.size()));
  for (auto const & [wayId, relIds] : wayToRelations)
  {
    WriteToSink(writer, wayId);
    WriteToSink(writer, static_cast<uint32_t>(relIds.size()));
    for (uint32_t id : relIds)
      WriteToSink(writer, id);
  }
}

// static
std::unordered_map<uint64_t, std::vector<uint32_t>> RouteRelationsBuilder::LoadWayToRelations(
    std::string const & dataFilePath)
{
  std::unordered_map<uint64_t, std::vector<uint32_t>> result;

  if (!Platform::IsFileExistsByFullPath(dataFilePath))
    return result;

  FileReader reader(dataFilePath);
  ReaderSource<FileReader> src(reader);

  uint32_t const numRelations = ReadPrimitiveFromSource<uint32_t>(src);
  // Skip relation metadata (we only need the way mappings here).
  for (uint32_t i = 0; i < numRelations; ++i)
  {
    feature::RouteRelationBase::Type type;
    uint32_t colorRGBA;
    std::string network;
    ReadFinalRelation(src, type, colorRGBA, network);
  }

  uint32_t const numMappings = ReadPrimitiveFromSource<uint32_t>(src);
  result.reserve(numMappings);
  for (uint32_t i = 0; i < numMappings; ++i)
  {
    uint64_t const wayId = ReadPrimitiveFromSource<uint64_t>(src);
    uint32_t const numIds = ReadPrimitiveFromSource<uint32_t>(src);
    std::vector<uint32_t> relIds(numIds);
    for (auto & id : relIds)
      id = ReadPrimitiveFromSource<uint32_t>(src);
    result.emplace(wayId, std::move(relIds));
  }

  return result;
}

// static
bool RouteRelationsBuilder::WriteRelationsSections(std::string const & mwmPath, std::string const & dataFilePath)
{
  if (!Platform::IsFileExistsByFullPath(dataFilePath))
  {
    LOG(LWARNING, ("Route relations data file not found:", dataFilePath));
    return false;
  }

  FileReader reader(dataFilePath);
  ReaderSource<FileReader> src(reader);

  uint32_t const numRelations = ReadPrimitiveFromSource<uint32_t>(src);
  if (numRelations == 0)
  {
    LOG(LINFO, ("No route relations to write for", mwmPath));
    return true;
  }

  // Serialize each relation to a buffer and record offsets for rel_offs section.
  std::vector<uint8_t> relationsData;
  MemWriter<std::vector<uint8_t>> relWriter(relationsData);

  feature::FeaturesOffsetsTable::Builder offsetsBuilder;

  for (uint32_t i = 0; i < numRelations; ++i)
  {
    feature::RouteRelationBase::Type type;
    uint32_t colorRGBA;
    std::string network;
    ReadFinalRelation(src, type, colorRGBA, network);

    feature::RouteRelationBase rel;
    rel.m_type = type;
    if (colorRGBA != 0)
      rel.m_color = dp::Color::FromRGBA(colorRGBA);
    rel.m_network = network;

    offsetsBuilder.PushOffset(static_cast<uint32_t>(relationsData.size()));
    rel.Write(relWriter);
  }

  // Skip way mappings (not needed here).

  // Build the offsets table.
  auto offsetsTable = feature::FeaturesOffsetsTable::Build(offsetsBuilder);

  // Write `relations` section.
  {
    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    auto writer = cont.GetWriter(RELATIONS_FILE_TAG);
    writer->Write(relationsData.data(), relationsData.size());
  }

  // Write `rel_offs` section (save to tmp file, then write into container).
  {
    std::string const tmpPath = mwmPath + ".rel_offs.tmp";
    offsetsTable->Save(tmpPath);
    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    cont.Write(tmpPath, RELATION_OFFSETS_FILE_TAG);
    Platform::RemoveFileIfExists(tmpPath);
  }

  LOG(LINFO, ("Written", numRelations, "route relations to", mwmPath));
  return true;
}

}  // namespace generator
