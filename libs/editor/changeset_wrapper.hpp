#pragma once

#include "editor/server_api.hpp"
#include "editor/xml_feature.hpp"

#include "base/exception.hpp"

#include <map>
#include <string>

class FeatureType;

namespace osm
{
class ChangesetWrapper
{
  using TypeCount = std::map<std::string, size_t>;

public:
  DECLARE_EXCEPTION(ChangesetWrapperException, RootException);
  DECLARE_EXCEPTION(NetworkErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(HttpErrorException, ChangesetWrapperException);
  DECLARE_EXCEPTION(CreateChangeSetFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(ModifyNodeFailedException, ChangesetWrapperException);
  DECLARE_EXCEPTION(LinearFeaturesAreNotSupportedException, ChangesetWrapperException);

  ChangesetWrapper(std::string const & keySecret, ServerApi06::KeyValueTags comments) noexcept;
  ~ChangesetWrapper();

  /// Throws exceptions from above list.
  void Create(editor::XMLFeature node);

  /// Throws exceptions from above list.
  /// Node should have correct OSM "id" attribute set.
  void Modify(editor::XMLFeature node);

  /// Throws exceptions from above list.
  void Delete(editor::XMLFeature node);

  /// Add a tag to the changeset
  void AddChangesetTag(std::string key, std::string value);

  /// Add item to ';' separated list for a changeset key
  void AddToChangesetKeyList(std::string key, std::string value);

private:
  ServerApi06::KeyValueTags m_changesetComments;
  ServerApi06 m_api;
  static constexpr uint64_t kInvalidChangesetId = 0;
  uint64_t m_changesetId = kInvalidChangesetId;
  static constexpr int kMaximumOsmChars = 255;

  TypeCount m_modified_types;
  TypeCount m_created_types;
  TypeCount m_deleted_types;
  static std::string TypeCountToString(TypeCount const & typeCount);
  std::string GetDescription() const;
};

}  // namespace osm
