package app.organicmaps.sdk;

import android.annotation.SuppressLint;
import android.graphics.Rect;
import android.os.Bundle;
import android.view.accessibility.AccessibilityNodeInfo;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.customview.widget.ExploreByTouchHelper;
import java.util.List;

// can't be an inner class due to JNI on frame ([I,[I,[I)V
class MapViewAccessibilityDelegate extends ExploreByTouchHelper implements Map.AccessibilityUpdateCallback
{
  private final int MAP_ID = 0;
  private final MapView mHost;
  private final Map mMap;
  private final Framework.AccessibilityNodeContext mNodeCache = new Framework.AccessibilityNodeContext();
  private boolean mExplorationEnabled;

  /**
   * Constructs a new helper that can expose a virtual view hierarchy for the
   * specified host view.
   *
   * @param host view whose virtual view hierarchy is exposed by this helper
   */
  public MapViewAccessibilityDelegate(@NonNull MapView host, @NonNull Map map)
  {
    super(host);
    mHost = host;
    mMap = map;
  }

  @Override
  protected int getVirtualViewAt(float x, float y)
  {
    if (!mExplorationEnabled)
      return HOST_ID;
    int id = Map.getAccessibilityNodeAtPoint(x, y);
    if (id == 0)
      return HOST_ID;
    return id;
  }

  @Override
  protected void getVisibleVirtualViews(List<Integer> virtualViewIds)
  {
    if (!mExplorationEnabled)
      return;
    virtualViewIds.add(MAP_ID);
    for (int node : Map.getAllAccessibilityNodes())
      virtualViewIds.add(node);
  }

  @Override
  protected void onPopulateNodeForHost(@NonNull AccessibilityNodeInfoCompat node)
  {
    if (!mExplorationEnabled)
      onPopulateNodeForMap(node);
    // otherwise, we don't want to be overly verbose when exploring the backdrop of the map
  }

  private void onPopulateNodeForMap(@NonNull AccessibilityNodeInfoCompat node)
  {
    // needed so we can explicitly ignore clicks
    node.addAction(AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_CLICK);
    node.setClickable(false);
    if (!mExplorationEnabled)
    {
      node.setContentDescription(
          mHost.getContext().getString(R.string.accessibility_map_content_description_collapsed));
      AccessibilityNodeInfoCompat.AccessibilityActionCompat actionExpand =
          new AccessibilityNodeInfoCompat.AccessibilityActionCompat(
              AccessibilityNodeInfoCompat.ACTION_EXPAND,
              mHost.getContext().getString(R.string.accessibility_map_action_expand_label));
      node.addAction(actionExpand);
    }
    else
    {
      node.setContentDescription(mHost.getContext().getString(R.string.accessibility_map_content_description_expanded));
      AccessibilityNodeInfoCompat.AccessibilityActionCompat actionCollapse =
          new AccessibilityNodeInfoCompat.AccessibilityActionCompat(
              AccessibilityNodeInfoCompat.ACTION_COLLAPSE,
              mHost.getContext().getString(R.string.accessibility_map_action_collapse_label));
      node.addAction(actionCollapse);
    }
  }

  @Override
  protected void onPopulateNodeForVirtualView(int virtualViewId, @NonNull AccessibilityNodeInfoCompat node)
  {
    if (virtualViewId == MAP_ID)
    {
      onPopulateNodeForMap(node);
      Rect r = new Rect(0, 0, mHost.getWidth(), mHost.getHeight());
      this.setBoundsInScreenFromBoundsInParent(node, r);
      return;
    }
    if (!mExplorationEnabled)
    {
      // ipc race, return an invisible node which will be pruned
      node.setBoundsInScreen(new Rect(-1, -1, -1, -1));
      node.setText("");
      return;
    }
    // Using ACTION_COLLAPSE makes the app unusable because it reads out "expanded" before every single node
    AccessibilityNodeInfoCompat.AccessibilityActionCompat actionCollapse =
        new AccessibilityNodeInfoCompat.AccessibilityActionCompat(
            AccessibilityNodeInfoCompat.ACTION_DISMISS,
            mHost.getContext().getString(R.string.accessibility_map_action_collapse_label));
    // TODO more actions like homing and following

    Map.getAccessibilityNode(virtualViewId, mNodeCache);
    node.setText(mNodeCache.accessibilityLabel);
    // TODO check whether the insets are actually being passed to drape, maybe we are applying them twice (ScreenBase)
    this.setBoundsInScreenFromBoundsInParent(
        node, new Rect((int) mNodeCache.left, (int) mNodeCache.top, (int) mNodeCache.right, (int) mNodeCache.bottom));
    // TODO set any other useful info, e.g. mark as image where relevant
    node.addAction(actionCollapse);
    node.setClickable(true);
  }

  private boolean onPerformActionForMap(int action, @Nullable Bundle arguments)
  {
    switch (action)
    {
    case AccessibilityNodeInfoCompat.ACTION_EXPAND: setExplorationEnabled(true); return true;
    case AccessibilityNodeInfoCompat.ACTION_COLLAPSE: setExplorationEnabled(false); return true;
    case AccessibilityNodeInfo.ACTION_CLICK:
      // suppress clicks on host
      return true;
    }
    return false;
  }

  // delegated to by MapView
  public boolean onPerformActionForHost(int action, @Nullable Bundle arguments)
  {
    return onPerformActionForMap(action, arguments);
  }

  @Override
  protected boolean onPerformActionForVirtualView(int virtualViewId, int action, @Nullable Bundle arguments)
  {
    if (virtualViewId == MAP_ID)
      return onPerformActionForMap(action, arguments);
    if (!mExplorationEnabled)
      // gobble everything
      return true;

    switch (action)
    {
    case AccessibilityNodeInfoCompat.ACTION_EXPAND: setExplorationEnabled(true); return true;
    case AccessibilityNodeInfoCompat.ACTION_COLLAPSE, AccessibilityNodeInfoCompat.ACTION_DISMISS:
      setExplorationEnabled(false);
      return true;
    case AccessibilityNodeInfo.ACTION_SELECT, AccessibilityNodeInfo.ACTION_CLICK: mHost.performClick(); return true;
    }
    // TODO
    return false;
  }

  @SuppressLint("AccessibilityFocus")
  /* moving focus to avoid leaving it on a deleted element, as a result of user request */
  private void setExplorationEnabled(boolean enabled)
  {
    mExplorationEnabled = enabled;
    mMap.setAccessibilityUpdateCallback(this);
    invalidateRoot();
    // keep focus as expected when node structure changes
    if (enabled)
      getAccessibilityNodeProvider(mHost).performAction(MAP_ID, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS,
                                                        null);
    else
      mHost.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null);
  }

  @Override
  public void frame(int[] removed, int[] updated, int[] added)
  {
    if (!mExplorationEnabled)
      return;
    // we must update the root if nodes added or removed
    // also update the root if there are many changed nodes, otherwise we get rate limited on binder transactions
    if (removed.length > 0 || added.length > 0 || updated.length > 5)
      invalidateRoot();
    else
      for (int id : updated)
      {
        invalidateVirtualView(id);
      }
  }
}
