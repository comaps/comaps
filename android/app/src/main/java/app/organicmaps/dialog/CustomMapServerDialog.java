package app.organicmaps.dialog;

import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.preference.PreferenceManager;

import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import app.organicmaps.R;
import app.organicmaps.sdk.Framework;

public final class CustomMapServerDialog
{
  public interface OnUrlAppliedListener
  {
    void onUrlApplied(@NonNull String url);
  }

  private CustomMapServerDialog() {}

  public static void show(@NonNull Context context,
                          @Nullable OnUrlAppliedListener listener)
  {
    View dialogView = LayoutInflater.from(context)
        .inflate(R.layout.dialog_custom_map_server, null);
    TextInputLayout til = dialogView.findViewById(R.id.til_custom_map_server);
    TextInputEditText edit = dialogView.findViewById(R.id.edit_custom_map_server);

    SharedPreferences prefs =
        PreferenceManager.getDefaultSharedPreferences(context);
    String current = prefs.getString(context.getString(R.string.pref_custom_map_download_url), "");
    edit.setText(current);

    MaterialAlertDialogBuilder builder =
        new MaterialAlertDialogBuilder(context)
            .setTitle(R.string.download_resources_custom_url_title)
            .setMessage(R.string.download_resources_custom_url_message)
            .setView(dialogView)
            .setNegativeButton(R.string.cancel, null)
            .setPositiveButton(R.string.save, null);

    AlertDialog dialog = builder.create();
    dialog.setOnShowListener(dlg -> {
      Button ok = dialog.getButton(DialogInterface.BUTTON_POSITIVE);
      ok.setOnClickListener(v -> {
        String url = edit.getText() != null ? edit.getText().toString().trim() : "";

        if (!url.isEmpty()
            && !url.startsWith("http://")
            && !url.startsWith("https://"))
        {
          til.setError(context.getString(R.string.download_resources_custom_url_error_scheme));
          return;
        }

        til.setError(null);

        String normalizedUrl = Framework.normalizeServerUrl(url);

        prefs.edit()
            .putString(context.getString(R.string.pref_custom_map_download_url), normalizedUrl)
            .apply();

        // Apply to native
        Framework.applyCustomMapDownloadUrl(context, normalizedUrl);

        if (listener != null)
          listener.onUrlApplied(normalizedUrl);

        dialog.dismiss();
      });
    });

    dialog.show();
  }
}
