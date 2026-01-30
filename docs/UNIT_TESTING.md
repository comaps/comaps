# Unit Testing

This guide covers how to build and run C++ unit tests for the CoMaps codebase. See [INSTALL.md](INSTALL.md) for how to set up the build environment. 

For user/beta testing of app releases, see [TESTING.md](TESTING.md).

## Building Tests

Tests are disabled by default in release builds. To build with tests enabled:

### Debug Build (Recommended for Testing)

```bash
# Debug build (-d) with all tests
./tools/unix/build_omim.sh -d

# Or build specific test target
./tools/unix/build_omim.sh -d indexer_tests
```
By default the binaries will be placed in `../omim-build-debug`

## Running Tests

### Using the Test Runner Script

The `tools/unix/run_tests.sh` script provides a convenient way to run tests:

```bash
# Ensure run_tests.sh is executable
chmod +x tools/unix/run_test.sh
# Run smoke test suite (quick validation)
./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke

# Run full test suite (all tests)
./tools/unix/run_tests.sh -b ../omim-build-debug -s full

# Run tests matching a filter
./tools/unix/run_tests.sh -b ../omim-build-debug -f "Normalize"
```

**Options:**
- `-b <path>` - Path to build directory (default: current directory)
- `-s <suite>` - Test suite: `smoke` or `full` (default: full)
- `-f <regex>` - Filter tests by name using regex
- `-h` - Show help

### Running Individual Test Binaries

You can also run test binaries directly:

```bash
cd ../omim-build-debug

# Run all tests in a binary
./indexer_tests

# Run specific test(s) using filter
./indexer_tests --filter="NormalizeAndSimplifyString"

# Run tests matching a pattern
./search_tests --filter="Ranking"
```

## Test Suites

### Smoke Suite

A quick validation suite covering core functionality:

- `base_tests` - Base utilities (logging, threading, containers)
- `coding_tests` - Serialization, compression, file I/O
- `generator_tests` - Map data generation
- `indexer_tests` - Map data indexing, search string utilities
- `map_tests` - Map business logic
- `mwm_tests` - MWM file format
- `platform_tests` - Platform abstraction layer
- `routing_tests` - Route planning and navigation
- `search_tests` - Search and ranking

### Full Suite

All test binaries found in the build directory (`*_tests`), as of January 2026:

**Core Libraries:**
- `base_tests` - Base utilities (logging, threading, containers)
- `coding_tests` - Serialization, compression, file I/O
- `geometry_tests` - Geometric primitives and algorithms
- `descriptions_tests` - Feature descriptions

**Platform & Storage:**
- `platform_tests` - Platform abstraction layer
- `storage_tests` - Map storage management
- `storage_integration_tests` - Storage integration tests

**Indexer & Data:**
- `indexer_tests` - Map data indexing, search string utilities
- `kml_tests` - KML file handling
- `mwm_tests` - MWM file format
- `mwm_diff_tests` - MWM diff/patch

**Editor:**
- `editor_tests` - OSM editing functionality
- `osm_auth_tests` - OSM authentication

**Graphics:**
- `drape_tests` - Drape rendering engine
- `drape_frontend_tests` - Drape frontend
- `shaders_tests` - Shader compilation - See upstream tracking bug for the test taking too long https://github.com/organicmaps/organicmaps/issues/223
- `style_tests` - Map styles

**Traffic & Transit:**
- `traffic_tests` - Traffic data
- `tracking_tests` - GPS tracking
- `transit_tests` - Public transit
- `transit_experimental_tests` - Experimental transit features
- `world_feed_tests` - Transit world feed
- `world_feed_integration_tests` - See upstream tracking bug for failures: https://github.com/organicmaps/organicmaps/issues/215

**Search:**
- `search_tests` - Search and ranking
- `search_integration_tests` - Search integration
- `search_quality_tests` - Search quality metrics

**Routing:**
- `routing_tests` - Route planning
- `routing_common_tests` - Common routing utilities
- `routing_integration_tests` - See upstream tracking bug for failures: https://github.com/organicmaps/organicmaps/issues/221
- `routing_quality_tests` - Routing quality metrics
- `routing_consistency_tests` - Routing consistency checks

**Generator:**
- `generator_tests` - Map data generation
- `generator_integration_tests` - See upstream tracking bug for failure: https://github.com/organicmaps/organicmaps/issues/225
- `address_parser_tests` - Address parsing

**Map:**
- `map_tests` - Map business logic
- `map_integration_tests` - Map integration tests
- `ge0_tests` - ge0 URL scheme

**3rd Party:**
- `opening_hours_tests` - Opening hours parsing
- `opening_hours_integration_tests` - See upstream tracking bug for failures: https://github.com/organicmaps/organicmaps/issues/219
- `opening_hours_supported_features_tests` - See upstream tracking bug for failures: https://github.com/organicmaps/organicmaps/issues/219
- `bsdiff_tests` - Binary diff

**Tools:**
- `openlr_tests` - OpenLR location referencing
- `poly_borders_tests` - Polygon borders
- `track_analyzing_tests` - Track analysis

## Writing Tests

Tests use a simple macro-based framework defined in `base/macros.hpp`:

```cpp
#include "testing/testing.hpp"

UNIT_TEST(MyTest_BasicFunctionality)
{
  // Arrange
  int a = 1, b = 2;

  // Act
  int result = a + b;

  // Assert
  TEST_EQUAL(result, 3, ());
  TEST(result > 0, ());
  TEST_NOT_EQUAL(result, 0, ());
}
```

**Common Test Macros:**
- `TEST(condition, message)` - Assert condition is true
- `TEST_EQUAL(a, b, message)` - Assert equality
- `TEST_NOT_EQUAL(a, b, message)` - Assert inequality
- `TEST_LESS(a, b, message)` - Assert a < b
- `TEST_GREATER(a, b, message)` - Assert a > b

Tests are organized in `*_tests` directories alongside their corresponding library code:
- `libs/indexer/indexer_tests/`
- `libs/search/search_tests/`
- etc.

## Common Issues

### Configuration-Related Failures

Some tests require data files from the `data/` directory. If you see failures related to missing categories or classifications:

```bash
# Ensure you're running from the repository root or build directory
# that has access to the data files
```

### Qt-Related Test Failures

Tests requiring Qt widgets need a display. On headless systems, simply set:

```bash
QT_QPA_PLATFORM="offscreen"
```

## Examples

```bash
# Quick test of specific test case after making changes
cd ../omim-build-debug && ninja indexer_tests && ./indexer_tests --filter="NormalizeAndSimplifyString"

# Run all search-related tests
./tools/unix/run_tests.sh -b ../omim-build-debug -f "Search|Ranking"

# Smoke test suite
./tools/unix/run_tests.sh -b ../omim-build-debug -s smoke

# Full test suite
./tools/unix/run_tests.sh -b ../omim-build-debug -s full
```
