#include "location_mark_point.hpp"

#include "drape_frontend/user_marks_provider.hpp"

#include <map>

const std::string kRoleToColor[] = {
    // From
    "BookmarkGreen",

    // At
    "BookmarkBlue",

    // Via
    "BookmarkOrange",

    // NotVia
    "BookmarkPurple",

    // To
    "BookmarkRed"
};

// TODO decide on UserMark::Type to use
LocationMarkPoint::LocationMarkPoint(m2::PointD const & ptOrg) : UserMark(ptOrg, UserMark::Type::ROUTING) {}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> LocationMarkPoint::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "bookmark-default-m"));
  return symbol;
}

df::ColorConstant LocationMarkPoint::GetColorConstant() const
{
  return kRoleToColor[m_role];
}

void LocationMarkPoint::SetRole(Role role)
{
  SetDirty();
  m_role = role;
}
