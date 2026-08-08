#include "routing/turns_notification_manager.hpp"

#include "platform/measurement_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <vector>

namespace routing
{
namespace turns
{
namespace sound
{

namespace
{
// If the distance between two sequential turns is less than kSecondTurnThresholdDistM
// the information about the second turn will be shown or pronounced
// when the user is approaching to the first one with "Then,".
double constexpr kSecondTurnThresholdDistM = 400.0;
// If the distance between two sequential turns is less than kDistanceNotifyThresholdM
// the notification will *not* append second distance, (like "In 500 meters. Turn left. Then. Turn right.")
double constexpr kDistanceNotifyThresholdM = 80.0;

// Returns true if the closest turn is an entrance to a roundabout and the second is
// an exit form a roundabout.
// Note. There are some cases when another turn (besides an exit from roundabout)
// follows an entrance to a roundabout. It could happened in case of turns inside a roundabout.
// Returns false otherwise.
bool IsClassicEntranceToRoundabout(routing::turns::TurnItemDist const & firstTurn,
                                   routing::turns::TurnItemDist const & secondTurn)
{
  return firstTurn.m_turnItem.m_turn == routing::turns::CarDirection::EnterRoundAbout &&
         secondTurn.m_turnItem.m_turn == routing::turns::CarDirection::LeaveRoundAbout;
}
}  // namespace

NotificationManager::NotificationManager()
  : m_enabled(false)
  , m_speedMetersPerSecond(0.0)
  , m_settings(8 /* m_startBeforeSecondsVehicle */, 35 /* m_minStartBeforeMetersVehicle */,
               150 /* m_maxStartBeforeMetersVehicle */, 170, /* m_minDistToSayNotificationMeters */
               8 /* m_startBeforeSecondsPedestrian */, 15 /* m_minStartBeforeMetersPedestrian */,
               25 /* m_maxStartBeforeMetersPedestrian */)
  , m_nextTurnNotificationProgress(PronouncedNotification::Nothing)
  , m_turnNotificationWithThen(false)
  , m_nextTurnIndex(0)
  , m_secondTurnNotification(CarDirection::None)
  , m_secondTurnNotificationIndex(0)
{}

// static
NotificationManager NotificationManager::CreateNotificationManagerForTesting(
    uint32_t startBeforeSeconds, uint32_t minStartBeforeMeters, uint32_t maxStartBeforeMeters,
    uint32_t minDistToSayNotificationMeters, measurement_utils::Units lengthUnits, std::string const & engShortJson,
    uint32_t notificationTimeSecond, double speedMeterPerSecond)
{
  NotificationManager notificationManager;
  notificationManager.m_settings =
      Settings(startBeforeSeconds, minStartBeforeMeters, maxStartBeforeMeters, minDistToSayNotificationMeters,
               startBeforeSeconds, minStartBeforeMeters, maxStartBeforeMeters);
  notificationManager.Enable(true);
  notificationManager.SetLengthUnits(lengthUnits);
  notificationManager.m_getTtsText.ForTestingSetLocaleWithJson(engShortJson, "en");
  notificationManager.m_settings.ForTestingSetNotificationTimeSecond(notificationTimeSecond);
  notificationManager.Reset();
  notificationManager.SetSpeedMetersPerSecond(speedMeterPerSecond);

  return notificationManager;
}

std::string NotificationManager::GenerateTurnText(uint32_t distanceUnits, uint8_t exitNum,
                                                  bool useThenInsteadOfDistance, TurnItem const & turn,
                                                  RouteSegment::RoadNameInfo const & nextStreetInfo,
                                                  bool useAtRoundaboutPrefix) const
{
  auto const lengthUnits = m_settings.GetLengthUnits();

  Notification notif{distanceUnits, exitNum, useThenInsteadOfDistance, turn.m_turn, lengthUnits};
  notif.m_useAtRoundaboutPrefix = useAtRoundaboutPrefix;
  if (turn.m_turn == CarDirection::None)
    notif.m_turnDirPedestrian = turn.m_pedestrianTurn;

  // https://github.com/organicmaps/organicmaps/issues/6146
  if (turn.m_turn != CarDirection::EnterRoundAbout)
    notif.m_nextStreetInfo = nextStreetInfo;

  // T-junction prefix replaces the distance in advance notifications only.
  if (turn.m_isEndOfRoad && distanceUnits > 0)
    notif.m_useAtEndOfRoadPrefix = true;

  return m_getTtsText.GetTurnNotification(notif);
}

std::string NotificationManager::GenerateRecalculatingText() const
{
  return m_getTtsText.GetRecalculatingNotification();
}

std::string NotificationManager::GenerateOffRouteText(uint32_t distanceMeters) const
{
  double const distanceUnits = m_settings.ConvertMetersToUnits(distanceMeters);

  return m_getTtsText.GetOffRouteNotification(distanceUnits, m_settings.GetLengthUnits());
}

std::string NotificationManager::GenerateSpeedCameraText() const
{
  return m_getTtsText.GetSpeedCameraNotification();
}

std::string NotificationManager::GenerateRoundaboutNotification(TurnItemDist const & entranceTurn,
                                                                TurnItemDist const & exitTurn,
                                                                RouteSegment::RoadNameInfo const & nextStreetInfo)
{
  if (m_nextTurnIndex != entranceTurn.m_turnItem.m_index)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
    m_nextTurnIndex = entranceTurn.m_turnItem.m_index;
  }

