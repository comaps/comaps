#import "MWMMapWidgets.h"
#import "EAGLView.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

@interface MWMMapWidgets ()

@property(nonatomic) float visualScale;
@property(nonatomic) CGRect availableArea;
@property(nonatomic) CGFloat top;
@property(nonatomic) CGFloat bottom;
@property(nonatomic) CGFloat leading;
@property(nonatomic) CGFloat trailing;

@end

@implementation MWMMapWidgets
{
  std::unique_ptr<gui::Skin> m_skin;
}

+ (MWMMapWidgets *)widgetsManager
{
  return [MapViewController sharedController].mapView.widgetsManager;
}

- (void)setupWidgets:(Framework::DrapeCreationParams &)p
{
  self.visualScale = p.m_visualScale;
  m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("walking"), p.m_visualScale));
  m_skin->Resize(p.m_surfaceWidth, p.m_surfaceHeight);
  m_skin->ForEach(
      [&p](gui::EWidget widget, gui::Position const & pos) { p.m_widgetsInitInfo[widget] = pos; });
  p.m_widgetsInitInfo[gui::WIDGET_SCALE_FPS_LABEL] = gui::Position(m2::PointF(self.visualScale * 10, self.visualScale * 45), dp::LeftTop);
  
  dispatch_async(dispatch_get_main_queue(), ^{
    [self updateLayout];
  });
}

- (void)resize:(CGSize)size
{
  if (m_skin != nullptr)
    m_skin->Resize(size.width, size.height);

  dispatch_async(dispatch_get_main_queue(), ^{
    [self updateLayout];
  });
}

- (void)updateLayout
{
  if (m_skin == nullptr)
    return;

  auto const visualScale =  GetFramework().GetVisualScale();

  gui::TWidgetsLayoutInfo layout;
  if ([MWMCarPlayService shared].isCarplayActivated)
  {
    auto const height = [[MWMCarPlayService shared] windowHeight] + 1000;
    layout[gui::WIDGET_SCALE_FPS_LABEL] = m2::PointF(0, 0);
    layout[gui::WIDGET_COMPASS] = m2::PointF(18 * visualScale, (height - 66) * visualScale);
    layout[gui::WIDGET_COPYRIGHT] = m2::PointF(0, (height - 24) * visualScale);
    layout[gui::WIDGET_RULER] = m2::PointF(0, (height - 24) * visualScale);
  }
  else
  {
    auto const height = [MapViewController sharedController].mainView.height;
    layout[gui::WIDGET_SCALE_FPS_LABEL] = m2::PointF(self.leading * visualScale, self.top * visualScale);
    layout[gui::WIDGET_COMPASS] = m2::PointF((self.leading + 18) * visualScale,  (height - self.bottom - 66) * visualScale);
    layout[gui::WIDGET_COPYRIGHT] = m2::PointF(self.leading * visualScale, (height - self.bottom - 24) * visualScale);
    layout[gui::WIDGET_RULER] = m2::PointF(self.leading * visualScale, (height - self.bottom - 24) * visualScale);
  }
  GetFramework().SetWidgetLayout(std::move(layout));
}

- (void)updatePaddingForTop:(CGFloat)top bottom:(CGFloat)bottom leading:(CGFloat)leading trailing:(CGFloat)trailing
{
  if (self.top != top || self.bottom != bottom || self.leading != leading || self.trailing != trailing) {
    self.top = top;
    self.bottom = bottom;
    self.leading = leading;
    self.trailing = trailing;
    [self updateLayout];
  }
}

@end
