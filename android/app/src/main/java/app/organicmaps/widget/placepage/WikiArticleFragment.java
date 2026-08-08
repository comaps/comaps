package app.organicmaps.widget.placepage;

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
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.WindowInsetUtils;

import java.util.Objects;

public class WikiArticleFragment extends BaseMwmFragment
{
  public static final String EXTRA_WIKI_ARTICLE = "description";

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

    final String textColor = colorToCssHex(android.R.attr.textColorPrimary);
    final String textColorSecondary = colorToCssHex(android.R.attr.textColorSecondary);

    webView.setBackgroundColor(ContextCompat.getColor(requireContext(), R.color.bg_app));
    webView.setVerticalScrollBarEnabled(true);

    final String html = buildHtml(mDescription, textColor, textColorSecondary);
    webView.loadDataWithBaseURL(
            null,
            html,
            "text/html",
            "UTF-8",
            null
    );
    ViewCompat.setOnApplyWindowInsetsListener(root, WindowInsetUtils.PaddingInsetsListener.excludeTop());
    return root;
  }

  @NonNull
  private String buildHtml(@NonNull String content, @NonNull String textColor, @NonNull String textColorSecondary)
  {

    return "<!DOCTYPE html>" +
            "<html dir='auto'>" +
            "<head>" +
            "<meta charset='utf-8'>" +
            "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0'>" +
            "<style>" +
            "html, body {" +
            "  margin: 0;" +
            "  padding: 0;" +
            "}" +
            "body {" +
            "  padding: 24px 20px 32px;" +
            "  color: " + textColor + ";" +
            "  line-height: 1.45;" +
            "  word-wrap: break-word;" +
            "  text-align: start;" +
            "  text-justify: inter-word;" +
            "}" +
            "p {" +
            "  margin: 0 0 16px 0;" +
            "  text-align: start;" +
            "}" +
            "h1, h2, h3 {" +
            "  margin: 24px 0 12px 0;" +
            "  line-height: 1.25;" +
            "  text-align: start;" +
            "  color: " + textColor + ";" +
            "}" +
            "img {" +
            "  max-width: 100%;" +
            "  height: auto;" +
            "}" +
            "a {" +
            "  text-decoration: none;" +
            "}" +
            ".source {" +
            "  margin-top: 24px;" +
            "  color: " + textColorSecondary + ";" +
            "  text-align: start;" +
            "}" +
            "</style>" +
            "</head>" +
            "<body>" +
            content +
            "<p class='source'><b>wikipedia.org</b></p>" +
            "</body>" +
            "</html>";
  }

  private String colorToCssHex(int colorRes)
  {
    final int color = ThemeUtils.getColor(requireContext(), colorRes);
    // Convert Android color int (0xAARRGGBB) to CSS hex with alpha (#RRGGBBAA)
    final int rgb = color & 0x00FFFFFF;
    final int alpha = (color >>> 24) & 0xFF;
    return String.format("#%08X", (rgb << 8) | alpha);
  }
}
