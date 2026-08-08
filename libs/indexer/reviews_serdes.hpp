/*!
 * Serialization/deserialization of reviews.
 *
 * \section Section layout:
 *
 * 1. version: \c uint16
 * 2. header: \ref HeaderV1
 * 3. feature ids: a sorted sequence of \c uint32
 * 4. reviews: a sequence of tuples representing individual reviews, grouped by feature; within
 *    each group ordered by date, descending:
 *     1. rating: \c uint8
 *     2. opinion index: \c varuint - index in strings
 *     3. author index: \c varuint - index in strings
 *     4. date: \c uint16 - number of days since 1970-01-01
 * 5. average ratings: a sequence of \c uint8 representing average ratings for features with corresponding indices
 * 6. review counts: a sequence of \c uint16 representing numbers of reviews for features with corresponding indices
 * 7. review index: a sequence of \c uint32 representing offsets of start of review blocks within the reviews subsection
 *    for features with corresponding indices
 * 8. strings: a BWT-compressed sequence of <code>string</code>s
 *
 */

#pragma once

#include <cstdint>

#include "coding/serdes_binary_header.hpp"
#include "coding/text_storage.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "defines.hpp"

#include "coding/dd_vector.hpp"
#include "indexer/reviews_model.hpp"

namespace reviews
{
enum class Version : uint16_t
{
  V1 = 1,
  Latest = V1
};

using SubsectionOffset = uint64_t;
using ReviewOffset = uint32_t;
using StringIndex = uint32_t;
using ReviewCount = uint16_t;
using EpochDays = uint16_t;

constexpr StringIndex STRING_INDEX_UNDEFINED = std::numeric_limits<uint32_t>::max();

/*! \brief A section header.
 */
struct HeaderV1
{
  SubsectionOffset featureIdsOffset = 0;
  SubsectionOffset reviewsOffset = 0;
  SubsectionOffset averageRatingsOffset = 0;
  SubsectionOffset reviewCountsOffset = 0;
  SubsectionOffset reviewIndexOffset = 0;
  SubsectionOffset stringsOffset = 0;
  SubsectionOffset eosOffset = 0;  // End of section.

  template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    visitor(featureIdsOffset, "featureIdsOffset");
    visitor(reviewsOffset, "reviewsOffset");
    visitor(averageRatingsOffset, "averageRatingsOffset");
    visitor(reviewCountsOffset, "reviewCountsOffset");
    visitor(reviewIndexOffset, "reviewIndexOffset");
    visitor(stringsOffset, "stringsOffset");
    visitor(eosOffset, "eosOffset");
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    coding::binary::HeaderSerVisitor<Sink> visitor(sink);
    visitor(*this);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    coding::binary::HeaderDesVisitor<Source> visitor(source);
    visitor(*this);
  }
};

