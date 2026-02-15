package app.organicmaps.sdk.transit;

import app.organicmaps.sdk.motis.api.GeocodeApi;
import app.organicmaps.sdk.motis.api.TimetableApi;
import app.organicmaps.sdk.motis.invoker.ApiCallback;
import app.organicmaps.sdk.motis.invoker.ApiClient;
import app.organicmaps.sdk.motis.invoker.ApiException;
import app.organicmaps.sdk.motis.model.LocationType;
import app.organicmaps.sdk.motis.model.Match;
import app.organicmaps.sdk.motis.model.Stoptimes200Response;
import app.organicmaps.sdk.util.concurrency.AsyncResult;

import java.time.OffsetDateTime;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Executor;

public class OnlineTransitService implements TransitService {

  private final GeocodeApi geocodeApi;
  private final TimetableApi timetableApi;
  private final Executor executor;

  public OnlineTransitService(String baseUrl, Executor executor) {
    ApiClient client = new ApiClient();
    client.setBasePath(baseUrl);

    this.geocodeApi = new GeocodeApi(client);
    this.timetableApi = new TimetableApi(client);
    this.executor = executor;
  }

  @Override
  public AsyncResult<List<Match>> reverseGeocodeStops(double lat, double lon) {

    AsyncResult<List<Match>> result = new AsyncResult<>(executor);

    try
    {
      geocodeApi.reverseGeocode(lat + ", " + lon)
          .type(LocationType.STOP)
          .executeAsync(new ApiCallback<>() {
            @Override
            public void onSuccess(List<Match> matches,
                                  int statusCode,
                                  Map<String, List<String>> headers) {
              result.complete(matches);
            }

            @Override
            public void onFailure(ApiException e,
                                  int statusCode,
                                  Map<String, List<String>> headers) {
              result.fail(e);
            }

            @Override public void onUploadProgress(long b,long c,boolean d){}
            @Override public void onDownloadProgress(long b,long c,boolean d){}
          });
    } catch (ApiException e)
    {
      result.fail(e);
    }
    return result;
  }

  @Override
  public AsyncResult<Stoptimes200Response> stopTimes(String stopId, int limit) {

    AsyncResult<Stoptimes200Response> result = new AsyncResult<>(executor);

    try {
      timetableApi.stoptimes(stopId, limit)
          .time(OffsetDateTime.now())
          .executeAsync(new ApiCallback<>() {

            @Override
            public void onSuccess(Stoptimes200Response response,
                                  int statusCode,
                                  Map<String, List<String>> headers) {
              result.complete(response);
            }

            @Override
            public void onFailure(ApiException e,
                                  int statusCode,
                                  Map<String, List<String>> headers) {
              result.fail(e);
            }

            @Override public void onUploadProgress(long b,long c,boolean d){}
            @Override public void onDownloadProgress(long b,long c,boolean d){}
          });
    } catch (ApiException e) {
      result.fail(e);
    }

    return result;
  }


  @Override
  public AsyncResult<Stoptimes200Response> closestStopWithTimes(
      double lat, double lon, int limit) {

    return reverseGeocodeStops(lat, lon)
        .thenApply(matches -> {
          if (matches.isEmpty()) {
            throw new RuntimeException("No stops found");
          }
          return matches.get(0).getId();
        })
        .thenCompose(stopId -> stopTimes(stopId, limit));
  }
}

