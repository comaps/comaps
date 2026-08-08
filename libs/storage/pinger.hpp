#pragma once

#include <string>
#include <vector>

namespace storage
{
class Pinger
{
public:
  using Endpoints = std::vector<std::string>;
  // Pings all endpoints and a returns latency-sorted list of available ones. Works synchronously.
  static Endpoints ExcludeUnavailableAndSortEndpoints(Endpoints const & urls);
};
}  // namespace storage
