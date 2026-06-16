#include "testing/testing.hpp"

#include "routing/routing_options.hpp"

#include <cstdint>
#include <vector>

using namespace routing;

namespace
{
using OptionType = RoutingOptions::OptionType;

class RoutingOptionsTests
{
public:
  RoutingOptionsTests() { m_savedOptions = RoutingOptions::LoadOptionsFromSettings(VehicleType::Car); }

  ~RoutingOptionsTests() { RoutingOptions::SaveOptionsToSettings(m_savedOptions); }

private:
  RoutingOptions m_savedOptions;
};

RoutingOptions CreateOptions(std::vector<RoutingOptions::Option> const & include)
{
  RoutingOptions options;

  for (auto type : include)
    options.Add(type);

  return options;
}

void Checker(std::vector<RoutingOptions::Option> const & include)
{
  RoutingOptions options = CreateOptions(include);

  for (auto type : include)
    TEST(options.Has(type), ());

  auto max = static_cast<OptionType>(RoutingOptions::Option::Max);
  for (uint8_t i = 1; i < max; i <<= 1)
  {
    bool hasInclude = false;
    auto type = static_cast<RoutingOptions::Option>(i);
    for (auto has : include)
      hasInclude |= (type == has);

    if (!hasInclude)
      TEST(!options.Has(static_cast<RoutingOptions::Option>(i)), ());
  }
}

UNIT_TEST(RoutingOptionTest)
{
  Checker({RoutingOptions::Option::Toll, RoutingOptions::Option::Motorway, RoutingOptions::Option::Dirty});
  Checker({RoutingOptions::Option::Toll, RoutingOptions::Option::Dirty});

  Checker({RoutingOptions::Option::Toll, RoutingOptions::Option::Ferry, RoutingOptions::Option::Dirty});

  Checker({RoutingOptions::Option::Dirty});
  Checker({RoutingOptions::Option::Toll});
  Checker({RoutingOptions::Option::Dirty, RoutingOptions::Option::Motorway});
  Checker({});
}

UNIT_CLASS_TEST(RoutingOptionsTests, GetSetTest)
{
  RoutingOptions options =
      CreateOptions({RoutingOptions::Option::Toll, RoutingOptions::Option::Motorway, RoutingOptions::Option::Dirty});

  RoutingOptions::SaveOptionsToSettings(options);
  RoutingOptions fromSettings = RoutingOptions::LoadOptionsFromSettings(VehicleType::Car);

  TEST_EQUAL(options.GetOptions(), fromSettings.GetOptions(), ());
}
}  // namespace
