#include "oa_processor.hpp"

#include "generator/utils.hpp"

#include "platform/platform.hpp"

#include <gflags/gflags.h>

#include <iostream>

DEFINE_string(data_path, "./data", "Data path with 'borders' folder inside");
DEFINE_string(output_path, "", "Output directory for .tempaddr files");
DEFINE_uint64(threads_count, 0,
              "Desired number of threads. 0 = set automatically");

MAIN_WITH_ERROR_HANDLING([](int argc, char ** argv)
{
  std::string const usage(
      "OpenAddresses preprocessor (C++ stage).  Reads OA TSV from stdin and\n"
      "writes per-region .tempaddr files.  Pipe from the Python preprocessor:\n\n"
      "  python3 openaddresses_preprocessor.py collection.zip \\\n"
      "    | oa_processor_tool --data_path=./data --output_path=/tmp/oa-out\n\n"
      "Sample usage:\n");
  gflags::SetUsageMessage(usage + argv[0] +
                          " --data_path=... --output_path=... < oa_addresses.tsv");

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  size_t const threadsNum = FLAGS_threads_count != 0
                                ? FLAGS_threads_count
                                : GetPlatform().CpuCores();
  CHECK(!FLAGS_output_path.empty(), ("Set output path!"));

  oa::ProcessOaStream(std::cin, FLAGS_data_path, FLAGS_output_path, threadsNum);

  return 0;
})
