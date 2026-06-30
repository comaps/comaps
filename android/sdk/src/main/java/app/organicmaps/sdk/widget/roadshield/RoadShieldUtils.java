package app.organicmaps.sdk.widget.roadshield;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;
import android.text.style.ImageSpan;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.roadshield.RoadShield;

public class RoadShieldUtils
{
  public interface RoadShieldSpanCreator
  {
    @NonNull
    CharacterStyle create(@NonNull RoadShieldDrawable roadShieldDrawable);
  }

  @NonNull
  public static CharSequence composeInstruction(@NonNull RoutingInfo info, float textSize)
  {
    return composeInstruction(RoadShieldUtils::createRoadShieldSpan, info, textSize, true);
  }

  @NonNull
  public static CharSequence composeInstruction(@NonNull RoadShieldSpanCreator roadShieldSpanCreator,
                                                @NonNull RoutingInfo info, float textSize, boolean drawOutline)
  {
    if (info.nextName.isEmpty() && info.nextRef.isEmpty() && info.nextJunctionRef.isEmpty() && info.nextDestinationRef.isEmpty() && info.nextDestination.isEmpty())
      return info.nextStreet;

    final StringBuilder text = new StringBuilder();
    final boolean hasExitInfo = !info.nextJunctionRef.isEmpty() || !info.nextDestinationRef.isEmpty()
        || !info.nextDestination.isEmpty();
    int junctionInsertPos = -1;
    int targetInsertPos = -1;

    // These follow roughly the same rules as GetRoadFullName in routing_session.cpp
    if (info.nextIsLink || hasExitInfo)
    {
      if (!info.nextJunctionRef.isEmpty())
      {
        if (info.nextStreetRoadShields != null && info.nextStreetRoadShields.hasJunctionRoadShields())
        {
          junctionInsertPos = text.length();
          appendShieldText(text, info.nextStreetRoadShields.junctionRoadShields);
        }
        else
          text.append(info.nextJunctionRef);
      }

      if (!info.nextDestinationRef.isEmpty())
      {
        if (text.length() > 0)
          text.append(": ");
        if (info.nextStreetRoadShields != null && info.nextStreetRoadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, info.nextStreetRoadShields.targetRoadShields);
        }
        else
          text.append(info.nextDestinationRef);
      }

      if (!info.nextDestination.isEmpty())
      {
        if (text.length() > 0)
          text.append("\n");
        text.append(String.join(" / ", info.nextDestination.split("\\s*;\\s*")));
      }
      else if (!info.nextName.isEmpty())
      {
        append(text, info.nextName);
      }
    }
    else
    {
      if (!info.nextRef.isEmpty())
      {
        if (info.nextStreetRoadShields != null && info.nextStreetRoadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, info.nextStreetRoadShields.targetRoadShields);
        }
      }
      if (!info.nextName.isEmpty())
      {
        append(text, info.nextName);
      }
    }

    if (text.length() == 0)
      return info.nextStreet;

    final SpannableStringBuilder result = new SpannableStringBuilder(text.toString());

    final float shieldTextSize = textSize * 0.85f;
    if (info.nextStreetRoadShields != null)
    {
      if (info.nextIsLink || hasExitInfo)
      {
        if (!info.nextJunctionRef.isEmpty() && info.nextStreetRoadShields.hasJunctionRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  info.nextStreetRoadShields.junctionRoadShields,
                  shieldTextSize, drawOutline, true, info.isLeftHandTraffic, junctionInsertPos);

        if (!info.nextDestinationRef.isEmpty() && info.nextStreetRoadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  info.nextStreetRoadShields.targetRoadShields,
                  shieldTextSize, drawOutline, false, false, targetInsertPos);
      }
      else
      {
        if (!info.nextRef.isEmpty() && info.nextStreetRoadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  info.nextStreetRoadShields.targetRoadShields,
                  shieldTextSize, drawOutline, false, false, targetInsertPos);
      }
    }

    return result;
  }

  private static void append(@NonNull StringBuilder sb, String string)
  {
    if (sb.length() > 0)
      sb.append(" ");
    sb.append(string);
  }

  private static void appendShieldText(@NonNull StringBuilder sb, @NonNull RoadShield[] shields)
  {
    for (int i = 0; i < shields.length; i++)
    {
      if (i > 0)
        sb.append(" ");
      sb.append(shields[i].text);
      if (shields[i].additionalText != null && !shields[i].additionalText.isEmpty())
        sb.append(" ").append(shields[i].additionalText);
    }
  }

  private static void insertShields(@NonNull RoadShieldSpanCreator roadShieldSpanCreator,
                                    @NonNull SpannableStringBuilder result,
                                    @NonNull RoadShield[] shields, float textSize, boolean drawOutline,
                                    boolean isJunction, boolean isLeftHandTraffic, int startOffset)
  {
    int start = startOffset;

    for (int i = 0; i < shields.length; i++)
    {
      final RoadShieldDrawable shield = new RoadShieldDrawable(shields[i], textSize, drawOutline,
                                                               isJunction, isLeftHandTraffic);
      final CharacterStyle shieldSpan = roadShieldSpanCreator.create(shield);
      final int end = start + shields[i].text.length();

      result.setSpan(shieldSpan, start, end, Spannable.SPAN_EXCLUSIVE_INCLUSIVE);

      int shieldLength = shields[i].text.length();
      if (shields[i].additionalText != null && !shields[i].additionalText.isEmpty())
        shieldLength += 1 + shields[i].additionalText.length();
      if (i < shields.length - 1)
        shieldLength += 1;

      start += shieldLength;
    }
  }

  @NonNull
  private static CharacterStyle createRoadShieldSpan(@NonNull RoadShieldDrawable roadShieldDrawable)
  {
    return new ImageSpan(roadShieldDrawable, ImageSpan.ALIGN_BOTTOM);
  }
}