  auto const lengthUnits = m_settings.GetLengthUnits();
  uint32_t const distanceToPronounceNotificationM =
      m_settings.ComputeDistToPronounceDistM(m_speedMetersPerSecond, false /* pedestrian */);

  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
  {
    if (!m_settings.TooCloseForFisrtNotification(entranceTurn.m_distMeters))
    {
      uint32_t const startPronounceDistMeters =
          m_settings.ComputeTurnDistanceM(m_speedMetersPerSecond) + distanceToPronounceNotificationM;
      if (entranceTurn.m_distMeters < startPronounceDistMeters)
      {
        if (m_turnNotificationWithThen)
        {
          FastForwardFirstTurnNotification();
        }
        else
        {
          double const distToPronounceMeters = entranceTurn.m_distMeters - distanceToPronounceNotificationM;
          if (distToPronounceMeters < 0)
          {
            FastForwardFirstTurnNotification();
            return {};
          }

          // First (advance) notification: "In X meters, at the roundabout, take the Nth exit [onto Street]".
          double const distToPronounceUnits = m_settings.ConvertMetersToUnits(distToPronounceMeters);
          uint32_t const roundedDistToPronounceUnits = m_settings.RoundByPresetSoundedDistancesUnits(distToPronounceUnits);
          m_nextTurnNotificationProgress = PronouncedNotification::First;
          // Tell the upcoming LeaveRoundAbout turn (when it becomes the first turn) to skip its
          // own first notification — we've already announced the exit instruction here.
          m_turnNotificationWithThen = true;
          return m_getTtsText.GetTurnNotification(
              {roundedDistToPronounceUnits, static_cast<uint8_t>(exitTurn.m_turnItem.m_exitNum),
               false /* useThenInsteadOfDistance */, CarDirection::LeaveRoundAbout, lengthUnits, nextStreetInfo,
               true /* useAtRoundaboutPrefix */});
        }
      }
    }
    else
    {
      m_nextTurnNotificationProgress = PronouncedNotification::First;
      FastForwardFirstTurnNotification();
    }
    return {};
  }

