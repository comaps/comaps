package app.organicmaps.api;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.sdk.util.log.Logger;

import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

/**
 * HTTP API client for location sharing server.
 * Sends encrypted location updates to the server.
 */
public class LocationSharingApiClient
{
  private static final String TAG = LocationSharingApiClient.class.getSimpleName();

  private static final int CONNECT_TIMEOUT_MS = 10000;
  private static final int READ_TIMEOUT_MS = 10000;

  private final String mServerBaseUrl;
  private final String mSessionId;
  private final Executor mExecutor;

  public interface Callback
  {
    void onSuccess();
    void onFailure(@NonNull String error);
  }

  public LocationSharingApiClient(@NonNull String serverBaseUrl, @NonNull String sessionId)
  {
    mServerBaseUrl = serverBaseUrl.endsWith("/") ? serverBaseUrl : serverBaseUrl + "/";
    mSessionId = sessionId;
    mExecutor = Executors.newSingleThreadExecutor();
  }

  /**
   * Create a new session on the server.
   * @param callback Result callback
   */
  public void createSession(@Nullable Callback callback)
  {
    mExecutor.execute(() -> {
      try
      {
        String url = mServerBaseUrl + "api/v1/session";
        String requestBody = "{\"sessionId\":\"" + mSessionId + "\"}";

        int responseCode = postJson(url, requestBody);

        if (responseCode >= 200 && responseCode < 300)
        {
          Logger.d(TAG, "Session created successfully: " + mSessionId);
          if (callback != null)
            callback.onSuccess();
        }
        else
        {
          String error = "Server returned error: " + responseCode;
          Logger.w(TAG, error);
          if (callback != null)
            callback.onFailure(error);
        }
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Failed to create session", e);
        if (callback != null)
          callback.onFailure(e.getMessage());
      }
    });
  }

  /**
   * Update location on the server with encrypted payload.
   * @param encryptedPayloadJson Encrypted payload JSON (from native code)
   * @param callback Result callback
   */
  public void updateLocation(@NonNull String encryptedPayloadJson, @Nullable Callback callback)
  {
    mExecutor.execute(() -> {
      try
      {
        String url = mServerBaseUrl + "api/v1/location/" + mSessionId;

        int responseCode = postJson(url, encryptedPayloadJson);

        if (responseCode >= 200 && responseCode < 300)
        {
          Logger.d(TAG, "Location updated successfully");
          if (callback != null)
            callback.onSuccess();
        }
        else
        {
          String error = "Server returned error: " + responseCode;
          Logger.w(TAG, error);
          if (callback != null)
            callback.onFailure(error);
        }
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Failed to update location", e);
        if (callback != null)
          callback.onFailure(e.getMessage());
      }
    });
  }

  /**
   * End the session on the server.
   */
  public void endSession()
  {
    mExecutor.execute(() -> {
      try
      {
        String url = mServerBaseUrl + "api/v1/session/" + mSessionId;
        deleteRequest(url);
        Logger.d(TAG, "Session ended: " + mSessionId);
      }
      catch (IOException e)
      {
        Logger.e(TAG, "Failed to end session", e);
      }
    });
  }

  /**
   * Send a POST request with JSON body.
   * @param urlString URL to send request to
   * @param jsonBody JSON request body
   * @return HTTP response code
   * @throws IOException on network error
   */
  private int postJson(@NonNull String urlString, @NonNull String jsonBody) throws IOException
  {
    URL url = new URL(urlString);
    HttpURLConnection connection = (HttpURLConnection) url.openConnection();

    try
    {
      connection.setRequestMethod("POST");
      connection.setRequestProperty("Content-Type", "application/json; charset=UTF-8");
      connection.setRequestProperty("Accept", "application/json");
      connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
      connection.setReadTimeout(READ_TIMEOUT_MS);
      connection.setDoOutput(true);

      // Write body
      byte[] bodyBytes = jsonBody.getBytes(StandardCharsets.UTF_8);
      connection.setFixedLengthStreamingMode(bodyBytes.length);

      try (OutputStream os = connection.getOutputStream())
      {
        os.write(bodyBytes);
        os.flush();
      }

      return connection.getResponseCode();
    }
    finally
    {
      connection.disconnect();
    }
  }

  /**
   * Send a DELETE request.
   * @param urlString URL to send request to
   * @return HTTP response code
   * @throws IOException on network error
   */
  private int deleteRequest(@NonNull String urlString) throws IOException
  {
    URL url = new URL(urlString);
    HttpURLConnection connection = (HttpURLConnection) url.openConnection();

    try
    {
      connection.setRequestMethod("DELETE");
      connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
      connection.setReadTimeout(READ_TIMEOUT_MS);

      return connection.getResponseCode();
    }
    finally
    {
      connection.disconnect();
    }
  }
}
