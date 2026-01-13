package app.organicmaps.editor;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.Editor;
import com.google.android.material.checkbox.MaterialCheckBox;
import com.google.android.material.textview.MaterialTextView;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class CuisineAdapter extends RecyclerView.Adapter<CuisineAdapter.ViewHolder>
{
  private static class Item implements Comparable<Item>
  {
    String cuisineTranslated;
    String cuisineKey;

    public Item(String key, String translation)
    {
      this.cuisineKey = key;
      this.cuisineTranslated = translation;
    }

    @Override
    public int compareTo(@NonNull Item another)
    {
      return cuisineTranslated.compareTo(another.cuisineTranslated);
    }
  }

  private final List<Item> mItems = new ArrayList<>();
  private final Set<String> mInitialSelectedKeys = new HashSet<>();
  private final Set<String> mFinalSelectedKeys = new HashSet<>();

  private String mFilter;

  public CuisineAdapter()
  {
    final String[] keys = Editor.nativeGetCuisines();
    final String[] selectedKeys = Editor.nativeGetSelectedCuisines();
    final String[] translations = Editor.nativeTranslateCuisines(keys);

    Arrays.sort(selectedKeys);
    for (int i = 0; i < keys.length; i++)
    {
      final String key = keys[i];
      mItems.add(new Item(key, translations[i]));

      if (Arrays.binarySearch(selectedKeys, key) >= 0)
        mInitialSelectedKeys.add(key);
    }
    mFinalSelectedKeys.addAll(mInitialSelectedKeys);
  }

  public void setFilter(@NonNull String filter)
  {
    if (filter.equals(mFilter))
      return;
    mFilter = filter;

    final String[] filteredKeys = Editor.nativeFilterCuisinesKeys(filter.trim());
    final String[] filteredValues = Editor.nativeTranslateCuisines(filteredKeys);

    mItems.clear();
    for (int i = 0; i < filteredKeys.length; i++)
      mItems.add(new Item(filteredKeys[i], filteredValues[i]));

    notifyDataSetChanged();
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_cuisine, parent, false));
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  public String[] getCuisines()
  {
    return mFinalSelectedKeys.toArray(new String[mFinalSelectedKeys.size()]);
  }

  public void resetCuisines()
  {
    // hacky?
    mFinalSelectedKeys.clear();
    mFinalSelectedKeys.addAll(mInitialSelectedKeys);
  }

  protected class ViewHolder extends RecyclerView.ViewHolder implements CompoundButton.OnCheckedChangeListener
  {
    final MaterialTextView cuisine;
    final MaterialCheckBox selected;

    public ViewHolder(View itemView)
    {
      super(itemView);
      cuisine = itemView.findViewById(R.id.cuisine);
      selected = itemView.findViewById(R.id.selected);
      selected.setOnCheckedChangeListener(this);
      itemView.setOnClickListener(v -> selected.toggle());
    }

    public void bind(int position)
    {
      final String text = mItems.get(position).cuisineTranslated;
      cuisine.setText(text);
      selected.setOnCheckedChangeListener(null);
      selected.setChecked(mFinalSelectedKeys.contains(mItems.get(position).cuisineKey));
      selected.setOnCheckedChangeListener(this);
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      final String key = mItems.get(getBindingAdapterPosition()).cuisineKey;
      if (isChecked)
        mFinalSelectedKeys.add(key);
      else
        mFinalSelectedKeys.remove(key);
    }
  }
}