  if (m_nextTurnNotificationProgress == PronouncedNotification::First &&
      entranceTurn.m_distMeters < distanceToPronounceNotificationM)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Second;
    FastForwardFirstTurnNotification();
    m_turnNotificationWithThen = true;
    // Reminder at entrance: "Take the Nth exit [onto Street]". useThenInsteadOfDistance routes
    // GetRoundaboutTextId to the "take_the_N_exit" key; the "Then" word itself is suppressed for
    // this specific case (LeaveRoundAbout + distance 0 + no roundabout prefix) inside GetTurnNotification.
    return m_getTtsText.GetTurnNotification(
        {0 /* distanceUnits */, static_cast<uint8_t>(exitTurn.m_turnItem.m_exitNum),
         true /* useThenInsteadOfDistance */, CarDirection::LeaveRoundAbout, lengthUnits, nextStreetInfo,
         false /* useAtRoundaboutPrefix */});
  }
  return {};
}

void NotificationManager::GenerateTurnNotifications(std::vector<TurnItemDist> const & turns,
                                                    std::vector<std::string> & turnNotifications)
{
  GenerateTurnNotifications(turns, turnNotifications, RouteSegment::RoadNameInfo{});
}

void NotificationManager::GenerateTurnNotifications(std::vector<TurnItemDist> const & turns,
                                                    std::vector<std::string> & turnNotifications,
                                                    RouteSegment::RoadNameInfo const & nextStreetInfo)
{
  m_secondTurnNotification = GenerateSecondTurnNotification(turns);

  turnNotifications.clear();
  if (!m_enabled || turns.empty())
    return;

  TurnItemDist const & firstTurn = turns.front();

  // Classic roundabout pattern: announce "In X meters, at the roundabout, take the Nth exit
  // [onto Street]" as a single combined notification, instead of separately announcing the
  // entrance ("Enter the roundabout") followed by the exit ("Then take the Nth exit").
  bool const isClassicRoundabout = turns.size() >= 2 && IsClassicEntranceToRoundabout(firstTurn, turns[1]);
  if (isClassicRoundabout)
  {
    TurnItemDist const & exitTurn = turns[1];

    // If a third turn follows shortly after the roundabout exit, skip the exit street name —
    // we'll announce it with the chained third turn instead.
    bool hasCloseThirdTurn = false;
    double distFromExitToThirdTurnM = 0.0;
    if (turns.size() >= 3)
    {
      distFromExitToThirdTurnM = turns[2].m_distMeters - exitTurn.m_distMeters;
      hasCloseThirdTurn = distFromExitToThirdTurnM <= kSecondTurnThresholdDistM;
    }
    RouteSegment::RoadNameInfo const & roundaboutStreetInfo =
        hasCloseThirdTurn ? RouteSegment::RoadNameInfo{} : nextStreetInfo;

    std::string notification = GenerateRoundaboutNotification(firstTurn, exitTurn, roundaboutStreetInfo);
    if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
      return;
    bool const hasNotification = !notification.empty();
    if (hasNotification)
      turnNotifications.emplace_back(std::move(notification));

    if (hasCloseThirdTurn && hasNotification)
    {
      TurnItemDist const & thirdTurn = turns[2];
      bool const isThirdTurnRoundaboutEntrance =
          turns.size() >= 4 && IsClassicEntranceToRoundabout(thirdTurn, turns[3]);

      std::string thirdNotification;
      if (isThirdTurnRoundaboutEntrance)
      {
        // Second roundabout in the chain: "Then at the roundabout, take the Xth exit."
        // The second roundabout's exit street name is not threaded through this layer.
        thirdNotification = m_getTtsText.GetTurnNotification(
            {0 /* distanceUnits */, static_cast<uint8_t>(turns[3].m_turnItem.m_exitNum),
             true /* useThenInsteadOfDistance */, CarDirection::LeaveRoundAbout, m_settings.GetLengthUnits(),
             RouteSegment::RoadNameInfo{}, true /* useAtRoundaboutPrefix */});
      }
      else
      {
        double const distForNotification =
            distFromExitToThirdTurnM < kDistanceNotifyThresholdM ? 0 : distFromExitToThirdTurnM;
        thirdNotification =
            GenerateTurnText(m_settings.ConvertMetersToUnits(distForNotification), thirdTurn.m_turnItem.m_exitNum,
                             true /* useThenInsteadOfDistance */, thirdTurn.m_turnItem, nextStreetInfo);
      }

      if (!thirdNotification.empty())
      {
        // Concatenate to avoid a TTS pause between the two instructions.
        if (!turnNotifications.empty())
          turnNotifications.back() += " " + thirdNotification;
        else
          turnNotifications.emplace_back(std::move(thirdNotification));
      }
      m_turnNotificationWithThen = true;
    }
    return;
  }

  // LeaveRoundAbout as the nearest turn — we're inside the roundabout, past the entrance.
  // Within ~60m of the exit, suppress: GPS+TTS latency would deliver the announcement after the
  // user has already exited. Outside that range, fall through to the normal distance-based path
  // (large multi-exit roundabouts need that to time the notification correctly).
  if (firstTurn.m_turnItem.m_turn == CarDirection::LeaveRoundAbout)
  {
    static constexpr double kSmallRoundaboutExitDistM = 60.0;
    if (m_nextTurnIndex != firstTurn.m_turnItem.m_index)
    {
      m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
      m_nextTurnIndex = firstTurn.m_turnItem.m_index;
    }
    if (firstTurn.m_distMeters < kSmallRoundaboutExitDistM)
    {
      m_nextTurnNotificationProgress = PronouncedNotification::Second;
      FastForwardFirstTurnNotification();
      return;
    }
  }

  std::string firstNotification = GenerateFirstTurnSound(firstTurn.m_turnItem, firstTurn.m_distMeters, nextStreetInfo);
  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
    return;
  if (firstNotification.empty())
    return;
  turnNotifications.emplace_back(std::move(firstNotification));

  // Generating notifications like "Then turn left" if necessary.
  if (turns.size() < 2)
    return;
  TurnItemDist const & secondTurn = turns[1];
  ASSERT_LESS_OR_EQUAL(firstTurn.m_distMeters, secondTurn.m_distMeters, ());

  double distBetweenTurnsMeters = secondTurn.m_distMeters - firstTurn.m_distMeters;
  ASSERT_GREATER_OR_EQUAL(distBetweenTurnsMeters, 0, ());
  if (distBetweenTurnsMeters > kSecondTurnThresholdDistM)
    return;

  if (distBetweenTurnsMeters < kDistanceNotifyThresholdM)
  {
    // Don't pronounce distance because of immediate "Then".
    distBetweenTurnsMeters = 0;
  }

  // If the second turn is itself a roundabout entrance with an exit, chain it as
  // "Then at the roundabout, take the Xth exit" instead of "Then enter the roundabout".
  bool const isSecondTurnRoundaboutEntrance =
      turns.size() >= 3 && IsClassicEntranceToRoundabout(secondTurn, turns[2]);

  std::string secondNotification;
  if (isSecondTurnRoundaboutEntrance)
  {
    TurnItemDist const & secondRoundaboutExit = turns[2];
    secondNotification = m_getTtsText.GetTurnNotification(
        {0 /* distanceUnits */, static_cast<uint8_t>(secondRoundaboutExit.m_turnItem.m_exitNum),
         true /* useThenInsteadOfDistance */, CarDirection::LeaveRoundAbout, m_settings.GetLengthUnits(),
         RouteSegment::RoadNameInfo{}, true /* useAtRoundaboutPrefix */});
  }
  else
  {
    secondNotification =
        GenerateTurnText(m_settings.ConvertMetersToUnits(distBetweenTurnsMeters), secondTurn.m_turnItem.m_exitNum,
                         true /* useThenInsteadOfDistance */, secondTurn.m_turnItem, RouteSegment::RoadNameInfo{});
  }
  if (secondNotification.empty())
    return;

  // Concatenate to avoid a TTS pause between the two instructions.
  if (!turnNotifications.empty())
    turnNotifications.back() += " " + secondNotification;
  else
    turnNotifications.emplace_back(std::move(secondNotification));

  // Log turn notifications TTS
  if (!turnNotifications.empty())
    for (auto const & notification : turnNotifications)
      LOG(LINFO, ("TTS:", notification));

  // Turn notification with word "Then" (about the second turn) will be pronounced.
  // When this second turn become the first one the first notification about the turn
  // shall be skipped.
  m_turnNotificationWithThen = true;
}

std::string NotificationManager::GenerateFirstTurnSound(TurnItem const & turn, double distanceToTurnMeters,
                                                        RouteSegment::RoadNameInfo const & nextStreetInfo)
{
  if (m_nextTurnIndex != turn.m_index)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
    m_nextTurnIndex = turn.m_index;
  }

