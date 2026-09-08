#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/color_constants.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <string>

/**
 * @brief A marker for a TraFF location reference point.
 *
 * TraFF location marker point are of type `UserMark::Type::ROUTING` (as functionally they are
 * equivalent to route checkpoints).
 *
 * They render as a place marker with the tip at the specified coordinates. The color expresses
 * the role of the point in the location: `from` in green, `to` in red, `at` in blue, `via` in orange,
 * `notVia` in purple.
 */
class LocationMarkPoint : public UserMark
{
public:
  /**
   * @brief The role of a point in a TraFF location (from, to etc.).
   */
  enum Role : uint8_t
  {
    From,
    At,
    Via,
    NotVia,
    To
  };

  LocationMarkPoint(m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  df::ColorConstant GetColorConstant() const override;
  dp::Anchor GetAnchor() const override { return dp::Bottom; }

  void SetRole(Role role);
  Role const GetRole() const { return m_role; }

private:
  Role m_role;
};
