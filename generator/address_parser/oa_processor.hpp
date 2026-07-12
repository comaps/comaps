#pragma once

#include <istream>
#include <string>

namespace oa
{
/// Read an OA TSV stream (lat, lon, house_number, street, postcode, editable)
/// and write per-region .tempaddr files into \a outputPath.
///
/// Region assignment uses the borders in \a bordersPath (the parent directory
/// containing a ``borders/`` subdirectory of .poly files).
void ProcessOaStream(std::istream & is, std::string const & bordersPath,
                     std::string const & outputPath, size_t numThreads);
}  // namespace oa