  bool const pedestrian = turn.m_pedestrianTurn != PedestrianDirection::None;

  uint32_t const distanceToPronounceNotificationM =
      m_settings.ComputeDistToPronounceDistM(m_speedMetersPerSecond, pedestrian);

  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
  {
    if (!m_settings.TooCloseForFisrtNotification(distanceToTurnMeters))
    {
      uint32_t const startPronounceDistMeters =
          m_settings.ComputeTurnDistanceM(m_speedMetersPerSecond) + distanceToPronounceNotificationM;
      if (distanceToTurnMeters < startPronounceDistMeters)
      {
        if (m_turnNotificationWithThen)
        {
          FastForwardFirstTurnNotification();
        }
        else
        {
          double const distToPronounceMeters = distanceToTurnMeters - distanceToPronounceNotificationM;
          if (distToPronounceMeters < 0)
          {
            FastForwardFirstTurnNotification();
            return {};  // The current position is too close to the turn for the first notification.
          }

          // Pronouncing first turn sound notification.
          double const distToPronounceUnits = m_settings.ConvertMetersToUnits(distToPronounceMeters);
          uint32_t const roundedDistToPronounceUnits =
              m_settings.RoundByPresetSoundedDistancesUnits(distToPronounceUnits);
          m_nextTurnNotificationProgress = PronouncedNotification::First;

          LOG(LINFO,
              ("TTS meters to pronounce", distanceToPronounceNotificationM, "meters to turn", distanceToTurnMeters,
               "meters to start pronounce", startPronounceDistMeters, "speed m/s", m_speedMetersPerSecond));
          return GenerateTurnText(roundedDistToPronounceUnits, turn.m_exitNum, false /* useThenInsteadOfDistance */,
                                  turn, nextStreetInfo);
        }
      }
    }
    else
    {
      // The first notification has not been pronounced but the distance to the turn is too short.
      // It happens if one turn follows shortly behind another one.
      m_nextTurnNotificationProgress = PronouncedNotification::First;
      FastForwardFirstTurnNotification();
    }
    return {};
  }

