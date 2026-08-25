#include "generator/reviews_section_builder.hpp"

#include "generator/reviews.hpp"

#include "base/logging.hpp"

#include "coding/files_container.hpp"

#include "indexer/reviews_serdes.hpp"

#include "platform/platform.hpp"

#include "defines.hpp"

namespace generator
{

using namespace ::reviews;
using namespace ::generator::reviews;

void BuildReviewsSection(std::string const & reviewsFile, std::string const & mwmFile)
{
  using namespace reviews;

  LOG(LINFO, ("Populating", REVIEWS_FILE_TAG, "section from", reviewsFile));

  std::string const osmIdToFeatureIdFile = mwmFile + OSM2FEATURE_FILE_EXTENSION;

  std::vector<FeatureId> featureIds;
  std::vector<FeatureReviews> reviews;
  try
  {
    LoadReviews(reviewsFile, osmIdToFeatureIdFile, featureIds, reviews);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Error loading reviews from", reviewsFile, e.Msg()));
    return;
  }

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto const writer = cont.GetWriter(REVIEWS_FILE_TAG);
  uint64_t sectionSize = writer->Pos();
  Serialize(*writer, featureIds, reviews);
  sectionSize = writer->Pos() - sectionSize;

  LOG(LINFO, ("Section", REVIEWS_FILE_TAG, "is built in", mwmFile, "Disk size =", sectionSize, "bytes"));
}
}  // namespace generator
