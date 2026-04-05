package app.organicmaps.bookmarks;

import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.imageview.ShapeableImageView;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.Utils;

import java.util.Objects;

public class BookmarkCategorySettingsFragment extends BaseMwmToolbarFragment
{
  private static final int TEXT_LENGTH_LIMIT = 60;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextInputEditText mEditDescView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextInputEditText mEditCategoryNameView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextInputLayout mEditCategoryNameLayout;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CategoryValidator mInputValidator;

  @NonNull
  private ShapeableImageView mSaveView;


  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    final Bundle args = requireArguments();
    mCategory = Objects.requireNonNull(
        Utils.getParcelable(args, BookmarkCategorySettingsActivity.EXTRA_BOOKMARK_CATEGORY, BookmarkCategory.class));
    mInputValidator = new CategoryValidator();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmark_category_settings, container, false);
    initViews(root);
    return root;
  }

  private void initViews(@NonNull View root)
  {
    mEditCategoryNameView = root.findViewById(R.id.edit_list_name_view);
    mEditCategoryNameLayout = root.findViewById(R.id.edit_list_name_input);
    mEditCategoryNameLayout.setEndIconOnClickListener(v -> clearAndFocus(mEditCategoryNameView));
    mEditCategoryNameView.setText(mCategory.getName());
    InputFilter[] f = {new InputFilter.LengthFilter(TEXT_LENGTH_LIMIT)};
    mEditCategoryNameView.setFilters(f);
    mEditCategoryNameView.requestFocus();
    // Add validator here
    mEditCategoryNameView.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2)
      {}

      @Override
      public void onTextChanged(CharSequence charSequence, int start, int before, int count)
      {
        BookmarkCategorySettingsFragment.this.validateCategoryName(charSequence.toString());
      }

      @Override
      public void afterTextChanged(Editable editable)
      {}
    });
    mEditDescView = root.findViewById(R.id.edit_description);
    mEditDescView.setText(mCategory.getDescription());
    mSaveView = root.findViewById(R.id.save);
    mSaveView.setOnClickListener(v -> onEditDoneClicked());
  }

  private void onEditDoneClicked()
  {
    final String newCategoryName = getEditableCategoryName();

    if (!validateCategoryName(newCategoryName))
      return;

    if (isCategoryNameChanged())
      BookmarkManager.INSTANCE.setCategoryName(mCategory.getId(), newCategoryName);

    if (isCategoryDescChanged())
      BookmarkManager.INSTANCE.setCategoryDescription(mCategory.getId(), getEditableCategoryDesc());

    requireActivity().finish();
  }

  private boolean isCategoryNameChanged()
  {
    String categoryName = getEditableCategoryName();
    return !TextUtils.equals(categoryName, mCategory.getName());
  }

  private boolean validateCategoryName(@Nullable String name)
  {
    if (!isCategoryNameChanged())
    {
      mEditCategoryNameLayout.setError(null);
      return true;
    }

    final String maybeError = mInputValidator.validate(requireActivity(), name);
    mEditCategoryNameLayout.setError(maybeError);
    mEditCategoryNameView.requestFocus();
    return maybeError == null;
  }

  @NonNull
  private String getEditableCategoryName()
  {
    return mEditCategoryNameView.getEditableText().toString().trim();
  }

  @NonNull
  private String getEditableCategoryDesc()
  {
    return mEditDescView.getEditableText().toString().trim();
  }

  private boolean isCategoryDescChanged()
  {
    String categoryDesc = getEditableCategoryDesc();
    return !TextUtils.equals(mCategory.getDescription(), categoryDesc);
  }

  private void clearAndFocus(TextInputEditText textView)
  {
    textView.getEditableText().clear();
    textView.requestFocus();
    InputUtils.showKeyboard(textView);
  }
}
