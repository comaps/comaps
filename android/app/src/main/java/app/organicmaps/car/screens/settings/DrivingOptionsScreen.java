package app.organicmaps.car.screens.settings;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.OnClickListener;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.Toggle;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.BorderAvoidanceMode;
import app.organicmaps.sdk.settings.RoadType;
import java.util.HashMap;
import java.util.Map;

public class DrivingOptionsScreen extends BaseMapScreen
{
  public static final Object DRIVING_OPTIONS_RESULT_CHANGED = 0x1;

  private record DrivingOption(RoadType roadType, @StringRes int text) {}

  private final DrivingOption[] mDrivingOptions = {new DrivingOption(RoadType.Toll, R.string.avoid_tolls),
                                                   new DrivingOption(RoadType.Dirty, R.string.avoid_unpaved),
                                                   new DrivingOption(RoadType.Ferry, R.string.avoid_ferry),
                                                   new DrivingOption(RoadType.Motorway, R.string.avoid_motorways),
                                                   new DrivingOption(RoadType.Steps, R.string.avoid_steps),
                                                   new DrivingOption(RoadType.Paved, R.string.avoid_paved)};

  @NonNull
  private final Map<RoadType, Boolean> mInitialDrivingOptionsState = new HashMap<>();
  @NonNull
  private BorderAvoidanceMode mInitialBorderMode = BorderAvoidanceMode.None;

  public DrivingOptionsScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);

    initDrivingOptionsState();
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createDrivingOptionsListTemplate());
    return builder.build();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    super.onStop(owner);
    boolean changed = false;
    for (final DrivingOption drivingOption : mDrivingOptions)
    {
      if (Boolean.TRUE.equals(mInitialDrivingOptionsState.get(drivingOption.roadType))
          != RoutingOptions.hasOption(drivingOption.roadType))
      {
        changed = true;
        break;
      }
    }
    if (!changed && mInitialBorderMode != RoutingOptions.getBorderAvoidanceMode())
      changed = true;
    if (changed)
      setResult(DRIVING_OPTIONS_RESULT_CHANGED);
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.driving_options_title));
    return builder.build();
  }

  @NonNull
  private ListTemplate createDrivingOptionsListTemplate()
  {
    final ItemList.Builder builder = new ItemList.Builder();
    for (final DrivingOption drivingOption : mDrivingOptions)
      builder.addItem(createDrivingOptionsToggle(drivingOption.roadType, drivingOption.text));
    builder.addItem(createBorderAvoidanceRow());
    return new ListTemplate.Builder().setHeader(createHeader()).setSingleList(builder.build()).build();
  }

  @NonNull
  private Row createDrivingOptionsToggle(RoadType roadType, @StringRes int title)
  {
    final OnClickListener listener = () ->
    {
      if (RoutingOptions.hasOption(roadType))
        RoutingOptions.removeOption(roadType);
      else
        RoutingOptions.addOption(roadType);
      invalidate();
    };
    return Toggle.create(getCarContext(), title, listener, RoutingOptions.hasOption(roadType));
  }

  @NonNull
  private Row createBorderAvoidanceRow()
  {
    final BorderAvoidanceMode currentMode = RoutingOptions.getBorderAvoidanceMode();
    final Row.Builder builder = new Row.Builder();
    builder.setTitle(getCarContext().getString(R.string.avoid_border_crossing));
    builder.addText(getBorderAvoidanceModeLabel(currentMode));
    builder.setOnClickListener(() -> {
      final BorderAvoidanceMode next;
      switch (currentMode)
      {
      case None: next = BorderAvoidanceMode.NonInternal; break;
      case NonInternal: next = BorderAvoidanceMode.Any; break;
      case Any: next = BorderAvoidanceMode.Specific; break;
      case Specific:
      default: next = BorderAvoidanceMode.None; break;
      }
      RoutingOptions.setBorderAvoidanceMode(next);
      invalidate();
    });
    return builder.build();
  }

  @NonNull
  private String getBorderAvoidanceModeLabel(@NonNull BorderAvoidanceMode mode)
  {
    switch (mode)
    {
    case Any: return getCarContext().getString(R.string.border_avoidance_any);
    case NonInternal: return getCarContext().getString(R.string.border_avoidance_non_internal);
    case Specific: return getCarContext().getString(R.string.border_avoidance_specific);
    case None:
    default: return getCarContext().getString(R.string.border_avoidance_none);
    }
  }

  private void initDrivingOptionsState()
  {
    for (final DrivingOption drivingOption : mDrivingOptions)
      mInitialDrivingOptionsState.put(drivingOption.roadType, RoutingOptions.hasOption(drivingOption.roadType));
    mInitialBorderMode = RoutingOptions.getBorderAvoidanceMode();
  }
}
