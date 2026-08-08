package app.organicmaps.routing;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textview.MaterialTextView;

import app.organicmaps.R;
import app.organicmaps.sdk.routing.RouteStepInfo;

public class DirectionsStepViewAdapter extends RecyclerView.Adapter<DirectionsStepViewAdapter.DirectionsStepViewHolder>
{
    private final Context mContext;
    private final RouteStepInfo[] mItems;

    public DirectionsStepViewAdapter(Context context, RouteStepInfo[] routeStepInfo)
    {
        mContext = context;
        mItems = routeStepInfo == null ? new RouteStepInfo[0] : routeStepInfo;
    }

    @NonNull
    @Override
    public DirectionsStepViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
    {
        return new DirectionsStepViewHolder(
                LayoutInflater.from(parent.getContext()).inflate(R.layout.directions_step_item, parent, false));
    }

    @Override
    public void onBindViewHolder(DirectionsStepViewHolder holder, int position)
    {
        RouteStepInfo info = mItems[position];

        int turnRes = info.getTurnDrawableRes();
        if (turnRes > 0)
            holder.mIcon.setImageResource(turnRes);
        else
            holder.mIcon.setImageResource(app.organicmaps.sdk.R.drawable.ic_bookmark_information);

        holder.mTitle.setText(info.formattedDistance.toString(mContext));
        holder.mSubtitle.setText(info.textualInstruction);
    }

    @Override
    public int getItemCount() { return mItems.length; }

    static class DirectionsStepViewHolder extends RecyclerView.ViewHolder
    {
        public ShapeableImageView mIcon;
        public MaterialTextView mTitle, mSubtitle;

        DirectionsStepViewHolder(@NonNull View itemView)
        {
            super(itemView);
            mIcon = itemView.findViewById(R.id.step_icon);
            mTitle = itemView.findViewById(R.id.step_title);
            mSubtitle = itemView.findViewById(R.id.step_subtitle);
        }
    }

}

