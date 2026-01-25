package app.organicmaps.sdk.downloader;

import android.os.AsyncTask;
import androidx.annotation.Keep;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.sdk.util.log.Logger;
import okhttp3.CacheControl;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;
import okhttp3.ResponseBody;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

// Used from JNI.
@Keep
@SuppressWarnings({"unused", "deprecation"}) // https://github.com/organicmaps/organicmaps/issues/3632
class ChunkTask extends AsyncTask<Void, byte[], Integer>
{
  private static final String TAG = ChunkTask.class.getSimpleName();

  private static final int TIMEOUT_IN_SECONDS = 10;

  private static final int IO_EXCEPTION = -1;
  private static final int WRITE_EXCEPTION = -2;
  private static final int INCONSISTENT_FILE_SIZE = -3;
  private static final int NON_HTTP_RESPONSE = -4;
  private static final int INVALID_URL = -5;
  private static final int CANCELLED = -6;

  private final long mHttpCallbackID;
  private final String mUrl;
  private final long mBeg;
  private final long mEnd;
  private final long mExpectedFileSize;
  private final byte[] mPostBody;
  private long mDownloadedBytes;

  private static final Executor sExecutors = Executors.newFixedThreadPool(4);

  public ChunkTask(long httpCallbackID, String url, long beg, long end, long expectedFileSize, byte[] postBody)
  {
    mHttpCallbackID = httpCallbackID;
    mUrl = url;
    mBeg = beg;
    mEnd = end;
    mExpectedFileSize = expectedFileSize;
    mPostBody = postBody;
  }

  @Override
  protected void onPreExecute()
  {}

  @Override
  protected void onPostExecute(Integer httpOrErrorCode)
  {
    // It seems like onPostExecute can be called (from GUI thread queue)
    // after the task was cancelled in destructor of HttpThread.
    // Reproduced by Samsung testers: touch Try Again for many times from
    // start activity when no connection is present.

    if (!isCancelled())
      nativeOnFinish(mHttpCallbackID, httpOrErrorCode, mBeg, mEnd);
  }

  @Override
  protected void onProgressUpdate(byte[]... data)
  {
    if (!isCancelled())
    {
      // Use progress event to save downloaded bytes.
      if (nativeOnWrite(mHttpCallbackID, mBeg + mDownloadedBytes, data[0], data[0].length))
        mDownloadedBytes += data[0].length;
      else
      {
        // Cancel downloading and notify about error.
        cancel(false);
        nativeOnFinish(mHttpCallbackID, WRITE_EXCEPTION, mBeg, mEnd);
      }
    }
  }

  void start()
  {
    executeOnExecutor(sExecutors, (Void[]) null);
  }

  private static long parseContentRange(String contentRangeValue)
  {
    if (contentRangeValue != null)
    {
      final int slashIndex = contentRangeValue.lastIndexOf('/');
      if (slashIndex >= 0)
      {
        try
        {
          return Long.parseLong(contentRangeValue.substring(slashIndex + 1));
        }
        catch (final NumberFormatException ex)
        {
          return -1;
        }
      }
    }
    return -1;
  }

  @Override
  protected Integer doInBackground(Void... p)
  {
    OkHttpClient client = new OkHttpClient.Builder()
        .connectTimeout(TIMEOUT_IN_SECONDS, TimeUnit.SECONDS)
        .readTimeout(TIMEOUT_IN_SECONDS, TimeUnit.SECONDS)
        .build();

    try
    {
      // .addHeader("Connection", "close") is work-around for situation when more than
      // one consequent POST requests can lead to stable
      // "java.net.ProtocolException: Unexpected status line:" on a client and Nginx HTTP 499 errors.
      // The only found reference to this bug is http://stackoverflow.com/a/24303115/1209392
      Request.Builder requestBuilder = new Request.Builder()
          .url(mUrl)
          .cacheControl(new CacheControl.Builder().noStore().noCache().build())
          .addHeader("Connection", "close");

      if (mPostBody != null)
      {
        requestBuilder = requestBuilder.post(RequestBody.create(mPostBody, null));
      }

      if (!(mBeg == 0 && mEnd < 0))
      {
        if (mEnd > 0)
          requestBuilder = requestBuilder.header("Range", StringUtils.formatUsingUsLocale("bytes=%d-%d", mBeg, mEnd));
        else
          requestBuilder = requestBuilder.header("Range", StringUtils.formatUsingUsLocale("bytes=%d-", mBeg));
      }

      Request request = requestBuilder.build();

      try (Response response = client.newCall(request).execute())
      {
        if (isCancelled())
          return CANCELLED;

        int err = response.code();
        if (err == HttpURLConnection.HTTP_NOT_FOUND)
          return err;

       final boolean isChunk = !(mBeg == 0 && mEnd < 0);
        if ((isChunk && err != HttpURLConnection.HTTP_PARTIAL) || (!isChunk && err != HttpURLConnection.HTTP_OK))
        {
          // we've set error code so client should be notified about the error
          Logger.w(TAG, "Error for " + mUrl + ": Server replied with code " + err
              + ", aborting download. Headers: " + request.headers());
          return INCONSISTENT_FILE_SIZE;
        }

        // Check for content size - are we downloading requested file or some router's garbage?
        if (mExpectedFileSize > 0)
        {
          long contentLength = parseContentRange(response.header("Content-Range"));
          if (contentLength < 0)
            contentLength = response.body().contentLength();

          if (contentLength != mExpectedFileSize)
          {
            // we've set error code so client should be notified about the error
            Logger.w(TAG, "Error for " + mUrl + ": Invalid file size received (" + contentLength + ") while expecting " + mExpectedFileSize + ". Aborting download.");
            return INCONSISTENT_FILE_SIZE;
          }
          // @TODO Else display received web page to user - router is redirecting us to some page ?
        }

        try (ResponseBody responseBody = response.body())
        {
          try (InputStream stream = responseBody.byteStream())
          {
            return downloadFromStream(stream);
          }
        }
      }
    }
    catch (final IOException ex)
    {
      Logger.d(TAG, "IOException in doInBackground for URL: " + mUrl, ex);
      return IO_EXCEPTION;
    }
  }

  private Integer downloadFromStream(InputStream stream)
  {
    // Because of timeouts in InputStream.read (for bad connection),
    // try to introduce dynamic buffer size to read in one query.
    final int[] arrSize = {128, 32, 1};
    int ret = IO_EXCEPTION;

    for (int size : arrSize)
    {
      try
      {
        ret = downloadFromStreamImpl(stream, size * Constants.KB);
        break;
      }
      catch (final IOException ex)
      {
        Logger.e(TAG, "IOException in downloadFromStream for buffer size: " + size, ex);
      }
    }

    Utils.closeSafely(stream);
    return ret;
  }

  /**
   * @throws IOException
   */
  private int downloadFromStreamImpl(InputStream stream, int bufferSize) throws IOException
  {
    final byte[] tempBuf = new byte[bufferSize];

    int readBytes;
    while ((readBytes = stream.read(tempBuf)) > 0)
    {
      if (isCancelled())
        return CANCELLED;

      final byte[] chunk = new byte[readBytes];
      System.arraycopy(tempBuf, 0, chunk, 0, readBytes);

      publishProgress(chunk);
    }

    // -1 - means the end of the stream (success), else - some error occurred
    return (readBytes == -1 ? HttpURLConnection.HTTP_OK : IO_EXCEPTION);
  }

  private static native boolean nativeOnWrite(long httpCallbackID, long beg, byte[] data, long size);
  private static native void nativeOnFinish(long httpCallbackID, long httpCode, long beg, long end);
}
