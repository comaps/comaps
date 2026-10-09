#include "testing/testing.hpp"

#include "routing/turns_notification_manager.hpp"
#include "routing/turns_sound_settings.hpp"

#include <vector>

namespace turns_sound_test
{
using namespace std;
using routing::RouteSegment;
using routing::turns::CarDirection;
using routing::turns::TurnItemDist;
using routing::turns::sound::NotificationManager;
using routing::turns::sound::Settings;

// An error to compare two double after conversion feet to meters.
double const kEps = 1.;

UNIT_TEST(TurnNotificationSettings_MetersTest)
{
  Settings const settings(
      20 /* notificationTimeSeconds */, 200 /* minNotificationDistanceUnits */, 700 /* maxNotificationDistanceUnits */,
      5 /* m_startBeforeSeconds */, 25 /* m_minStartBeforeMeters */, 150 /* m_maxStartBeforeMeters */,
      170 /* m_minDistToSayNotificationMeters */, {100, 200, 300, 400, 500, 600, 700} /* soundedDistancesUnits */,
      measurement_utils::Units::Metric /* lengthUnits */);

  TEST(settings.IsValid(), ());
  TEST(AlmostEqualAbs(settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 20., kEps), ());
  TEST(AlmostEqualAbs(settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 300., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(300 /* distanceInUnits */), 300, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 100, ());

  TEST_EQUAL(settings.ComputeTurnDistanceM(0. /* speedMetersPerSecond */), 200., ());
  TEST_EQUAL(settings.ComputeTurnDistanceM(10. /* speedMetersPerSecond */), 200., ());
  TEST_EQUAL(settings.ComputeTurnDistanceM(20. /* speedMetersPerSecond */), 400., ());
  TEST_EQUAL(settings.ComputeTurnDistanceM(35. /* speedMetersPerSecond */), 700., ());
  TEST_EQUAL(settings.ComputeTurnDistanceM(200. /* speedMetersPerSecond */), 700., ());

  TEST_EQUAL(settings.ComputeDistToPronounceDistM(0. /* speedMetersPerSecond */), 25., ());
  TEST_EQUAL(settings.ComputeDistToPronounceDistM(10. /* speedMetersPerSecond */), 50., ());
  TEST_EQUAL(settings.ComputeDistToPronounceDistM(20. /* speedMetersPerSecond */), 100., ());
  TEST_EQUAL(settings.ComputeDistToPronounceDistM(35. /* speedMetersPerSecond */), 150., ());
  TEST_EQUAL(settings.ComputeDistToPronounceDistM(200. /* speedMetersPerSecond */), 150., ());
}

UNIT_TEST(TurnNotificationSettings_FeetTest)
{
  Settings const settings(
      20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */, 2000 /* maxNotificationDistanceUnits */,
      5 /* m_startBeforeSeconds */, 25 /* m_minStartBeforeMeters */, 150 /* m_maxStartBeforeMeters */,
      170 /* m_minDistToSayNotificationMeters */, {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
      measurement_utils::Units::Imperial /* lengthUnits */);

  TEST(settings.IsValid(), ());
  TEST(AlmostEqualAbs(settings.ConvertMetersPerSecondToUnitsPerSecond(20.), 65., kEps), ());
  TEST(AlmostEqualAbs(settings.ConvertMetersPerSecondToUnitsPerSecond(0.), 0., kEps), ());
  TEST(AlmostEqualAbs(settings.ConvertUnitsToMeters(300. /* distanceInUnits */), 91., kEps), ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(500 /* distanceInUnits */), 600, ());
  TEST_EQUAL(settings.RoundByPresetSoundedDistancesUnits(0 /* distanceInUnits */), 200, ());
}

UNIT_TEST(TurnNotificationSettings_NotValidTest)
{
  Settings settings1(
      20 /* notificationTimeSeconds */, 500 /* minNotificationDistanceUnits */, 2000 /* maxNotificationDistanceUnits */,
      5 /* m_startBeforeSeconds */, 25 /* m_minStartBeforeMeters */, 150 /* m_maxStartBeforeMeters */,
      170 /* m_minDistToSayNotificationMeters */, {200, 400, 800, 600, 1000, 1500, 2000} /* soundedDistancesUnits */,
      measurement_utils::Units::Imperial /* lengthUnits */);
  TEST(!settings1.IsValid(), ());

  Settings settings2(20 /* notificationTimeSeconds */, 5000 /* minNotificationDistanceUnits */,
                     2000 /* maxNotificationDistanceUnits */, 5 /* m_startBeforeSeconds */,
                     25 /* m_minStartBeforeMeters */, 150 /* m_maxStartBeforeMeters */,
                     170 /* m_minDistToSayNotificationMeters */,
                     {200, 400, 600, 800, 1000, 1500, 2000} /* soundedDistancesUnits */,
                     measurement_utils::Units::Metric /* lengthUnits */);
  TEST(!settings2.IsValid(), ());
}

UNIT_TEST(TurnsSound_NotificationDistances)
{
  NotificationManager const unconfigured;
  TEST_EQUAL(unconfigured.GetFirstNotificationDistM(), 0, ());
  TEST_EQUAL(unconfigured.GetSecondNotificationDistM(), 0, ());

  auto slow = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, "{}",
      20 /* notificationTimeSecond */, 10.0 /* speedMeterPerSecond */);
  TEST_EQUAL(slow.GetSecondNotificationDistM(), 50, ());
  TEST_EQUAL(slow.GetFirstNotificationDistM(), 250, ());

  auto fast = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, "{}",
      20 /* notificationTimeSecond */, 30.0 /* speedMeterPerSecond */);
  TEST_EQUAL(fast.GetSecondNotificationDistM(), 100, ());
  TEST_EQUAL(fast.GetFirstNotificationDistM(), 700, ());
}

UNIT_TEST(TurnsSound_MetersTest)
{
  string const engShortJson =
      "\
      {\
      \"in_600_meters\":\"In 600 meters.\",\
      \"make_a_right_turn\":\"Make a right turn.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      20 /* notificationTimeSecond */, 30.0 /* speedMeterPerSecond */);

  vector<TurnItemDist> turns = {{{5 /* idx */, CarDirection::TurnRight}, 1000.}};
  vector<string> turnNotifications;

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 700 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 700.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turns.front().m_distMeters = 699.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 600 meters. Make a right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 650 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 650.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  notificationManager.SetSpeedMetersPerSecond(32.);

  // 150 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 150.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 100 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 100.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turns.front().m_distMeters = 99.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification2 = {{"Make a right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 99 meters till the turn again. No sound notifications is required.
  turns.front().m_distMeters = 99.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 50 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 50.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 0 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 0.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  TEST(notificationManager.IsEnabled(), ());
}

// Test case:
// - Two turns;
// - They are close to each other;
// So the first notification of the second turn shall be skipped.
UNIT_TEST(TurnsSound_MetersTwoTurnsTest)
{
  string const engShortJson =
      "\
      {\
      \"in_600_meters\":\"In 600 meters.\",\
      \"make_a_sharp_right_turn\":\"Make a sharp right turn.\",\
      \"enter_the_roundabout\":\"At the roundabout.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      20 /* notificationTimeSecond */, 35.0 /* speedMeterPerSecond */);

  vector<TurnItemDist> turns = {{{5 /* idx */, CarDirection::TurnSharpRight}, 800.}};
  vector<string> turnNotifications;

  // Starting nearing the first turn.
  // 800 meters till the turn. No sound notifications is required.
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 700 meters till the turn. It's time to pronounce the first voice notification.
  // The speed is high.
  // The compensation of
  // NotificationManager::m_startBeforeSeconds/NotificationManager::m_minStartBeforeMeters/
  // NotificationManager::m_maxStartBeforeMeters is not enough.
  // The user will be closer to the turn while pronouncing despite the compensation.
  // So it should be pronounced "In 600 meters."
  turns.front().m_distMeters = 700.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 600 meters. Make a sharp right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  notificationManager.SetSpeedMetersPerSecond(32.);

  // 150 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 150.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turns.front().m_distMeters = 99.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification2 = {{"Make a sharp right turn."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  notificationManager.SetSpeedMetersPerSecond(10.);

  // 0 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 0.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  vector<TurnItemDist> turns2 = {{{11 /* idx */, CarDirection::EnterRoundAbout, 2 /* exitNum */}, 60.}};

  // Starting nearing the second turn.
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 40 meters till the second turn. It's time to pronounce the second voice notification
  // without the first one.
  turns2.front().m_distMeters = 40.;
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  vector<string> const expectedNotification3 = {{"At the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification3, ());

  TEST(notificationManager.IsEnabled(), ());
}

UNIT_TEST(TurnsSound_FeetTest)
{
  string const engShortJson =
      "\
      {\
      \"in_2000_feet\":\"In 2000 feet.\",\
      \"enter_the_roundabout\":\"At the roundabout.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Imperial, engShortJson,
      20 /* notificationTimeSecond */, 30.0 /* speedMeterPerSecond */);

  vector<TurnItemDist> turns = {{{7 /* idx */, CarDirection::EnterRoundAbout, 3 /* exitNum */}, 1000.}};
  vector<string> turnNotifications;

  // Starting nearing the turnItem.
  // 1000 meters till the turn. No sound notifications is required.
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 700 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 700.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 699 meters till the turn. It's time to pronounce the first voice notification.
  // Why? The current speed is 30 meters per seconds. According to correctSettingsMeters
  // we need to play the first voice notification 20 seconds before the turn.
  // Besides that we need 5 seconds (but 100 meters maximum) for playing the notification.
  // So we start playing the first notification when the distance till the turn is less
  // then 20 seconds * 30 meters per seconds + 100 meters = 700 meters.
  turns.front().m_distMeters = 699.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification1 = {{"In 2000 feet. At the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // 650 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 650.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 150 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 150.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 100 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 100.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 99 meters till the turn. It's time to pronounce the second voice notification.
  turns.front().m_distMeters = 99.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  vector<string> const expectedNotification2 = {{"At the roundabout."}};
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  // 99 meters till the turn again. No sound notifications is required.
  turns.front().m_distMeters = 99.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 50 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 50.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // 0 meters till the turn. No sound notifications is required.
  turns.front().m_distMeters = 0.;
  notificationManager.GenerateTurnNotifications(turns, turnNotifications);
  TEST(turnNotifications.empty(), ());

  TEST(notificationManager.IsEnabled(), ());
}

UNIT_TEST(TurnsSound_ComposedTurnTest)
{
  string const engShortJson =
      "\
      {\
      \"in_600_meters\":\"In 600 meters.\",\
      \"in_200_meters\":\"In 200 meters.\",\
      \"make_a_right_turn\":\"Turn right.\",\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"then\":\"Then.\",\
      \"you_have_reached_the_destination\":\"You have reached the destination.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30.0 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;

  // Starting nearing the first turn.
  // 800 meters till the first turn.
  vector<TurnItemDist> const turns1 = {{{5 /* idx */, CarDirection::TurnRight}, 800. /* m_distMeters */},
                                       {{10 /* idx */, CarDirection::EnterRoundAbout}, 1000. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 620 meters till the first turn, roundabout 45m further with exit 2.
  turnNotifications.clear();
  vector<TurnItemDist> const turns2 = {
      {{5 /* idx */, CarDirection::TurnRight}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 665. /* m_distMeters */},
      {{12 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 720. /* m_distMeters */}};
  vector<string> const expectedNotification2 = {{"In 600 meters. Turn right. Then. At the roundabout. Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification2, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::EnterRoundAbout, ());

  // 300 meters till the first turn.
  turnNotifications.clear();
  vector<TurnItemDist> const turns3 = {{{5 /* idx */, CarDirection::TurnRight}, 300. /* m_distMeters */},
                                       {{10 /* idx */, CarDirection::EnterRoundAbout}, 500. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::EnterRoundAbout, ());

  // 20 meters till the first turn. Route::GetNextTurns() looks past the roundabout entrance to its
  // exit, so the chained instruction names the exit rather than trailing off at "At the roundabout".
  turnNotifications.clear();
  vector<TurnItemDist> const turns4 = {
      {{5 /* idx */, CarDirection::TurnRight}, 20. /* m_distMeters */},
      {{10 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 220. /* m_distMeters */},
      {{12 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 275. /* m_distMeters */}};
  vector<string> const expectedNotification4 = {{"Turn right. Then. At the roundabout. Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns4, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification4, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::EnterRoundAbout, ());

  // After the first turn.
  turnNotifications.clear();
  vector<TurnItemDist> const turns5 = {
      {{10 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 180. /* m_distMeters */},
      {{12 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 235. /* m_distMeters */},
      {{15 /* idx */, CarDirection::ReachedYourDestination}, 1235. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns5, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // Just before the roundabout: the reminder names the exit.
  turnNotifications.clear();
  vector<TurnItemDist> const turns6 = {
      {{10 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 10. /* m_distMeters */},
      {{12 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 65. /* m_distMeters */},
      {{15 /* idx */, CarDirection::ReachedYourDestination}, 1065. /* m_distMeters */}};
  vector<string> const expectedNotification6 = {{"Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns6, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification6, ());
}

UNIT_TEST(TurnsSound_RoundaboutTurnTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Leave the roundabout.\",\
      \"take_the_1_exit\":\"Take the first exit.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"take_the_4_exit\":\"Take the fourth exit.\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"in_1_kilometer\":\"In 1 kilometer.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\",\
      \"then\":\"Then.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;

  // Starting nearing the first turn.
  // 1000 meters till the first turn.
  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 1000. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 2000. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 620 meters till the first turn.
  // Classic roundabout entrance+exit: combined "In X meters, at the roundabout, take the Nth exit".
  vector<TurnItemDist> const turns2 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 1620. /* m_distMeters */}};
  vector<string> const expectedNotification2 = {{"In 600 meters. At the roundabout. Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification2, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 3 meters till the first turn — reminder at entrance: "Take the Nth exit".
  vector<TurnItemDist> const turns3 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 3. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 1003. /* m_distMeters */}};
  vector<string> const expectedNotification3 = {{"Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification3, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 900 meters till the second turn.
  vector<TurnItemDist> const turns4 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 900. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 1 /* m_exitNum */}, 1900. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns4, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 300 meters till the second turn.
  vector<TurnItemDist> const turns5 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 300. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 1 /* m_exitNum */}, 1300. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns5, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 3 meters till the exit — small-roundabout exit notifications are suppressed (GPS+TTS
  // latency would deliver the announcement after the user has already exited).
  vector<TurnItemDist> const turns6 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 3. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 1 /* m_exitNum */}, 1003. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns6, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 5 meters till the third turn — first call fast-forwards, second emits the reminder.
  vector<TurnItemDist> const turns7 = {
      {{15 /* idx */, CarDirection::EnterRoundAbout, 1 /* m_exitNum */}, 5. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 1 /* m_exitNum */}, 1005. /* m_distMeters */}};
  vector<string> const expectedNotification7 = {{"Take the first exit."}};
  notificationManager.GenerateTurnNotifications(turns7, turnNotifications);  // The first notification fast forwarding.
  notificationManager.GenerateTurnNotifications(turns7, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification7, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 900 meters till the 4th turn.
  notificationManager.Reset();
  vector<TurnItemDist> const turns8 = {
      {{25 /* idx */, CarDirection::EnterRoundAbout, 4 /* m_exitNum */}, 900. /* m_distMeters */},
      {{30 /* idx */, CarDirection::LeaveRoundAbout, 4 /* m_exitNum */}, 1200. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns8, turnNotifications);
  TEST(turnNotifications.empty(), ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::None, ());

  // 620 meters till the 4th turn — combined announcement now includes the exit street name.
  vector<TurnItemDist> const turns9 = {
      {{25 /* idx */, CarDirection::EnterRoundAbout, 4 /* m_exitNum */}, 620. /* m_distMeters */},
      {{30 /* idx */, CarDirection::LeaveRoundAbout, 4 /* m_exitNum */}, 1000. /* m_distMeters */}};
  vector<string> const expectedNotification9 = {
      {"In 600 meters. At the roundabout. Take the fourth exit onto Main street"}};
  notificationManager.GenerateTurnNotifications(turns9, turnNotifications,
                                                routing::RouteSegment::RoadNameInfo("Main street"));
  TEST_EQUAL(turnNotifications, expectedNotification9, ());
  TEST_EQUAL(notificationManager.GetSecondTurnNotification(), CarDirection::LeaveRoundAbout, ());
}

// Small roundabout - The turn that follows a roundabout exit should be announced once, whether or not it was
// chained to the roundabout instruction.
UNIT_TEST(TurnsSound_TurnAfterRoundaboutExitTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"make_a_right_turn\":\"Turn right.\",\
      \"in_300_meters\":\"In 300 meters.\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"then\":\"Then.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;

  // SCENARIO 1

  // A turn 300 meters past the exit is chained to the roundabout instruction.
  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 660. /* m_distMeters */},
      {{15 /* idx */, CarDirection::TurnRight}, 960. /* m_distMeters */}};
  vector<string> const expectedNotification1 = {
      {"In 600 meters. At the roundabout. Take the second exit. Then. In 300 meters. Turn right."}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // Inside the roundabout. The exit is not called out, and the chained turn is not repeated.
  vector<TurnItemDist> const turns2 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 40. /* m_distMeters */},
      {{15 /* idx */, CarDirection::TurnRight}, 340. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // Past the exit. The chained turn is not repeated as its own advance notification.
  vector<TurnItemDist> const turns3 = {{{15 /* idx */, CarDirection::TurnRight}, 340. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // Only the advance notification was skipped. The reminder still fires on the approach, so the
  // driver is told to turn as they reach it.
  vector<TurnItemDist> const turns3AtTheTurn = {{{15 /* idx */, CarDirection::TurnRight}, 90. /* m_distMeters */}};
  vector<string> const expectedNotification3 = {{"Turn right."}};
  notificationManager.GenerateTurnNotifications(turns3AtTheTurn, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification3, ());

  // SCENARIO 2

  // A roundabout announced on its own, with no turn close enough after the exit to chain.
  notificationManager.Reset();
  vector<TurnItemDist> const turns4 = {
      {{20 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{25 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 660. /* m_distMeters */}};
  vector<string> const expectedNotification4 = {{"In 600 meters. At the roundabout. Take the second exit."}};
  notificationManager.GenerateTurnNotifications(turns4, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification4, ());

  // Inside the roundabout, a turn 50 meters past the exit has appeared (e.g. recalc). It was never chained to
  // anything, so it is announced here on its own rather than going unannounced.
  vector<TurnItemDist> const turns5 = {
      {{25 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 40. /* m_distMeters */},
      {{30 /* idx */, CarDirection::TurnRight}, 90. /* m_distMeters */}};
  vector<string> const expectedNotification5 = {{"Turn right."}};
  notificationManager.GenerateTurnNotifications(turns5, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification5, ());

  // And it is not announced a second time when it becomes the closest turn.
  vector<TurnItemDist> const turns6 = {{{30 /* idx */, CarDirection::TurnRight}, 60. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns6, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // The reminder on the approach is still announced.
  vector<TurnItemDist> const turns6AtTheTurn = {{{30 /* idx */, CarDirection::TurnRight}, 40. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns6AtTheTurn, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification5, ());


  // SCENARIO 3

  // A roundabout announced on its own, then a position update that lands past the exit without the
  // exit ever having been the closest turn. Nothing was chained to the roundabout instruction, so
  // the next turn must still get its own first notification.
  notificationManager.Reset();
  vector<TurnItemDist> const turns7 = {
      {{35 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{40 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 660. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns7, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification4, ());

  vector<TurnItemDist> const turns8 = {{{45 /* idx */, CarDirection::TurnRight}, 620. /* m_distMeters */}};
  vector<string> const expectedNotification8 = {{"In 600 meters. Turn right."}};
  notificationManager.GenerateTurnNotifications(turns8, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification8, ());
}

// "Exit the roundabout." is never announced with a distance in front of it.
UNIT_TEST(TurnsSound_NoAdvanceRoundaboutExitNotificationTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"in_200_meters\":\"In 200 meters.\",\
      \"then\":\"Then.\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;

  // The entrance is too close for a first notification, so nothing is announced for it.
  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 50. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 350. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // Inside a large roundabout, 300 meters before the exit. No "In 200 meters. Exit the roundabout."
  vector<TurnItemDist> const turns2 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 300. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications);
  TEST(turnNotifications.empty(), ());

  // On a large roundabout the exit is still called out, as a reminder without a distance in front.
  vector<TurnItemDist> const turns3 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 90. /* m_distMeters */}};
  vector<string> const expectedNotification3 = {{"Exit the roundabout."}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications);
  TEST_EQUAL(turnNotifications, expectedNotification3, ());

  // On a small roundabout the exit is suppressed: it would arrive after the user has already left.
  notificationManager.Reset();
  vector<TurnItemDist> const turns4 = {
      {{15 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 20. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns4, turnNotifications);
  TEST(turnNotifications.empty(), ());
}

// A turn shortly after a roundabout exit is chained to the roundabout instruction, and carries the
// street name for the turn instead of the exit street name.
UNIT_TEST(TurnsSound_RoundaboutWithCloseTurnTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"make_a_right_turn\":\"Turn right.\",\
      \"make_a_right_turn_street\":\"Turn right\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;
  RouteSegment::RoadNameInfo const streetInfo("Maylands Avenue");

  // Turn right 100 meters after the exit, within the chaining threshold. The street name belongs
  // to the turn, not to the roundabout exit.
  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 670. /* m_distMeters */},
      {{15 /* idx */, CarDirection::TurnRight}, 770. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications.size(), 1, (turnNotifications));
  TEST_EQUAL(turnNotifications[0],
             "In 600 meters. At the roundabout. Take the second exit. Then. Turn right onto Maylands Avenue", ());

  // Beyond the threshold the turn is not chained, and the street name goes with the exit instead.
  notificationManager.Reset();
  vector<TurnItemDist> const turns2 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 670. /* m_distMeters */},
      {{15 /* idx */, CarDirection::TurnRight}, 1100. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications.size(), 1, (turnNotifications));
  TEST(turnNotifications[0].find("Maylands Avenue") != string::npos, (turnNotifications[0]));
}

// Two consecutive roundabouts.
UNIT_TEST(TurnsSound_ConsecutiveRoundaboutsTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"take_the_3_exit\":\"Take the third exit.\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;
  RouteSegment::RoadNameInfo const streetInfo("High Street");

  // First roundabout at 620 m, second 80 m past its exit.
  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 670. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 750. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 800. /* m_distMeters */}};
  vector<string> const expectedNotification1 = {
      {"In 600 meters. At the roundabout. Take the second exit. Then. At the roundabout. Take the third exit."}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // The reminder at the entrance chains the second roundabout the same way.
  vector<TurnItemDist> const turns2 = {
      {{5 /* idx */, CarDirection::EnterRoundAbout, 2 /* m_exitNum */}, 5. /* m_distMeters */},
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 55. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 95. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 145. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications.size(), 1, (turnNotifications));
  TEST(turnNotifications[0].find("second exit") != string::npos, (turnNotifications[0]));
  TEST(turnNotifications[0].find("third exit") != string::npos, (turnNotifications[0]));
  TEST(turnNotifications[0].find("Then") != string::npos, (turnNotifications[0]));
}

// Inside the first roundabout with a second one close behind its exit.
UNIT_TEST(TurnsSound_InsideRoundaboutApproachingSecondTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"take_the_3_exit\":\"Take the third exit.\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;
  RouteSegment::RoadNameInfo const streetInfo("High Street");

  // The exit call-out is never announced with a distance in front of it: the advance
  // instruction was already given as "at the roundabout, take the Nth exit" at the entrance
  // It is announced as a reminder, once the exit is close enough.
  vector<TurnItemDist> const turns1 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 700. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 750. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications, streetInfo);
  TEST(turnNotifications.empty(), (turnNotifications));

  // 65 m from the exit: close enough for the reminder, and still beyond the 60 m at which the exit
  // call-out is suppressed. The second roundabout is chained to it.
  vector<TurnItemDist> const turns2 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 65. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 145. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 195. /* m_distMeters */}};
  vector<string> const expectedNotification2 = {{"Exit the roundabout. Then. At the roundabout. Take the third exit."}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification2, ());

  // A new roundabout, its exit 620 m ahead. Nothing is announced: a roundabout exit never gets a
  // distance-prefixed advance notification, and it is too early for the reminder.
  //

  vector<TurnItemDist> const turns3 = {
      {{25 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 620. /* m_distMeters */},
      {{30 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 700. /* m_distMeters */},
      {{35 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 750. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications, streetInfo);
  TEST(turnNotifications.empty(), (turnNotifications));

  // At that exit (5 m away) the call-out is suppressed, but nothing has been said about the
  // roundabout that follows it, so it is announced on its own.
  vector<TurnItemDist> const turns4 = {
      {{25 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 5. /* m_distMeters */},
      {{30 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 85. /* m_distMeters */},
      {{35 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 135. /* m_distMeters */}};
  vector<string> const expectedNotification4 = {{"At the roundabout. Take the third exit."}};
  notificationManager.GenerateTurnNotifications(turns4, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification4, ());

  // Not repeated on the next position update at the same exit.
  vector<TurnItemDist> const turns5 = {
      {{25 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 3. /* m_distMeters */},
      {{30 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 83. /* m_distMeters */},
      {{35 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 133. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns5, turnNotifications, streetInfo);
  TEST(turnNotifications.empty(), (turnNotifications));
}

// Inside a small roundabout, closer to its exit than the suppression threshold, with a second
// roundabout shortly after it. The exit call-out stays suppressed, but the second roundabout must
// still be announced.
UNIT_TEST(TurnsSound_SmallRoundaboutExitWithCloseSecondRoundaboutTest)
{
  string const engShortJson =
      "\
      {\
      \"enter_the_roundabout\":\"At the roundabout.\",\
      \"leave_the_roundabout\":\"Exit the roundabout.\",\
      \"take_the_2_exit\":\"Take the second exit.\",\
      \"take_the_3_exit\":\"Take the third exit.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;
  RouteSegment::RoadNameInfo const streetInfo("High Street");

  vector<TurnItemDist> const turns1 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 40. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 90. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 140. /* m_distMeters */}};
  vector<string> const expectedNotification1 = {{"At the roundabout. Take the third exit."}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // Not repeated on the next position update while still at the same exit.
  vector<TurnItemDist> const turns2 = {
      {{10 /* idx */, CarDirection::LeaveRoundAbout, 2 /* m_exitNum */}, 20. /* m_distMeters */},
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 70. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 120. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications, streetInfo);
  TEST(turnNotifications.empty(), (turnNotifications));

  // And not again as its own first notification once it becomes the closest turn.
  vector<TurnItemDist> const turns3 = {
      {{15 /* idx */, CarDirection::EnterRoundAbout, 3 /* m_exitNum */}, 50. /* m_distMeters */},
      {{20 /* idx */, CarDirection::LeaveRoundAbout, 3 /* m_exitNum */}, 100. /* m_distMeters */}};
  notificationManager.GenerateTurnNotifications(turns3, turnNotifications, streetInfo);
  TEST(turnNotifications.empty(), (turnNotifications));
}


// A slight turn with another turn just after it carries no street name either
UNIT_TEST(TurnsSound_SlightTurnWithCloseSecondTurnTest)
{
  string const engShortJson =
      "\
      {\
      \"make_a_slight_right_turn\":\"Slight right.\",\
      \"make_a_slight_right_turn_street\":\"Slight right\",\
      \"make_a_right_turn\":\"Turn right.\",\
      \"in_600_meters\":\"In 600 meters.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\"\
      }";
  auto notificationManager = NotificationManager::CreateNotificationManagerForTesting(
      5 /* startBeforeSeconds */, 10 /* minStartBeforeMeters */, 100 /* maxStartBeforeMeters */,
      100 /* minDistToSayNotificationMeters */, measurement_utils::Units::Metric, engShortJson,
      30 /* notificationTimeSecond */, 20.0 /* speedMeterPerSecond */);

  vector<string> turnNotifications;
  RouteSegment::RoadNameInfo const streetInfo("High Street");

  vector<TurnItemDist> const turns1 = {
      {{5 /* idx */, CarDirection::TurnSlightRight}, 620. /* m_distMeters */},
      {{10 /* idx */, CarDirection::TurnRight}, 680. /* m_distMeters */}};
  vector<string> const expectedNotification1 = {{"In 600 meters. Slight right. Then. Turn right."}};
  notificationManager.GenerateTurnNotifications(turns1, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification1, ());

  // With nothing immediately after it, the slight turn keeps the street name.
  notificationManager.Reset();
  vector<TurnItemDist> const turns2 = {
      {{15 /* idx */, CarDirection::TurnSlightRight}, 620. /* m_distMeters */},
      {{20 /* idx */, CarDirection::TurnRight}, 1100. /* m_distMeters */}};
  vector<string> const expectedNotification2 = {{"In 600 meters Slight right onto High Street"}};
  notificationManager.GenerateTurnNotifications(turns2, turnNotifications, streetInfo);
  TEST_EQUAL(turnNotifications, expectedNotification2, ());
}

UNIT_TEST(GetJsonBufferTest)
{
  string const localeNameEn = "en";
  string jsonBuffer;
  TEST(GetJsonBuffer(platform::TextSource::TtsSound, localeNameEn, jsonBuffer), ());
  TEST(!jsonBuffer.empty(), ());

  string const localeNameRu = "ru";
  jsonBuffer.clear();
  TEST(GetJsonBuffer(platform::TextSource::TtsSound, localeNameRu, jsonBuffer), ());
  TEST(!jsonBuffer.empty(), ());
}
}  // namespace turns_sound_test
