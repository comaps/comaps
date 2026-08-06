package app.organicmaps.widget;

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.IdRes;
import androidx.annotation.Nullable;

import com.google.android.material.button.MaterialButtonToggleGroup;

import app.organicmaps.R;

public class TriStateButtonView extends MaterialButtonToggleGroup
{
  private static final int[] TRISTATE_IDS = {R.id.button_false, R.id.button_true, R.id.button_indeterminate};

  public TriStateButtonView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    inflate(context, R.layout.tristate_checkbox, this);
    setSingleSelection(true);
    setSelectionRequired(true);
  }

  public int getState()
  {
    return fromTristate(getCheckedButtonId());
  }

  public void setState(int state)
  {
    check(toTristate(state));
  }

  private @IdRes int toTristate(int state)
  {
    if (state < 0 || state >= TRISTATE_IDS.length)
      throw new IllegalStateException("Unexpected tristate state value: " + state);
    return TRISTATE_IDS[state];
  }

  private int fromTristate(@IdRes int res)
  {
    for (int i = 0; i < TRISTATE_IDS.length; i++)
      if (TRISTATE_IDS[i] == res)
        return i;
    throw new IllegalStateException("Unexpected tristate state res: " + res);
  }
}