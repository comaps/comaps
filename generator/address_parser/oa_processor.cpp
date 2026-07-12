#include "oa_processor.hpp"

#include "generator/address_enricher.hpp"
#include "generator/affiliation.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "coding/file_writer.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include "defines.hpp"

#include <atomic>
#include <mutex>
#include <string_view>
#include <vector>

using namespace generator;

namespace
{
bool ParseOaLine(std::string_view line, AddressEnricher::RawEntryBase & e,
                 std::vector<m2::PointD> & points)
{
    // Tab-separated: lat  lon  house_number  street  postcode  editable  source_name  license_url  license_name
    std::string_view fields[9];
    size_t fieldIdx = 0;
    size_t pos = 0;

    while (fieldIdx < 9 && pos <= line.size())
    {
        size_t tab = line.find('\t', pos);
        if (tab == std::string_view::npos)
            tab = line.size();
        fields[fieldIdx++] = line.substr(pos, tab - pos);
        pos = tab + 1;
    }

    if (fieldIdx < 6)
        return false;

    double lat, lon;
    if (!strings::to_double(fields[0], lat) || !strings::to_double(fields[1], lon))
        return false;

    e.m_from = std::string(fields[2]);
    e.m_to = std::string(fields[2]);  // Single-point: from == to
    e.m_street = std::string(fields[3]);
    e.m_postcode = std::string(fields[4]);
    e.m_interpol = feature::InterpolType::None;
    e.m_editable = fields[5] == "1";

    if (fieldIdx > 6 && !fields[6].empty())
        e.m_sourceName = std::string(fields[6]);
    if (fieldIdx > 7 && !fields[7].empty())
        e.m_licenseUrl = std::string(fields[7]);
    if (fieldIdx > 8 && !fields[8].empty())
        e.m_licenseName = std::string(fields[8]);

    ms::LatLon const ll(lat, lon);
    points.push_back(mercator::FromLatLon(ll));

    return true;
}
}  // namespace

namespace oa
{
void ProcessOaStream(std::istream & is, std::string const & bordersPath,
                     std::string const & outputPath, size_t numThreads)
{
    feature::CountriesFilesAffiliation affiliation(bordersPath, true /* haveBordersForWholeWorld */);
    base::ComputationalThreadPool workers(numThreads);

    std::mutex writersMtx;
    std::map<std::string, FileWriter> writers;

    auto getWriter = [&](std::string const & country) -> FileWriter &
    {
        std::lock_guard guard(writersMtx);
        auto res = writers.try_emplace(
            country, base::JoinPath(outputPath, country) + TEMP_ADDR_EXTENSION);
        if (res.second)
        {
            uint8_t const header[2] = {AddressEnricher::kTempAddrMagic,
                                       AddressEnricher::kTempAddrVersion};
            res.first->second.Write(header, sizeof(header));
        }
        return res.first->second;
    };

    std::atomic<size_t> incomplete = 0;
    std::atomic<size_t> total = 0;

    std::string line;
    while (std::getline(is, line))
    {
        ++total;
        workers.SubmitWork([&, copy = std::move(line)]() mutable
        {
            AddressEnricher::RawEntryBase e;
            std::vector<m2::PointD> points;

            if (!ParseOaLine(copy, e, points) || e.m_street.empty())
            {
                ++incomplete;
                return;
            }

            auto const countries = affiliation.GetAffiliations(points);

            std::vector<int64_t> iPoints;
            iPoints.reserve(points.size());
            for (auto const & p : points)
                iPoints.push_back(PointToInt64Obsolete(p, kPointCoordBits));

            for (auto const & country : countries)
            {
                auto & writer = getWriter(country);
                std::lock_guard guard(writersMtx);
                e.Save(writer);
                rw::Write(writer, iPoints);
            }
        });
    }

    workers.WaitingStop();

    LOG(LINFO, ("OA entries:", total.load(), "Incomplete:", incomplete.load()));
}
}  // namespace oa
