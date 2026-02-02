package app.organicmaps.editor;

import android.graphics.Typeface;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.Language;

import com.google.android.material.textview.MaterialTextView;

public class LanguagesAdapter extends RecyclerView.Adapter<LanguagesAdapter.Holder>
{
  public interface OnLanguageSelectedListener
  {
    void onLanguageSelected(Language language);
  }

  @NonNull private final Language[] mLanguages;
  @NonNull private final OnLanguageSelectedListener mListener;
  @Nullable private final Language mPreselectedLanguage;

  public LanguagesAdapter(@NonNull OnLanguageSelectedListener listener, @NonNull Language[] languages,
                          @Nullable Language preselectedLanguage)
  {
    mListener = listener;
    mLanguages = languages;
    mPreselectedLanguage = preselectedLanguage;
  }

  @Override
  public Holder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    return new Holder(LayoutInflater.from(parent.getContext()).inflate(R.layout.item_language, parent, false));
  }

  @Override
  public void onBindViewHolder(Holder holder, int position)
  {
    holder.bind(position);
  }

  @Override
  public int getItemCount()
  {
    return mLanguages.length;
  }

  protected class Holder extends RecyclerView.ViewHolder
  {
    MaterialTextView name;

    public Holder(View itemView)
    {
      super(itemView);
      name = (MaterialTextView) itemView;
      itemView.setOnClickListener(v -> mListener.onLanguageSelected(mLanguages[getBindingAdapterPosition()]));
    }

    public void bind(int position)
    {
      Language language = mLanguages[position];
      name.setText(language.name);
      boolean isSelected = mPreselectedLanguage != null && mPreselectedLanguage.code.equals(language.code);
      name.setTypeface(null, isSelected ? Typeface.BOLD : Typeface.NORMAL);
    }
  }
}
