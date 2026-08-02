package app.organicmaps.sdk.editor;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.WorkerThread;

import app.organicmaps.sdk.bookmarks.data.FeatureId;

public final class ReviewEditor
{
  private ReviewEditor() {}

  /**
   * Resolves the review editor URL for the specified feature. Might involve a network call to OSM API.
   *
   * @param featureId the id of the feature to resolve editor URL for
   * @return the review editor URL for the specified feature, or <code>null</code> if no editor is available
   */
  @WorkerThread
  @Nullable
  public static native String nativeGetReviewEditorUrl(@NonNull FeatureId featureId);
}
