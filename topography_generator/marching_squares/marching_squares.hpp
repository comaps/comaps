#pragma once

#include "topography_generator/marching_squares/contours_builder.hpp"
#include "topography_generator/marching_squares/square.hpp"
#include "topography_generator/utils/contours.hpp"
#include "topography_generator/utils/values_provider.hpp"

#include "base/logging.hpp"

namespace topography_generator
{
template <typename ValueType>
class MarchingSquares
{
public:
  MarchingSquares(ms::LatLon const & leftBottom, ms::LatLon const & rightTop,
                  double step, ValueType valueStep, ValuesProvider<ValueType> & valuesProvider,
                  std::string const & debugId)
    : m_leftBottom(leftBottom)
    , m_rightTop(rightTop)
    , m_step(step)
    , m_valueStep(valueStep)
    , m_valuesProvider(valuesProvider)
    , m_debugId(debugId)
  {
    CHECK_GREATER(m_rightTop.m_lon, m_leftBottom.m_lon, ());
    CHECK_GREATER(m_rightTop.m_lat, m_leftBottom.m_lat, ());

    m_stepsCountLon = static_cast<size_t>((m_rightTop.m_lon - m_leftBottom.m_lon) / step);
    m_stepsCountLat = static_cast<size_t>((m_rightTop.m_lat - m_leftBottom.m_lat) / step);

    CHECK_GREATER(m_stepsCountLon, 0, ());
    CHECK_GREATER(m_stepsCountLat, 0, ());
  }

  void GenerateContours(Contours<ValueType> & result)
  {
    std::vector<ValueType> grid((m_stepsCountLat + 1) * (m_stepsCountLon + 1));

    ScanValuesInRect(result, grid);
    result.m_valueStep = m_valueStep;

    auto const levelsCount = static_cast<size_t>(result.m_maxValue - result.m_minValue) / m_valueStep;
    if (levelsCount == 0)
    {
      LOG(LINFO, ("Contours can't be generated: min and max values are equal:", result.m_minValue));
      return;
    }

    ContoursBuilder contoursBuilder(levelsCount, m_debugId);
    Square<ValueType> square(result.m_minValue, m_valueStep, m_debugId);

    for (size_t i = 0; i < m_stepsCountLat; ++i)
    {
      contoursBuilder.BeginLine();
      for (size_t j = 0; j < m_stepsCountLon; ++j)
      {
        // This point should be calculated _exact_ the same way as in ScanValuesInRect.
        // leftBottom + m_step doesn't work due to different floating results.

        square.Init(
            m_leftBottom.m_lon + m_step * j,        // Left
            m_leftBottom.m_lat + m_step * i,        // Bottom
            m_leftBottom.m_lon + m_step * (j + 1),  // Right
            m_leftBottom.m_lat + m_step * (i + 1),  // Top

            grid[Idx(i, j)],          // LB
            grid[Idx(i, j + 1)],      // RB
            grid[Idx(i + 1, j)],      // LT
            grid[Idx(i + 1, j + 1)],  // RT

            m_valuesProvider.GetInvalidValue());

        square.GenerateSegments(contoursBuilder);
      }

      contoursBuilder.EndLine(i == m_stepsCountLat - 1 /* finalLine */);
    }

    contoursBuilder.GetContours(result.m_minValue, result.m_valueStep, result.m_contours);
  }

private:
  size_t Idx(size_t iLat, size_t jLon) const { return iLat * (m_stepsCountLon + 1) + jLon; }

  void ScanValuesInRect(Contours<ValueType> & res, std::vector<ValueType> & grid) const
  {
    res.m_minValue = res.m_maxValue = m_valuesProvider.GetValue(m_leftBottom);
    res.m_invalidValuesCount = 0;

    for (size_t i = 0; i <= m_stepsCountLat; ++i)
    {
      for (size_t j = 0; j <= m_stepsCountLon; ++j)
      {
        ms::LatLon const pos(m_leftBottom.m_lat + m_step * i, m_leftBottom.m_lon + m_step * j);
        auto const value = m_valuesProvider.GetValue(pos);
        grid[Idx(i, j)] = value;

        if (value == m_valuesProvider.GetInvalidValue())
        {
          ++res.m_invalidValuesCount;
          continue;
        }
        if (value < res.m_minValue)
          res.m_minValue = value;
        if (value > res.m_maxValue)
          res.m_maxValue = value;
      }
    }

    if (res.m_invalidValuesCount > 0)
      LOG(LWARNING, ("Tile", m_debugId, "contains", res.m_invalidValuesCount, "invalid values."));

    Square<ValueType>::ToLevelsRange(m_valueStep, res.m_minValue, res.m_maxValue);

    CHECK_GREATER_OR_EQUAL(res.m_maxValue, res.m_minValue, (m_debugId));
  }

  ms::LatLon const m_leftBottom;
  ms::LatLon const m_rightTop;
  double const m_step;
  ValueType const m_valueStep;
  ValuesProvider<ValueType> & m_valuesProvider;

  size_t m_stepsCountLon;
  size_t m_stepsCountLat;

  std::string m_debugId;
};
}  // namespace topography_generator
