#pragma once

#include <string>

namespace generator
{

/*!
 * \brief Write a reviews section to the specified MWM file.
 * \param reviewsFile a path to reviews JSON file output by the reviews preprocessing tool
 * \param mwmFile a path to the MWM file being updated
 */
void BuildReviewsSection(std::string const & reviewsFile, std::string const & mwmFile);
}  // namespace generator
