#include "testing/testing.hpp"

#include "routing/routing_tests/tools.hpp"

#include "routing/absent_regions_finder.hpp"
#include "routing/async_router.hpp"
#include "routing/router.hpp"
#include "routing/routing_callbacks.hpp"

#include "geometry/point2d.hpp"

#include "platform/platform.hpp"

#include "base/timer.hpp"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace async_router_test
{
using namespace routing;
using namespace std::placeholders;
using namespace std;

class DummyRouter : public IRouter
{
  RouterResultCode m_result;
  set<string> m_absent;

public:
  DummyRouter(RouterResultCode code, set<string> const & absent) : m_result(code), m_absent(absent) {}

  // IRouter overrides:
  string GetName() const override { return "Dummy"; }
  void SetGuides(GuidesTracks && /* guides */) override {}
  RouterResultCode CalculateRoute(Checkpoints const & checkpoints, m2::PointD const & startDirection,
                                  bool adjustToPrevRoute, RouterDelegate const & delegate, Route & route) override
  {
    auto const routeId = route.GetRouteId();
    route = Route("dummy", checkpoints.GetPoints().cbegin(), checkpoints.GetPoints().cend(), routeId);

    for (auto const & absent : m_absent)
      route.AddAbsentCountry(absent);

    return m_result;
  }

  bool FindClosestProjectionToRoad(m2::PointD const & point, m2::PointD const & direction, double radius,
                                   EdgeProj & proj) override
  {
    return false;
  }
};

struct DummyRoutingCallbacks
{
  vector<RouterResultCode> m_codes;
  vector<set<string>> m_absent;
  vector<uint64_t> m_routeIds;
  condition_variable m_cv;
  mutex m_lock;
  uint32_t const m_expected;
  uint32_t m_called;

  explicit DummyRoutingCallbacks(uint32_t expectedCalls) : m_expected(expectedCalls), m_called(0) {}

  // ReadyCallbackOwnership callback
  void operator()(shared_ptr<Route> route, RouterResultCode code)
  {
    CHECK(route, ());
    m_codes.push_back(code);
    m_absent.emplace_back(route->GetAbsentCountries());
    m_routeIds.push_back(route->GetRouteId());
    TestAndNotifyReadyCallbacks();
  }

  // NeedMoreMapsCallback callback
  void operator()(uint64_t routeId, set<string> const & absent)
  {
    m_codes.push_back(RouterResultCode::NeedMoreMaps);
    m_absent.emplace_back(absent);
    TestAndNotifyReadyCallbacks();
  }

  void WaitFinish()
  {
    unique_lock<mutex> lk(m_lock);
    return m_cv.wait(lk, [this] { return m_called == m_expected; });
  }

  void TestAndNotifyReadyCallbacks()
  {
    {
      lock_guard<mutex> calledLock(m_lock);
      ++m_called;
      TEST_LESS_OR_EQUAL(m_called, m_expected, ("The result callback called more times than expected."));
    }
    m_cv.notify_all();
  }
};

UNIT_CLASS_TEST(AsyncGuiThreadTest, RouteIdsAdvanceForEachBuild)
{
  auto router = make_unique<DummyRouter>(RouterResultCode::NoError, set<string>{});
  AsyncRouter async(nullptr /* pointCheckCallback */);
  async.SetRouter(std::move(router), nullptr /* absentRegionsFinder */);

  auto const checkpoints = Checkpoints({1, 2} /* start */, {5, 6} /* finish */);
  auto const calculate = [&async, &checkpoints](DummyRoutingCallbacks & callbacks)
  {
    async.CalculateRoute(checkpoints, {3, 4} /* direction */, false /* adjustToPrevRoute */,
                         bind(ref(callbacks), _1, _2), nullptr /* needMoreMapsCallback */,
                         nullptr /* removeRouteCallback */, nullptr /* progressCallback */);
    callbacks.WaitFinish();
  };

  DummyRoutingCallbacks firstBuild(1 /* expectedCalls */);
  calculate(firstBuild);
  DummyRoutingCallbacks secondBuild(1 /* expectedCalls */);
  calculate(secondBuild);

  TEST_EQUAL(firstBuild.m_routeIds.size(), 1, ());
  TEST_EQUAL(secondBuild.m_routeIds.size(), 1, ());
  TEST_NOT_EQUAL(firstBuild.m_routeIds.front(), 0, ());
  TEST_GREATER(secondBuild.m_routeIds.front(), firstBuild.m_routeIds.front(), ());
}

// TODO(o.khlopkova) Uncomment and update these tests.

// UNIT_CLASS_TEST(AsyncGuiThreadTest, NeedMoreMapsSignalTest)
//{
//  set<string> const absentData({"test1", "test2"});
//  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher(absentData));
//  unique_ptr<IRouter> router(new DummyRouter(RouterResultCode::NoError, {}));
//  DummyRoutingCallbacks resultCallback(2 /* expectedCalls */);
//  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
//  async.SetRouter(std::move(router), std::move(fetcher));
//  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
//                       bind(ref(resultCallback), _1, _2) /* readyCallback */,
//                       bind(ref(resultCallback), _1, _2) /* needMoreMapsCallback */,
//                       nullptr /* removeRouteCallback */, nullptr /* progressCallback */);
//
//  resultCallback.WaitFinish();
//
//  TEST_EQUAL(resultCallback.m_codes.size(), 2, ());
//  TEST_EQUAL(resultCallback.m_codes[0], RouterResultCode::NoError, ());
//  TEST_EQUAL(resultCallback.m_codes[1], RouterResultCode::NeedMoreMaps, ());
//  TEST_EQUAL(resultCallback.m_absent.size(), 2, ());
//  TEST(resultCallback.m_absent[0].empty(), ());
//  TEST_EQUAL(resultCallback.m_absent[1].size(), 2, ());
//  TEST_EQUAL(resultCallback.m_absent[1], absentData, ());
//}

// UNIT_CLASS_TEST(AsyncGuiThreadTest, StandardAsyncFogTest)
//{
//  unique_ptr<IOnlineFetcher> fetcher(new DummyFetcher({}));
//  unique_ptr<IRouter> router(new DummyRouter(RouterResultCode::NoError, {}));
//  DummyRoutingCallbacks resultCallback(1 /* expectedCalls */);
//  AsyncRouter async(DummyStatisticsCallback, nullptr /* pointCheckCallback */);
//  async.SetRouter(std::move(router), std::move(fetcher));
//  async.CalculateRoute(Checkpoints({1, 2} /* start */, {5, 6} /* finish */), {3, 4}, false,
//                       bind(ref(resultCallback), _1, _2), nullptr /* needMoreMapsCallback */,
//                       nullptr /* progressCallback */, nullptr /* removeRouteCallback */);
//
//  resultCallback.WaitFinish();
//
//  TEST_EQUAL(resultCallback.m_codes.size(), 1, ());
//  TEST_EQUAL(resultCallback.m_codes[0], RouterResultCode::NoError, ());
//  TEST_EQUAL(resultCallback.m_absent.size(), 1, ());
//  TEST(resultCallback.m_absent[0].empty(), ());
//}
}  //  namespace async_router_test
