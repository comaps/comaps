package app.organicmaps.sdk.transit;

import app.organicmaps.sdk.motis.model.Match;
import app.organicmaps.sdk.motis.model.Stoptimes200Response;
import app.organicmaps.sdk.util.concurrency.AsyncResult;

import java.util.List;

public interface TransitService {

  AsyncResult<List<Match>> reverseGeocodeStops(double lat, double lon);

  AsyncResult<Stoptimes200Response> stopTimes(String stopId, int limit);

  AsyncResult<Stoptimes200Response> closestStopWithTimes(
      double lat, double lon, int limit);
}

