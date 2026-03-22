#pragma once

#include "storage/country_tree.hpp"
#include "storage/storage_defines.hpp"

#include <optional>
#include <string>
#include <vector>

namespace storage
{
// Loads CountryTree only, without affiliations/synonyms/catalogIds.
std::optional<CountryTree> LoadCountriesFromFile(std::string const & path);

// Returns topmost country id prior root id or |countryId| itself, if it's already a topmost node or
// disputed territory id if |countryId| is a disputed territory or belongs to disputed territory.
CountryId GetTopmostParentFor(CountryTree const & countries, CountryId const & countryId);

// Returns the list of top-level country/group IDs from the tree (direct children of root).
// Excludes "World" and "WorldCoasts". Suitable for the "Avoid specific countries" picker.
std::vector<CountryId> GetTopLevelCountries(CountryTree const & countries);
}  // namespace storage
