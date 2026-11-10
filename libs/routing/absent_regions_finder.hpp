#pragma once

#include "routing/regions_decl.hpp"
#include "routing/routing_callbacks.hpp"

#include "base/thread.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>

namespace routing
{
class RouterDelegate;

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
  AbsentRegionsFinder(CountryFileGetterFn countryFileGetter, LocalFileCheckerFn localFileChecker,
                      std::shared_ptr<NumMwmIds> numMwmIds, DataSource & dataSource);

  /**
   * @brief Prepares the set of absent regions (MWMs) for later retrieval.
   *
   * This determines the MWMs in which the checkpoints are located. If the checkpoints are located in
   * different MWMs, it creates a new thread `m_routerThread` and starts routing in it.
   *
   * If any of `checkpoints` is located in a missing MWM, it is added to `m_absentRegions`.
   *
   * @param checkpoints The checkpoints of the route (start, optional intermediate points, destination)
   * @param delegate
   * @return A `RouterResultCode` returned by the router (`RouterResultCode::NoError` if the checkpoints
   * are located in the same MWM)
   */
  RouterResultCode GenerateAbsentRegions(Checkpoints const & checkpoints, RouterDelegate const & delegate);

  using RegionsSetT = std::set<std::string>;

  /**
   * @brief Retrieves the MWMs needed to build the route.
   *
   * When called for the first time after `GenerateAbsentRegions()`, this method waits for the
   * routing thread `m_routerThread` to finish and returns results from it. Results are cached and
   * subsequent calls are served from the cache.
   *
   * @return the list of MWM names.
   */
  RegionsSetT GetAllRegions();

  /**
   * @brief Retrieves the missing MWMs needed to build the route.
   *
   * This calls `GetAllRegions()` and strips from the result all regions already present on the
   * device, leaving only the missing ones. These are then enriched with the contents of
   * `m_absentRegions`, the latter is cleared.  If the call to `GetAllRegions()` is the first one after
   * calling `GenerateAbsentRegions()`, this involves waiting for the router thread to finish.
   *
   * @return the list of missing MWM names.
   */
  RegionsSetT GetAbsentRegions();

private:
  CountryFileGetterFn const m_countryFileGetterFn;
  LocalFileCheckerFn const m_localFileCheckerFn;

  RegionsSetT m_absentRegions;

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
   * This is a cached list of regions, populated by `GetAllRegions()` when there is a router thread
   * and used by subsequent calls to that function until a new router thread is started.
   * Calling `GenerateAbsentRegions()` clears this member.
   */
  RegionsSetT m_regions;
};
}  // namespace routing