  if (m_nextTurnNotificationProgress == PronouncedNotification::First &&
      distanceToTurnMeters < distanceToPronounceNotificationM)
  {
    m_nextTurnNotificationProgress = PronouncedNotification::Second;
    FastForwardFirstTurnNotification();

    LOG(LINFO, ("TTS meters to pronounce", distanceToPronounceNotificationM, "meters to turn", distanceToTurnMeters,
                "speed m/s", m_speedMetersPerSecond));
    return GenerateTurnText(0 /* distMeters */, turn.m_exitNum, false /* useThenInsteadOfDistance */, turn,
                            nextStreetInfo);
  }
  return {};
}

void NotificationManager::Enable(bool enable)
{
  if (enable && !m_enabled)
    Reset();
  m_enabled = enable;
}

void NotificationManager::SetLengthUnits(measurement_utils::Units units)
{
  m_settings.SetLengthUnits(units);
  switch (units)
  {
  case measurement_utils::Units::Metric:
    m_settings.SetState(30 /* notificationTimeSeconds */, 200 /* minNotificationDistanceUnits */,
                        2000 /* maxNotificationDistanceUnits */, GetSoundedDistMeters() /* soundedDistancesUnits */,
                        measurement_utils::Units::Metric /* lengthUnits */);
    return;
  case measurement_utils::Units::Imperial:
    m_settings.SetState(30 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */,
                        5000 /* maxNotificationDistanceUnits */, GetSoundedDistFeet() /* soundedDistancesUnits */,
                        measurement_utils::Units::Imperial /* lengthUnits */);
    return;
  }
}

void NotificationManager::SetSpeedMetersPerSecond(double speed)
{
  // When the quality of GPS data is bad the current speed may be less then zero.
  // It's easy to reproduce at an office with Nexus 5.
  // In that case zero meters per second is used.
  m_speedMetersPerSecond = std::max(0., speed);
}

void NotificationManager::Reset()
{
  m_nextTurnNotificationProgress = PronouncedNotification::Nothing;
  m_nextTurnIndex = 0;
  m_turnNotificationWithThen = false;
  m_secondTurnNotification = CarDirection::None;
  m_secondTurnNotificationIndex = 0;
}

void NotificationManager::FastForwardFirstTurnNotification()
{
  m_turnNotificationWithThen = false;
  if (m_nextTurnNotificationProgress == PronouncedNotification::Nothing)
    m_nextTurnNotificationProgress = PronouncedNotification::First;
}

CarDirection NotificationManager::GenerateSecondTurnNotification(std::vector<TurnItemDist> const & turns)
{
  if (turns.size() < 2)
  {
    m_secondTurnNotificationIndex = 0;
    return CarDirection::None;
  }

  TurnItemDist const & firstTurn = turns[0];
  TurnItemDist const & secondTurn = turns[1];

  if (firstTurn.m_turnItem.m_index != m_secondTurnNotificationIndex)
    m_secondTurnNotificationIndex = 0;  // It's a new closest (first) turn.
  else if (m_secondTurnNotificationIndex != 0)
    return secondTurn.m_turnItem.m_turn;  // m_secondTurnNotificationIndex was set to true before.

  double const distBetweenTurnsMeters = secondTurn.m_distMeters - firstTurn.m_distMeters;
  ASSERT_LESS_OR_EQUAL(0., distBetweenTurnsMeters, ());

  if (distBetweenTurnsMeters > kSecondTurnThresholdDistM)
    return CarDirection::None;

  uint32_t const startPronounceDistMeters = m_settings.ComputeTurnDistanceM(m_speedMetersPerSecond) +
                                            m_settings.ComputeDistToPronounceDistM(m_speedMetersPerSecond);

  if (firstTurn.m_distMeters <= startPronounceDistMeters)
  {
    m_secondTurnNotificationIndex = firstTurn.m_turnItem.m_index;
    return secondTurn.m_turnItem.m_turn;  // It's time to inform about the turn after the next one.
  }

  return CarDirection::None;
}

void NotificationManager::SetLocaleWithJsonForTesting(std::string const & json, std::string const & locale)
{
  m_getTtsText.ForTestingSetLocaleWithJson(json, locale);
}

std::string DebugPrint(PronouncedNotification notificationProgress)
{
  switch (notificationProgress)
  {
  case PronouncedNotification::Nothing: return "Nothing";
  case PronouncedNotification::First: return "First";
  case PronouncedNotification::Second: return "Second";
  }

  ASSERT(false, ());
  std::stringstream out;
  out << "unknown PronouncedNotification (" << static_cast<int>(notificationProgress) << ")";
  return out.str();
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
