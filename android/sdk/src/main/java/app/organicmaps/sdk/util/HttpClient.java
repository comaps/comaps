/*******************************************************************************
 * The MIT License (MIT)
 * <p/>
 * Copyright (c) 2014 Alexander Borsuk <me@alex.bio> from Minsk, Belarus
 * <p/>
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * <p/>
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * <p/>
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

package app.organicmaps.sdk.util;

import android.text.TextUtils;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import app.organicmaps.sdk.util.log.Logger;
import okhttp3.CacheControl;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;

import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

// Used by JNI.
@Keep
@SuppressWarnings("unused")
public final class HttpClient
{
  private static final String TAG = HttpClient.class.getSimpleName();

  // TODO(AlexZ): tune for larger files
  private final static int STREAM_BUFFER_SIZE = 1024 * 64;

  private static OkHttpClient client = new OkHttpClient();

  public static Params run(@NonNull final Params p) throws IOException, NullPointerException
  {
    if (TextUtils.isEmpty(p.httpMethod))
      throw new IllegalArgumentException("Please set valid HTTP method for request at Params.httpMethod field.");

    Logger.d(TAG, "Connecting to " + Utils.makeUrlSafe(p.url));

    try
    {
      // NullPointerException, MalformedUrlException, IOException
      // Redirects from http to https or vice versa are not supported by Android implementation.

      Request.Builder requestBuilder = new Request.Builder()
          .url(p.url)
          .cacheControl(
              new CacheControl.Builder()
                  .noStore()
                  .noCache()
                  .build()
          );

      for (KeyValue header : (KeyValue[]) p.getHeaders())
        requestBuilder = requestBuilder.header(header.getKey(), header.getValue());

      client = client.newBuilder()
          .followRedirects(p.followRedirects)
          .followSslRedirects(p.followRedirects)
          .connectTimeout(Duration.ofMillis(p.timeoutMillisec))
          .readTimeout(Duration.ofMillis(p.timeoutMillisec))
          .build();


      if (!TextUtils.isEmpty(p.cookies))
        requestBuilder = requestBuilder.header("Cookie", p.cookies);

      if (!TextUtils.isEmpty(p.inputFilePath) || p.data != null)
      {
        // Send (POST, PUT...) data to the server.
        if (TextUtils.isEmpty(requestBuilder.getHeaders$okhttp().get("Content-Type")))
          throw new NullPointerException("Please set Content-Type for request.");

        // Work-around for situation when more than one consequent POST requests can lead to stable
        // "java.net.ProtocolException: Unexpected status line:" on a client and Nginx HTTP 499 errors.
        // The only found reference to this bug is http://stackoverflow.com/a/24303115/1209392
        requestBuilder = requestBuilder.header("Connection", "close");
        if (p.data != null)
        {
          requestBuilder = requestBuilder.method(p.httpMethod, RequestBody.create(p.data, MediaType.parse("application/octet-stream")));
          Logger.d(TAG, "Sent " + p.httpMethod + " with content of size " + p.data.length);
        }
        else
        {
          File file = new File(p.inputFilePath);
          requestBuilder = requestBuilder.method(p.httpMethod, RequestBody.create(file, MediaType.parse("application/octet-stream")));
          Logger.d(TAG, "Sent " + p.httpMethod + " with file of size " + file.length());
        }
      }

      Request request = requestBuilder.build();

      try (Response response = client.newCall(request).execute())
      {
        // GET data from the server or receive response body
        p.httpResponseCode = response.code();
        Logger.d(TAG, "Received HTTP " + p.httpResponseCode + " from server, content encoding = "
            + response.header("Content-Encoding") + ", for request = " + Utils.makeUrlSafe(p.url));

        if (p.httpResponseCode >= 300 && p.httpResponseCode < 400)
          p.receivedUrl = request.header("Location");
        else
          p.receivedUrl = response.request().url().toString();

        p.headers.clear();
        if (p.loadHeaders)
        {
          for (Map.Entry<String, List<String>> header : response.headers().toMultimap().entrySet())
          {
            p.headers.add(
                new KeyValue(StringUtils.toLowerCase(header.getKey()), TextUtils.join(", ", header.getValue())));
          }
        }
        else
        {
          List<String> cookies = response.headers("Set-Cookie");
          p.headers.add(new KeyValue("Set-Cookie", TextUtils.join(", ", cookies)));
        }

        OutputStream ostream;
        if (!TextUtils.isEmpty(p.outputFilePath))
          ostream = new BufferedOutputStream(new FileOutputStream(p.outputFilePath), STREAM_BUFFER_SIZE);
        else
          ostream = new ByteArrayOutputStream(STREAM_BUFFER_SIZE);
        // TODO(AlexZ): Add HTTP resume support in the future for partially downloaded files
        final InputStream istream = response.body().byteStream();
        final byte[] buffer = new byte[STREAM_BUFFER_SIZE];
        // gzip encoding is transparently enabled and we can't use Content-Length for
        // body reading if server has gzipped it.
        int bytesRead;
        while ((bytesRead = istream.read(buffer, 0, STREAM_BUFFER_SIZE)) > 0)
        {
          // Read everything if Content-Length is not known in advance.
          ostream.write(buffer, 0, bytesRead);
        }
        istream.close(); // IOException
        ostream.close(); // IOException
        if (ostream instanceof ByteArrayOutputStream)
          p.data = ((ByteArrayOutputStream) ostream).toByteArray();

      }
      catch (IOException ex)
      {
        // Exception here means that there is no body in the response.
      }
    } catch (Exception e)
    {
      Logger.d(TAG, Arrays.toString(e.getStackTrace()));
    }
    return p;
  }

  // Used by JNI.
  @Keep
  @SuppressWarnings("unused")
  private static class Params
  {
    public void setHeaders(@NonNull KeyValue[] array)
    {
      headers = new ArrayList<>(Arrays.asList(array));
    }

    public Object[] getHeaders()
    {
      return headers.toArray();
    }

    public String url;
    // Can be different from url in case of redirects.
    String receivedUrl;
    String httpMethod;
    // Should be specified for any request whose method allows non-empty body.
    // On return, contains received Content-Type or null.
    // Can be specified for any request whose method allows non-empty body.
    // On return, contains received Content-Encoding or null.
    public byte[] data;
    // Send from input file if specified instead of data.
    String inputFilePath;
    // Received data is stored here if not null or in data otherwise.
    String outputFilePath;
    String cookies;
    ArrayList<KeyValue> headers = new ArrayList<>();
    int httpResponseCode = -1;
    boolean followRedirects = true;
    boolean loadHeaders;
    int timeoutMillisec = Constants.READ_TIMEOUT_MS;

    // Simple GET request constructor.
    public Params(String url)
    {
      this.url = url;
      httpMethod = "GET";
    }
  }
}
