package app.organicmaps.widget.placepage;

import android.content.res.Configuration;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebSettings;
import android.webkit.WebView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.util.Utils;
import app.organicmaps.util.WindowInsetUtils;

import java.util.Locale;
import java.util.Objects;

public class WikiArticleFragment extends BaseMwmFragment
{
  public static final String EXTRA_WIKI_ARTICLE = "description";
  private static final String SOURCE_SUFFIX = "<p><b>wikipedia.org</b></p>";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private String mDescription;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mDescription = Objects.requireNonNull(requireArguments().getString(EXTRA_WIKI_ARTICLE));
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_place_description, container, false);
    WebView webView = root.findViewById(R.id.webview);
    WebSettings settings = webView.getSettings();
    settings.setBuiltInZoomControls(false);
    settings.setDisplayZoomControls(false);

    String textColor = colorToHex(isDarkMode() ? R.color.text_light : R.color.text_dark);
    String bgColor = colorToHex(R.color.bg_window);

    webView.setBackgroundColor(ContextCompat.getColor(requireContext(), R.color.bg_window));
    webView.setVerticalScrollBarEnabled(true);

    String html = buildHtml(mDescription + SOURCE_SUFFIX, textColor, bgColor);
    webView.loadDataWithBaseURL(
            "https://wikipedia.org/",
            html,
            Utils.TEXT_HTML,
            Utils.UTF_8,
            null
    );
    ViewCompat.setOnApplyWindowInsetsListener(root, WindowInsetUtils.PaddingInsetsListener.excludeTop());
    return root;
  }

  @NonNull
  private String buildHtml(@NonNull String content, @NonNull String textColor, @NonNull String bgColor)
  {
    String secondaryTextColor = isDarkMode() ? "#A0A0A0" : "#6B6B6B";
    String linkColor = isDarkMode() ? "#8AB4F8" : "#1565C0";

    return "<!DOCTYPE html>" +
            "<html>" +
            "<head>" +
            "<meta charset='utf-8'>" +
            "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'>" +
            "<style>" +
            "html, body {" +
            "  margin: 0;" +
            "  padding: 0;" +
            "  background: " + bgColor + ";" +
            "}" +
            "body {" +
            "  padding: 24px 20px 32px;" +
            "  color: " + textColor + ";" +
            "  background: " + bgColor + ";" +
            "  font-size: 17px;" +
            "  line-height: 1.65;" +
            "  font-family: sans-serif;" +
            "  word-wrap: break-word;" +
            "  text-align: justify;" +
            "  text-justify: inter-word;" +
            "}" +
            "p {" +
            "  margin: 0 0 16px 0;" +
            "  text-align: justify;" +
            "}" +
            "h1, h2, h3 {" +
            "  margin: 24px 0 12px 0;" +
            "  line-height: 1.25;" +
            "  text-align: left;" +
            "  color: " + textColor + ";" +
            "}" +
            "img {" +
            "  max-width: 100%;" +
            "  height: auto;" +
            "}" +
            "a {" +
            "  color: " + linkColor + ";" +
            "  text-decoration: none;" +
            "}" +
            ".source {" +
            "  margin-top: 24px;" +
            "  color: " + secondaryTextColor + ";" +
            "  font-size: 14px;" +
            "  text-align: left;" +
            "}" +
            "</style>" +
            "</head>" +
            "<body>" +
            content +
            "</body>" +
            "</html>";
  }

  private boolean isDarkMode()
  {
    int nightFlags = getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
    return nightFlags == Configuration.UI_MODE_NIGHT_YES;
  }
  private String colorToHex(int colorRes)
  {
    return String.format(Locale.ROOT, "#%06X", 0xFFFFFF & ContextCompat.getColor(requireContext(), colorRes));
  }
}