namespace internal
{
template <typename Sink>
void SerializeFeaturesIds(Sink & sink, std::vector<FeatureId> const & featureIds)
{
  for (auto const & featureId : featureIds)
    WriteToSink(sink, featureId);
}

inline EpochDays ToEpochDays(std::chrono::year_month_day const & date)
{
  return static_cast<EpochDays>(std::chrono::local_days{date}.time_since_epoch().count());
}

inline std::chrono::year_month_day ToDate(EpochDays const & epochDays)
{
  using namespace std::chrono;
  return year_month_day{local_days{days{epochDays}}};
}

template <typename Sink>
void SerializeFeatureReviews(Sink & sink, uint64_t const startPos, FeatureReviews const & reviews,
                             std::map<std::string, StringIndex> const & stringIndex,
                             std::vector<ReviewOffset> & reviewOffsets)
{
  reviewOffsets.push_back(sink.Pos() - startPos);
  for (Review const & review : reviews.reviews)
  {
    WriteToSink(sink, review.rating);
    WriteVarUint(sink, stringIndex.at(review.opinion));
    WriteVarUint(sink, stringIndex.at(review.author));
    WriteToSink(sink, ToEpochDays(review.date));
  }
}

template <typename Sink>
void SerializeReviews(Sink & sink, std::vector<FeatureReviews> const & reviews, std::vector<std::string> & strings,
                      std::vector<ReviewOffset> & reviewOffsets)
{
  // build the string index
  std::map<std::string, StringIndex> stringIndex;
  for (FeatureReviews const & fr : reviews)
  {
    for (Review const & r : fr.reviews)
    {
      stringIndex[r.author] = STRING_INDEX_UNDEFINED;
      stringIndex[r.opinion] = STRING_INDEX_UNDEFINED;
    }
  }
  for (auto const & s : stringIndex | std::views::keys)
  {
    strings.push_back(s);
    stringIndex[s] = strings.size() - 1;
  }

  auto const startPos = sink.Pos();
  for (FeatureReviews const & fr : reviews)
    SerializeFeatureReviews(sink, startPos, fr, stringIndex, reviewOffsets);
  // extra offset entry to mark the end of reviews subsection as an offset from the start of the subsection
  reviewOffsets.push_back(sink.Pos() - startPos);
}

template <typename Sink>
void SerializeAverageRatings(Sink & sink, std::vector<FeatureReviews> const & reviews)
{
  for (auto const & review : reviews)
    WriteToSink(sink, review.averageRating);
}

template <typename Sink>
void SerializeReviewCounts(Sink & sink, std::vector<FeatureReviews> const & reviews)
{
  for (auto const & review : reviews)
    WriteToSink(sink, static_cast<ReviewCount>(review.reviews.size()));
}

template <typename Sink>
void SerializeReviewIndex(Sink & sink, std::vector<ReviewOffset> const & reviewOffsets)
{
  for (auto const & reviewOffset : reviewOffsets)
    WriteToSink(sink, reviewOffset);
}

template <typename Sink>
void SerializeStrings(Sink & sink, std::vector<std::string> const & strings)
{
  coding::BlockedTextStorageWriter<Sink> writer(sink, 20000 /* blockSize */);
  for (auto const & s : strings)
    writer.Append(s);
}

}  // namespace internal

/*!
 * \brief Writes the reviews section to MWM file represented by \c sink.
 * \tparam Sink the type of sink to write to
 * \param[in,out] sink the sink to write to
 * \param[in] featureIds a sorted vector of MWM feature ids
 * \param[in] reviews a vector of \c FeatureReviews, in the order consistent with \c featureIds
 */
template <typename Sink>
void Serialize(Sink & sink, std::vector<FeatureId> const & featureIds, std::vector<FeatureReviews> const & reviews)
{
  CHECK_EQUAL(featureIds.size(), reviews.size(), ());

  WriteToSink(sink, static_cast<uint16_t>(Version::Latest));

  auto const startPos = sink.Pos();

  HeaderV1 header;
  header.Serialize(sink);

  header.featureIdsOffset = sink.Pos() - startPos;
  internal::SerializeFeaturesIds(sink, featureIds);

  std::vector<std::string> strings;
  std::vector<ReviewOffset> reviewOffsets;
  header.reviewsOffset = sink.Pos() - startPos;
  internal::SerializeReviews(sink, reviews, strings, reviewOffsets);

  header.averageRatingsOffset = sink.Pos() - startPos;
  internal::SerializeAverageRatings(sink, reviews);

  header.reviewCountsOffset = sink.Pos() - startPos;
  internal::SerializeReviewCounts(sink, reviews);

  header.reviewIndexOffset = sink.Pos() - startPos;
  internal::SerializeReviewIndex(sink, reviewOffsets);

  header.stringsOffset = sink.Pos() - startPos;
  internal::SerializeStrings(sink, strings);

  header.eosOffset = sink.Pos() - startPos;
  sink.Seek(startPos);
  header.Serialize(sink);
  sink.Seek(startPos + header.eosOffset);
}

