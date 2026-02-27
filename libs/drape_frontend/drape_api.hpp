#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/color.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <mutex>
#include <string>
#include <vector>

#include "3party/ankerl/unordered_dense.h"

namespace df
{
/**
 * @brief Geometry and style for a line rendered with Drape.
 */
struct DrapeApiLineData
{
  DrapeApiLineData() = default;

  DrapeApiLineData(std::vector<m2::PointD> const & points, dp::Color const & color) : m_points(points), m_color(color)
  {}

  /**
   * @brief Enables showing the points of the line.
   *
   * Calling this method will set the internal showPoints attribute to true (causing the points of
   * the line to be shown as dots) and the markPoints attribite to the value supplied.
   *
   * @param markPoints Whether to mark the points.
   */
  DrapeApiLineData & ShowPoints(bool markPoints)
  {
    m_showPoints = true;
    m_markPoints = markPoints;
    return *this;
  }

  /**
   * @brief Sets the width for the line in pixels.
   */
  DrapeApiLineData & Width(float width)
  {
    m_width = width;
    return *this;
  }

  DrapeApiLineData & ShowId()
  {
    m_showId = true;
    return *this;
  }

  std::vector<m2::PointD> m_points;
  float m_width = 1.0f;
  dp::Color m_color;

  bool m_showPoints = false;
  bool m_markPoints = false;
  bool m_showId = false;
};

class DrapeApi
{
public:
  using TLines = ankerl::unordered_dense::map<std::string, DrapeApiLineData>;

  DrapeApi() = default;

  void SetDrapeEngine(ref_ptr<DrapeEngine> engine);

  /**
   * @brief Adds a new line.
   *
   * @param id A unique identifier for the line. If a line with the same identifier exists, it will
   * be replaced by the new line.
   * @param data The data (geometry and style) for the new line.
   */
  void AddLine(std::string const & id, DrapeApiLineData const & data);

  /**
   * @brief Removes a line.
   *
   * @param id The id of the line to remove.
   */
  void RemoveLine(std::string const & id);

  /**
   * @brief Removes all lines added with AddLine.
   */
  void Clear();
  void Invalidate();

private:
  DrapeEngineSafePtr m_engine;
  TLines m_lines;
};
}  // namespace df
