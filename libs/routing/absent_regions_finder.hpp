#pragma once

#include "routing/regions_decl.hpp"
#include "routing/router_delegate.hpp"

#include "base/thread.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace routing
{
using LocalFileCheckerFn = std::function<bool(std::string const &)>;

/**
 * @brief Generates a list of MWMs needed to build a route.
 *
 * The `AbsentRegionsFinder` class encapsulates generation of MWM names of absent regions needed
 * for building the route between `checkpoints`. For this purpose a separate worker thread is used.
 */
class AbsentRegionsFinder
{
public:
  AbsentRegionsFinder(CountryFileGetterFn const & countryFileGetter, LocalFileCheckerFn const & localFileChecker,
                      std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource);

  /**
   * @brief Creates new thread `m_routerThread` and starts routing in it.
   * @param checkpoints The checkpoints of the route (start, optional intermediate points, destination)
   * @param delegate
   */
  void GenerateAbsentRegions(Checkpoints const & checkpoints, RouterDelegate const & delegate);

  /**
   * @brief Retrieves the MWMs needed to build the route.
   *
   * When called for the first time after `GenerateAbsentRegions()`, this method waits for the
   * routing thread `m_routerThread` to finish and returns results from it. Results are cached and
   * subsequent calls are served from the cache.
   *
   * @param countries Receives the list of MWM names.
   */
  void GetAllRegions(std::set<std::string> & countries);

  /**
   * @brief Retrieves the missing MWMs needed to build the route.
   *
   * This calls `GetAllRegions()` and strips from the result all regions already present on the
   * device, leaving only the missing ones. If the call to `GetAllRegions()` is the first one after
   * calling `GenerateAbsentRegions()`, this involves waiting for the router thread to finish.
   *
   * @param absentCountries Receives the list of missing MWM names.
   */
  void GetAbsentRegions(std::set<std::string> & absentCountries);

private:
  bool AreCheckpointsInSameMwm(Checkpoints const & checkpoints) const;

  CountryFileGetterFn const m_countryFileGetterFn;
  LocalFileCheckerFn const m_localFileCheckerFn;

  std::shared_ptr<NumMwmIds> m_numMwmIds;
  DataSource & m_dataSource;

  std::unique_ptr<threads::Thread> m_routerThread;

  /**
   * @brief Mutex for access to member variables.
   *
   * Methods which access any of the non-`const` class member variables must lock this mutex while
   * doing so.
   */
  std::mutex m_mutex;

  /**
   * @brief Regions required for building the last route.
   *
   * This member is cleared by `GenerateAbsentRegions()` and populated by `GetAllRegions()`.
   */
  std::set<std::string> m_regions;
};
}  // namespace routing
