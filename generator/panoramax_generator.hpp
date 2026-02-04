#pragma once

#include "generator/feature_builder.hpp"

#include <functional>
#include <string>

namespace generator
{
// Generates Panoramax imagery point features from binary files.
// Binary files are created by the panoramax_preprocessor.py script.
class PanoramaxFeaturesGenerator
{
public:
  explicit PanoramaxFeaturesGenerator(std::string const & panoramaxDir);

  using FeaturesCollectFn = std::function<void(feature::FeatureBuilder && fb)>;
  void GeneratePanoramax(std::string const & countryName, FeaturesCollectFn const & fn) const;

private:
  std::string m_panoramaxDir;
  uint32_t m_panoramaxType;   // Classificator type for panoramax|image
  uint32_t m_sequenceType;    // Classificator type for panoramax|sequence (line)
};
}  // namespace generator
