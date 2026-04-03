package app.organicmaps.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.BorderAvoidanceMode;
import app.organicmaps.sdk.settings.RoadType;
import com.google.android.material.materialswitch.MaterialSwitch;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_ROAD_TYPES = "road_types";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  @NonNull
  private BorderAvoidanceMode mInitialBorderMode = BorderAvoidanceMode.None;
  @NonNull
  private Set<String> mInitialBorderCountries = Collections.emptySet();

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes();
    mInitialBorderMode = RoutingOptions.getBorderAvoidanceMode();
    mInitialBorderCountries = RoutingOptions.getAvoidedBorderCountries();
    return root;
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
    {
      result.add(RoadType.values()[each]);
    }
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
    {
      savedRoadTypes.add(each.ordinal());
    }
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
  }

  private boolean areSettingsNotChanged()
  {
    Set<RoadType> lastActiveRoadTypes = RoutingOptions.getActiveRoadTypes();
    if (!mRoadTypes.equals(lastActiveRoadTypes))
      return false;
    if (mInitialBorderMode != RoutingOptions.getBorderAvoidanceMode())
      return false;
    if (!mInitialBorderCountries.equals(RoutingOptions.getAvoidedBorderCountries()))
      return false;
    return true;
  }

  @Override
  public boolean onBackPressed()
  {
    if (areSettingsNotChanged())
    {
      requireActivity().setResult(Activity.RESULT_CANCELED);
    }
    else
    {
      requireActivity().setResult(Activity.RESULT_OK);
      RoutingController.get().rebuildLastRoute();
    }

    return super.onBackPressed();
  }

  private void initViews(@NonNull View root)
  {
    MaterialSwitch tollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.Toll));
    CompoundButton.OnCheckedChangeListener tollBtnListener = new ToggleRoutingOptionListener(RoadType.Toll, root);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    MaterialSwitch motorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.Motorway));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.Motorway, root);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    MaterialSwitch ferriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry));
    CompoundButton.OnCheckedChangeListener ferryBtnListener = new ToggleRoutingOptionListener(RoadType.Ferry, root);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    MaterialSwitch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty));
    dirtyRoadsBtn.setEnabled(!RoutingOptions.hasOption(RoadType.Paved) || RoutingOptions.hasOption(RoadType.Dirty));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener = new ToggleRoutingOptionListener(RoadType.Dirty, root);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);

    MaterialSwitch stepsBtn = root.findViewById(R.id.avoid_steps_btn);
    stepsBtn.setChecked(RoutingOptions.hasOption(RoadType.Steps));
    CompoundButton.OnCheckedChangeListener stepsBtnListener = new ToggleRoutingOptionListener(RoadType.Steps, root);
    stepsBtn.setOnCheckedChangeListener(stepsBtnListener);

    MaterialSwitch pavedBtn = root.findViewById(R.id.avoid_paved_roads_btn);
    pavedBtn.setChecked(RoutingOptions.hasOption(RoadType.Paved));
    pavedBtn.setEnabled(!RoutingOptions.hasOption(RoadType.Dirty) || RoutingOptions.hasOption(RoadType.Paved));
    CompoundButton.OnCheckedChangeListener pavedBtnListener = new ToggleRoutingOptionListener(RoadType.Paved, root);
    pavedBtn.setOnCheckedChangeListener(pavedBtnListener);

    View borderRow = root.findViewById(R.id.avoid_border_crossing_row);
    TextView borderSummary = root.findViewById(R.id.avoid_border_crossing_summary);
    updateBorderAvoidanceSummary(borderSummary);
    borderRow.setOnClickListener(v -> showBorderAvoidanceDialog(borderSummary));
  }

  private void updateBorderAvoidanceSummary(@NonNull TextView summaryView)
  {
    BorderAvoidanceMode mode = RoutingOptions.getBorderAvoidanceMode();
    switch (mode)
    {
    case Any: summaryView.setText(R.string.border_avoidance_any); break;
    case NonInternal: summaryView.setText(R.string.border_avoidance_non_internal); break;
    case Specific:
      int count = RoutingOptions.getAvoidedBorderCountries().size();
      if (count > 0)
        summaryView.setText(getResources().getQuantityString(R.plurals.border_avoidance_selected_count, count, count));
      else
        summaryView.setText(R.string.border_avoidance_specific);
      break;
    case None:
    default: summaryView.setText(R.string.border_avoidance_none); break;
    }
  }

  private void showBorderAvoidanceDialog(@NonNull TextView summaryView)
  {
    BorderAvoidanceMode currentMode = RoutingOptions.getBorderAvoidanceMode();
    final BorderAvoidanceMode[] selectedMode = {currentMode};

    CharSequence[] items = {getString(R.string.border_avoidance_none), getString(R.string.border_avoidance_any),
                            getString(R.string.border_avoidance_non_internal),
                            getString(R.string.border_avoidance_specific)};
    int checkedItem = currentMode.ordinal();

    AlertDialog.Builder builder = new AlertDialog.Builder(requireActivity())
                                      .setTitle(R.string.avoid_border_crossing)
                                      .setSingleChoiceItems(items, checkedItem, (dialog, which) -> {
                                        selectedMode[0] = BorderAvoidanceMode.values()[which];
                                      });

    builder.setNeutralButton(R.string.border_avoidance_select_countries, null);
    builder.setPositiveButton(android.R.string.ok, (dialog, which) -> {
      RoutingOptions.setBorderAvoidanceMode(selectedMode[0]);
      updateBorderAvoidanceSummary(summaryView);
      RoutingController.get().rebuildLastRoute();
    });
    builder.setNegativeButton(android.R.string.cancel, null);

    AlertDialog dialog = builder.create();
    dialog.show();

    dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(v -> {
      if (selectedMode[0] != BorderAvoidanceMode.Specific)
        selectedMode[0] = BorderAvoidanceMode.Specific;
      RoutingOptions.setBorderAvoidanceMode(selectedMode[0]);
      dialog.dismiss();
      showBorderCountriesPicker(summaryView);
    });
  }

  private void showBorderCountriesPicker(@NonNull TextView summaryView)
  {
    Set<String> allCountries = RoutingOptions.getTopLevelCountries();
    if (allCountries.isEmpty())
    {
      new AlertDialog.Builder(requireActivity())
          .setTitle(R.string.border_avoidance_select_countries)
          .setMessage("No countries available. Countries list will be available after downloading map data.")
          .setPositiveButton(android.R.string.ok, null)
          .show();
      return;
    }

    Set<String> selectedCountries = RoutingOptions.getAvoidedBorderCountries();

    String[] countryArray = allCountries.toArray(new String[0]);
    java.util.Arrays.sort(countryArray);
    boolean[] checkedItems = new boolean[countryArray.length];
    for (int i = 0; i < countryArray.length; i++)
      checkedItems[i] = selectedCountries.contains(countryArray[i]);

    new AlertDialog.Builder(requireActivity())
        .setTitle(R.string.border_avoidance_select_countries)
        .setMultiChoiceItems(countryArray, checkedItems,
                             (dialog, which, isChecked) -> { checkedItems[which] = isChecked; })
        .setPositiveButton(android.R.string.ok,
                           (dialog, which) -> {
                             Set<String> newSelection = new HashSet<>();
                             for (int i = 0; i < countryArray.length; i++)
                             {
                               if (checkedItems[i])
                                 newSelection.add(countryArray[i]);
                             }
                             RoutingOptions.setAvoidedBorderCountries(newSelection);
                             updateBorderAvoidanceSummary(summaryView);
                             RoutingController.get().rebuildLastRoute();
                           })
        .setNegativeButton(android.R.string.cancel, null)
        .show();
  }

  private static class ToggleRoutingOptionListener implements CompoundButton.OnCheckedChangeListener
  {
    @NonNull
    private final RoadType mRoadType;

    @NonNull
    private final View mRoot;

    private ToggleRoutingOptionListener(@NonNull RoadType roadType, @NonNull View root)
    {
      mRoadType = roadType;
      mRoot = root;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
        RoutingOptions.addOption(mRoadType);
      else
        RoutingOptions.removeOption(mRoadType);

      MaterialSwitch dirtyRoadsBtn = mRoot.findViewById(R.id.avoid_dirty_roads_btn);
      MaterialSwitch pavedBtn = mRoot.findViewById(R.id.avoid_paved_roads_btn);
      if (mRoadType == RoadType.Dirty)
      {
        pavedBtn.setEnabled(!isChecked);
        if (isChecked)
        {
          pavedBtn.setChecked(false);
          dirtyRoadsBtn.setEnabled(true);
        }
      }
      else if (mRoadType == RoadType.Paved)
      {
        dirtyRoadsBtn.setEnabled(!isChecked);
        if (isChecked)
        {
          dirtyRoadsBtn.setChecked(false);
          pavedBtn.setEnabled(true);
        }
      }
    }
  }
}
