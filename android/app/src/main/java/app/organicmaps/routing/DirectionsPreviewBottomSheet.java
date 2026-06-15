package app.organicmaps.routing;

import android.app.Dialog;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.divider.MaterialDividerItemDecoration;

import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.Utils;

public class DirectionsPreviewBottomSheet
        extends BottomSheetDialogFragment implements View.OnClickListener
{

    DirectionsStepViewAdapter mDirectionsStepViewAdapter;

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
    {
        View v = inflater.inflate(R.layout.directions_bottom_sheet, container, false);

        RecyclerView directionsStepList = v.findViewById(R.id.directions_step_list);
        LinearLayoutManager layoutManager = new LinearLayoutManager(getContext());
        directionsStepList.setLayoutManager(layoutManager);

        mDirectionsStepViewAdapter = new DirectionsStepViewAdapter(getContext(), Framework.nativeGetRouteSteps(Utils.getLanguageCode()));

        directionsStepList.setAdapter(mDirectionsStepViewAdapter);

        return v;
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
    {
        Dialog dialog = super.onCreateDialog(savedInstanceState);

        dialog.setOnShowListener(dialogInterface -> {
            FrameLayout bottomSheet =
                    ((BottomSheetDialog) dialogInterface).findViewById(com.google.android.material.R.id.design_bottom_sheet);

            if (bottomSheet != null) {
                BottomSheetBehavior<FrameLayout> behavior = BottomSheetBehavior.from(bottomSheet);
                behavior.setState(BottomSheetBehavior.STATE_HALF_EXPANDED);
            }
        });

        dialog.setOnKeyListener((dialog1, keyCode, event) -> {
            if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_UP)
            {
                dismiss();
                return true;
            }

            return false;
        });

        return dialog;
    }

    @Override
    public void onClick(View v)
    {
        // no implementation
    }

}
