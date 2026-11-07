import SwiftUI

enum PlacePageAction: Hashable {
  case download
  case call
  case routeFrom
  case routeTo
  case routeAddStop
  case routeRemoveStop
  case bookmark
  case deleteTrack
  case saveTrackRecording
  case discardTrackRecording
  case avoidToll
  case avoidDirty
  case avoidFerry
}