class Deserializer
{
public:
  /*!
   * \brief Reads the reviews for a specified feature.
   * \tparam Reader the type of reader to read from
   * \param reader the reader to read from
   * \param featureId the id of the feature whose reviews should be returned
   * \return the feature reviews, if present
   */
  template <typename Reader>
  std::optional<FeatureReviews> Deserialize(Reader & reader, FeatureId featureId)
  {
    NonOwningReaderSource source(reader);
    auto const version = static_cast<Version>(ReadPrimitiveFromSource<uint16_t>(source));

    auto subReader = reader.CreateSubReader(source.Pos(), source.Size());
    CHECK(subReader, ());
    switch (version)
    {
    case Version::V1: return DeserializeV1(*subReader, featureId);
    default:
      LOG(LWARNING, ("unsupported", REVIEWS_FILE_TAG, "section version", static_cast<uint16_t>(version),
                     "when loading reviews for feature", featureId));
      return {};
    }
  }

private:
  template <typename Reader>
  std::optional<FeatureReviews> DeserializeV1(Reader & reader, FeatureId const featureId)
  {
    InitializeIfNeeded(reader);

    Rating averageRating = 0;
    ReviewOffset reviewsStartOffset = 0;
    ReviewOffset reviewsEndOffset = 0;
    {
      ReaderPtr<Reader> idsSubReader(CreateSubReader(reader, m_header.featureIdsOffset, m_header.reviewsOffset));
      DDVector<FeatureId, ReaderPtr<Reader>> ids(idsSubReader);
      auto const it = std::lower_bound(ids.begin(), ids.end(), featureId);
      if (it == ids.end() || *it != featureId)
        return {};

      auto const d = static_cast<uint32_t>(std::distance(ids.begin(), it));

      ReaderPtr<Reader> avgRatingsSubReader(
          CreateSubReader(reader, m_header.averageRatingsOffset, m_header.reviewCountsOffset));
      DDVector<Rating, ReaderPtr<Reader>> avgRatings(avgRatingsSubReader);
      CHECK_LESS(d, avgRatings.size(), ());
      averageRating = avgRatings[d];

      ReaderPtr<Reader> idxSubReader(CreateSubReader(reader, m_header.reviewIndexOffset, m_header.stringsOffset));
      DDVector<ReviewOffset, ReaderPtr<Reader>> idx(idxSubReader);
      CHECK_LESS(d + 1, idx.size(), ());
      reviewsStartOffset = idx[d];
      reviewsEndOffset = idx[d + 1];
    }

    std::vector<Review> reviews;
    {
      auto stringsSubReader = CreateSubReader(reader, m_header.stringsOffset, m_header.eosOffset);
      auto reviewsSubReader = CreateSubReader(reader, m_header.reviewsOffset + reviewsStartOffset,
                                              m_header.reviewsOffset + reviewsEndOffset);
      NonOwningReaderSource source(*reviewsSubReader);
      while (source.Size() > 0)
      {
        auto const rating = ReadPrimitiveFromSource<Rating>(source);
        auto const opinionIndex = ReadVarUint<StringIndex>(source);
        auto const authorIndex = ReadVarUint<StringIndex>(source);
        auto const date = ReadPrimitiveFromSource<EpochDays>(source);
        auto const opinion = m_stringsReader.ExtractString(*stringsSubReader, opinionIndex);
        auto const author = m_stringsReader.ExtractString(*stringsSubReader, authorIndex);
        reviews.push_back(
            Review{.rating = rating, .opinion = opinion, .author = author, .date = internal::ToDate(date)});
      }
    }

    return FeatureReviews{.averageRating = averageRating, .reviews = reviews};
  }

  template <typename Reader>
  std::unique_ptr<Reader> CreateSubReader(Reader & reader, uint64_t const start, uint64_t const end)
  {
    CHECK(m_initialized, ());
    CHECK_GREATER_OR_EQUAL(end, start, ());
    auto const size = end - start;
    return reader.CreateSubReader(start, size);
  }

  template <typename Reader>
  void InitializeIfNeeded(Reader & reader)
  {
    if (m_initialized)
      return;

    {
      NonOwningReaderSource source(reader);
      m_header.Deserialize(source);
    }

    m_initialized = true;
  }

  bool m_initialized = false;
  HeaderV1 m_header;
  coding::BlockedTextStorageReader m_stringsReader;
};

}  // namespace reviews
