package app.organicmaps.widget.placepage.sections;

import static android.view.View.INVISIBLE;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.GridLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;

import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.ChargeSocketDescriptor;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.text.DecimalFormat;

public class PlacePageChargeSocketsFragment extends Fragment implements Observer<MapObject>
{
  private GridLayout mGrid;
  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_charge_sockets_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mGrid = view.findViewById(R.id.socket_grid);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject == null)
    {
      return;
    }

    mGrid.removeAllViews();

    ChargeSocketDescriptor[] sockets = Framework.nativeGetActiveObjectChargeSockets();

    LayoutInflater inflater = LayoutInflater.from(requireContext());

    for (ChargeSocketDescriptor socket : sockets)
    {
      View itemView = inflater.inflate(R.layout.item_charge_socket, mGrid, false);

      itemView.setClickable(false);
      itemView.setFocusable(false);

      MaterialTextView type = itemView.findViewById(R.id.socket_type);
      ShapeableImageView icon = itemView.findViewById(R.id.socket_icon);
      MaterialTextView power = itemView.findViewById(R.id.socket_power);
      MaterialTextView count = itemView.findViewById(R.id.socket_count);

      // load SVG icon converted into VectorDrawable in res/drawable
      @SuppressLint("DiscouragedApi")
      int resIconId = getResources().getIdentifier("ic_charge_socket_" + socket.visualType(), "drawable",
                                                   requireContext().getPackageName());
      if (resIconId != 0)
      {
        icon.setImageResource(resIconId);
      }

      @SuppressLint("DiscouragedApi")
      int resTypeId =
          getResources().getIdentifier("charge_socket_" + socket.visualType(), "string", requireContext().getPackageName());
      if (resTypeId != 0)
      {
        type.setText(resTypeId);
      }

      if (socket.power() != 0)
      {
        DecimalFormat df = new DecimalFormat("#.##");
        power.setText(getString(R.string.kw_label, df.format(socket.power())));
      }
      else if (socket.ignorePower()) {
        power.setVisibility(INVISIBLE);
      }

      if (socket.count() != 0)
      {
        count.setText(getString(R.string.count_label, socket.count()));
      }

      mGrid.addView(itemView);
    }
  }
}
