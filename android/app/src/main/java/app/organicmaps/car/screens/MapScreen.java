package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.bookmarks.BookmarkCategoriesScreen;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.car.util.SuggestionsHelpers;
import app.organicmaps.car.util.UiHelpers;

public class MapScreen extends BaseMapScreen
{
  public MapScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    SuggestionsHelpers.updateSuggestions(getCarContext());

    final NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setActionStrip(createActionStrip());
    builder.setMapActionStrip(UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer()));
    return builder.build();
  }
  @NonNull
  private ActionStrip createActionStrip()
  {
    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(createIconAction(R.drawable.ic_bookmarks, this::openBookmarks));
    builder.addAction(createIconAction(R.drawable.ic_search, this::openSearch));
    builder.addAction(UiHelpers.createSettingsAction(this, getSurfaceRenderer()));
    return builder.build();
  }

  @NonNull
  private Action createIconAction(int drawableRes, @NonNull Runnable onClick)
  {
    final CarIcon icon = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), drawableRes)).build();
    final Action.Builder builder = new Action.Builder();
    builder.setIcon(icon);
    builder.setOnClickListener(onClick::run);
    return builder.build();
  }

  private void openSearch()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new SearchScreen.Builder(getCarContext(), getSurfaceRenderer()).build());
  }

  private void openBookmarks()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new BookmarkCategoriesScreen(getCarContext(), getSurfaceRenderer()));
  }
}
