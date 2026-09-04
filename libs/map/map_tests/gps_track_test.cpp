#include "testing/testing.hpp"

#include "map/gps_track.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/latlon.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include <chrono>
#include <functional>
#include <future>
#include <utility>
#include <vector>

namespace gps_track_test
{
using namespace std;
using namespace std::chrono;

inline location::GpsInfo Make(double timestamp, ms::LatLon const & ll, double speed)
{
  location::GpsInfo info;
  info.m_timestamp = timestamp;
  info.m_speed = speed;
  info.m_latitude = ll.m_lat;
  info.m_longitude = ll.m_lon;
  info.m_horizontalAccuracy = 15;
  info.m_source = location::EAndroidNative;
  return info;
}

inline string GetGpsTrackFilePath()
{
  return base::JoinPath(GetPlatform().WritableDir(), "gpstrack_test.bin");
}

class GpsTrackCallback
{
public:
  GpsTrackCallback() : m_toRemove(make_pair(GpsTrack::kInvalidId, GpsTrack::kInvalidId)), m_gotCallback(false) {}
  void OnUpdate(vector<pair<size_t, location::GpsInfo>> && toAdd, pair<size_t, size_t> const & toRemove)
  {
    m_toAdd = std::move(toAdd);
    m_toRemove = toRemove;

    lock_guard<mutex> lg(m_mutex);
    m_gotCallback = true;
    m_cv.notify_one();
  }
  void Reset()
  {
    m_toAdd.clear();
    m_toRemove = make_pair(GpsTrack::kInvalidId, GpsTrack::kInvalidId);

    lock_guard<mutex> lg(m_mutex);
    m_gotCallback = false;
  }
  bool WaitForCallback(seconds t)
  {
    unique_lock<mutex> ul(m_mutex);
    return m_cv.wait_for(ul, t, [this]() -> bool { return m_gotCallback; });
  }

  vector<pair<size_t, location::GpsInfo>> m_toAdd;
  pair<size_t, size_t> m_toRemove;

private:
  mutex m_mutex;
  condition_variable m_cv;
  bool m_gotCallback;
};

seconds const kWaitForCallbackTimeout = seconds(5);

UNIT_TEST(GpsTrack_ReadBeforeInitialization)
{
  GpsTrack track(GetGpsTrackFilePath());
  TEST_EQUAL(track.GetElevationInfo().GetSize(), 0, ());
  TEST_EQUAL(track.GetTrackStatistics().m_length, 0.0, ());
  TEST_EQUAL(track.GetSize(), 0, ());
  TEST(track.IsEmpty(), ());
  track.ForEachPoint([](location::GpsInfo const &, size_t)
  {
    TEST(false, ("An uninitialized track must be empty"));
    return true;
  });
}

UNIT_TEST(GpsTrack_ElevationSnapshot)
{
  string const filePath = GetGpsTrackFilePath();
  FileWriter::DeleteFileX(filePath);
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));

  // Keep callback state alive until the track's worker has joined.
  promise<size_t> recorded;
  promise<size_t> cleared;
  auto recordedResult = recorded.get_future();
  auto clearedResult = cleared.get_future();
  GpsTrack track(filePath);
  track.SetCallback([&](auto && toAdd, auto const &, TrackStatistics const &)
  {
    // Callbacks must be able to read snapshots without deadlocking.
    auto const size = track.GetElevationInfo().GetSize();
    if (toAdd.empty())
      cleared.set_value(size);
    else
      recorded.set_value(size);
  });

  auto first = Make(1, ms::LatLon(0, 0), 1);
  first.m_altitude = 10;
  auto second = Make(2, ms::LatLon(0, 0.001), 1);
  second.m_altitude = 20;
  track.AddPoints({first, second});
  TEST(recordedResult.wait_for(kWaitForCallbackTimeout) == future_status::ready, ());
  TEST_EQUAL(recordedResult.get(), 2, ());

  auto const & snapshot = track.GetElevationInfo();
  TEST_EQUAL(snapshot.GetSize(), 2, ());
  size_t visited = 0;
  track.ForEachPoint([&](location::GpsInfo const & point, size_t id)
  {
    TEST_EQUAL(track.GetSize(), 2, ("Point callbacks must run without holding the collection lock"));
    TEST_EQUAL(id, 0, ());
    TEST_EQUAL(point.m_altitude, 10, ());
    ++visited;
    return false;
  });
  TEST_EQUAL(visited, 1, ());
  track.Clear();
  TEST(clearedResult.wait_for(kWaitForCallbackTimeout) == future_status::ready, ());
  TEST_EQUAL(clearedResult.get(), 0, ());
  TEST_EQUAL(track.GetElevationInfo().GetSize(), 0, ());
  TEST_EQUAL(snapshot.GetSize(), 2, ("Clearing the worker collection must not change an existing snapshot"));
  TEST_EQUAL(snapshot.GetPoints()[1].m_point.GetAltitude(), 20, ());
}

UNIT_TEST(GpsTrack_Simple)
{
  string const filePath = GetGpsTrackFilePath();
  SCOPE_GUARD(gpsTestFileDeleter, bind(FileWriter::DeleteFileX, filePath));
  FileWriter::DeleteFileX(filePath);

  time_t const t = system_clock::to_time_t(system_clock::now());
  double const timestamp = t;
  LOG(LINFO, ("Timestamp", ctime(&t), timestamp));

  size_t const writeItemCount = 50000;

  vector<location::GpsInfo> points;
  points.reserve(writeItemCount);
  for (size_t i = 0; i < writeItemCount; ++i)
    points.emplace_back(Make(timestamp + i, ms::LatLon(-90.0 + i, -180.0 + i), 10 + i));

  // Store points
  {
    GpsTrack track(filePath);

    track.AddPoints(points);

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speed, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }

  // Restore points
  {
    GpsTrack track(filePath);

    GpsTrackCallback callback;

    track.SetCallback(bind(&GpsTrackCallback::OnUpdate, &callback, placeholders::_1, placeholders::_2));

    TEST(callback.WaitForCallback(kWaitForCallbackTimeout), ());

    TEST_EQUAL(callback.m_toRemove.first, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toRemove.second, GpsTrack::kInvalidId, ());
    TEST_EQUAL(callback.m_toAdd.size(), writeItemCount, ());
    for (size_t i = 0; i < writeItemCount; ++i)
    {
      TEST_EQUAL(i, callback.m_toAdd[i].first, ());
      TEST_EQUAL(points[i].m_timestamp, callback.m_toAdd[i].second.m_timestamp, ());
      TEST_EQUAL(points[i].m_speed, callback.m_toAdd[i].second.m_speed, ());
      TEST_EQUAL(points[i].m_latitude, callback.m_toAdd[i].second.m_latitude, ());
      TEST_EQUAL(points[i].m_longitude, callback.m_toAdd[i].second.m_longitude, ());
    }
  }
}
}  // namespace gps_track_test
