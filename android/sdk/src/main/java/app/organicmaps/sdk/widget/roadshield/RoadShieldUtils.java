package app.organicmaps.sdk.widget.roadshield;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;
import android.text.style.ImageSpan;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.routing.RoadInfo;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.roadshield.RoadShield;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;

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
    final RoadInfo nextRoadInfo = info.nextRoadInfo;
    final RoadShieldInfo roadShields = nextRoadInfo.shields;
    final String formattedRoadName = info.getNextRoadName();
    if (nextRoadInfo.name.isEmpty() && nextRoadInfo.ref.isEmpty() && nextRoadInfo.junctionRef.isEmpty()
        && nextRoadInfo.destinationRef.isEmpty() && nextRoadInfo.destination.isEmpty())
      return formattedRoadName;

    final StringBuilder text = new StringBuilder();
    final boolean hasExitInfo = !nextRoadInfo.junctionRef.isEmpty() || !nextRoadInfo.destinationRef.isEmpty()
        || !nextRoadInfo.destination.isEmpty();
    int junctionInsertPos = -1;
    int targetInsertPos = -1;

    // These follow roughly the same rules as GetRoadFullName in routing_session.cpp
    if (nextRoadInfo.isLink || hasExitInfo)
    {
      if (!nextRoadInfo.junctionRef.isEmpty())
      {
        if (roadShields != null && roadShields.hasJunctionRoadShields())
        {
          junctionInsertPos = text.length();
          appendShieldText(text, roadShields.junctionRoadShields);
        }
        else
          text.append(nextRoadInfo.junctionRef);
      }

      if (!nextRoadInfo.destinationRef.isEmpty())
      {
        if (text.length() > 0)
          text.append(": ");
        if (roadShields != null && roadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, roadShields.targetRoadShields);
        }
        else
          text.append(nextRoadInfo.destinationRef);
      }

      if (!nextRoadInfo.destination.isEmpty())
      {
        if (text.length() > 0)
          text.append("\n");
        text.append(String.join(" / ", nextRoadInfo.destination.split("\\s*;\\s*")));
      }
      else if (!nextRoadInfo.name.isEmpty())
      {
        append(text, nextRoadInfo.name);
      }
    }
    else
    {
      if (!nextRoadInfo.ref.isEmpty())
      {
        if (roadShields != null && roadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, roadShields.targetRoadShields);
        }
      }
      if (!nextRoadInfo.name.isEmpty())
      {
        append(text, nextRoadInfo.name);
      }
    }

    if (text.length() == 0)
      return formattedRoadName;

    final SpannableStringBuilder result = new SpannableStringBuilder(text.toString());

    final float shieldTextSize = textSize * 0.85f;
    if (roadShields != null)
    {
      if (nextRoadInfo.isLink || hasExitInfo)
      {
        if (!nextRoadInfo.junctionRef.isEmpty() && roadShields.hasJunctionRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  roadShields.junctionRoadShields,
                  shieldTextSize, drawOutline, true, info.isLeftHandTraffic, junctionInsertPos);

        if (!nextRoadInfo.destinationRef.isEmpty() && roadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  roadShields.targetRoadShields,
                  shieldTextSize, drawOutline, false, false, targetInsertPos);
      }
      else
      {
        if (!nextRoadInfo.ref.isEmpty() && roadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result,
                  roadShields.targetRoadShields,
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
