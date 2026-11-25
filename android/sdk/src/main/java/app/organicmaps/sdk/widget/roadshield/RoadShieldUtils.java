package app.organicmaps.sdk.widget.roadshield;

import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.CharacterStyle;
import android.text.style.ImageSpan;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.RoadInfo;
import app.organicmaps.sdk.routing.RouteStepInfo;
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

  public static final class RoadDescription
  {
    final String street;
    @Nullable
    final RoadShieldInfo shields;
    final String name;
    final String ref;
    final String junctionRef;
    final String destinationRef;
    final String destination;
    final boolean isLink;
    final boolean isLeftHandTraffic;

    public RoadDescription(String street, @Nullable RoadShieldInfo shields, String name, String ref,
                           String junctionRef, String destinationRef, String destination, boolean isLink,
                           boolean isLeftHandTraffic)
    {
      this.street = street;
      this.shields = shields;
      this.name = name;
      this.ref = ref;
      this.junctionRef = junctionRef;
      this.destinationRef = destinationRef;
      this.destination = destination;
      this.isLink = isLink;
      this.isLeftHandTraffic = isLeftHandTraffic;
    }

    @NonNull
    public static RoadDescription forNext(@NonNull RoutingInfo info)
    {
      final RoadInfo r = info.nextRoadInfo;
      return new RoadDescription(info.getNextRoadName(), r.shields, r.name, r.ref, r.junctionRef, r.destinationRef,
                                 r.destination, r.isLink, info.isLeftHandTraffic);
    }

    @NonNull
    public static RoadDescription forStep(@NonNull RouteStepInfo step, boolean isLeftHandTraffic)
    {
      final String name = step.toStreetName == null ? "" : step.toStreetName;
      final String street = step.toFormattedRoadName.isEmpty() ? name : step.toFormattedRoadName;
      return new RoadDescription(street, step.toRoadShields, name, step.toRef, step.toJunctionRef,
                                 step.toDestinationRef, step.toDestination, step.toIsLink, isLeftHandTraffic);
    }
  }

  @NonNull
  public static CharSequence composeInstruction(@NonNull RoutingInfo info, float textSize)
  {
    return composeInstruction(RoadShieldUtils::createRoadShieldSpan, RoadDescription.forNext(info), textSize, true,
                              /* multiline */ true);
  }

  @NonNull
  public static CharSequence composeInstruction(@NonNull RoadShieldSpanCreator roadShieldSpanCreator,
                                                @NonNull RoadDescription road, float textSize, boolean drawOutline,
                                                boolean multiline)
  {
    final RoadShieldInfo roadShields = road.shields;
    final String formattedRoadName = road.street;
    if (road.name.isEmpty() && road.ref.isEmpty() && road.junctionRef.isEmpty() && road.destinationRef.isEmpty()
        && road.destination.isEmpty())
      return formattedRoadName;

    final StringBuilder text = new StringBuilder();
    final boolean hasExitInfo =
        !road.junctionRef.isEmpty() || !road.destinationRef.isEmpty() || !road.destination.isEmpty();
    int junctionInsertPos = -1;
    int targetInsertPos = -1;

    // These follow roughly the same rules as GetRoadFullName in routing_session.cpp
    if (road.isLink || hasExitInfo)
    {
      if (!road.junctionRef.isEmpty())
      {
        if (roadShields != null && roadShields.hasJunctionRoadShields())
        {
          junctionInsertPos = text.length();
          appendShieldText(text, roadShields.junctionRoadShields);
        }
        else
          text.append(road.junctionRef);
      }

      if (!road.destinationRef.isEmpty())
      {
        if (text.length() > 0)
          text.append(": ");
        if (roadShields != null && roadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, roadShields.targetRoadShields);
        }
        else
          text.append(road.destinationRef);
      }

      if (!road.destination.isEmpty())
      {
        if (text.length() > 0)
          text.append(multiline ? "\n" : " ");
        text.append(String.join(" / ", road.destination.split("\\s*;\\s*")));
      }
      else if (!road.name.isEmpty())
      {
        append(text, road.name);
      }
    }
    else
    {
      if (!road.ref.isEmpty())
      {
        if (roadShields != null && roadShields.hasTargetRoadShields())
        {
          targetInsertPos = text.length();
          appendShieldText(text, roadShields.targetRoadShields);
        }
      }
      if (!road.name.isEmpty())
      {
        append(text, road.name);
      }
    }

    if (text.length() == 0)
      return formattedRoadName;

    final SpannableStringBuilder result = new SpannableStringBuilder(text.toString());

    final float shieldTextSize = textSize * 0.85f;
    if (roadShields != null)
    {
      if (road.isLink || hasExitInfo)
      {
        if (!road.junctionRef.isEmpty() && roadShields.hasJunctionRoadShields())
          insertShields(roadShieldSpanCreator, result, roadShields.junctionRoadShields, shieldTextSize, drawOutline,
                        true, road.isLeftHandTraffic, junctionInsertPos);

        if (!road.destinationRef.isEmpty() && roadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result, roadShields.targetRoadShields, shieldTextSize, drawOutline,
                        false, false, targetInsertPos);
      }
      else
      {
        if (!road.ref.isEmpty() && roadShields.hasTargetRoadShields())
          insertShields(roadShieldSpanCreator, result, roadShields.targetRoadShields, shieldTextSize, drawOutline,
                        false, false, targetInsertPos);
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
