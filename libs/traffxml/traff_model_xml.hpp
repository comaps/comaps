#pragma once

#include "traffxml/traff_model.hpp"

#include "geometry/rect2d.hpp"

#include "indexer/data_source.hpp"

#include <functional>
#include <string>
#include <vector>

namespace pugi
{
class xml_document;
class xml_node;
}  // namespace pugi

namespace traffxml
{
/**
 * @brief Retrieves a TraFF feed from an XML document.
 *
 * The document must conform loosely to the TraFF specification (currently version 0.8).
 *
 * The name of the root element is not verified, but the `message` elements must be its immediate
 * children.
 *
 * Values which cannot be parsed correctly are skipped.
 *
 * Events whose event type does not match their event class are skipped.
 *
 * Messages, events, locations or points which lack mandatory information are skipped.
 *
 * If children are skipped but the parent remains valid, parsing it will report success.
 *
 * Parsing the feed will report failure if all its messages fail to parse, but not if it has no
 * messages.
 *
 * In addition to the TraFF specification, we also use a custom extension, `mwm_coloring`, which is
 * a child of `message` and holds decoded traffic coloring. In order to parse it, `dataSource` must
 * be specified. If `dataSource` is `nullopt`, coloring will be ignored. It is recommended to pass
 * `dataSource` if, and only if, parsing an XML stream that is expected to contain traffic coloring.
 * This is only expected to occur in cached data. TraFF from external sources is not expected to
 * contain `mwm_coloring` elements and souch information should be ignored in feeds from outside.
 *
 * @note To pass a reference to the framework data source (assuming the `framework` is the framework
 * instance), use `std::cref(framework.GetDataSource())`.
 *
 * @note Custom elements and attributes which are not part of the TraFF specification, other than
 * `mwm_coloring`, are ignored.
 *
 * @param document The XML document from which to retrieve the messages.
 * @param dataSource The data source for coloring, see description.
 * @param feed Receives the TraFF feed.
 * @return `true` on success, `false` on failure.
 */
bool ParseTraff(pugi::xml_document const & document,
                std::optional<std::reference_wrapper<const DataSource>> const & dataSource,
                TraffFeed & feed);

/**
 * @brief Generates XML from a TraFF feed.
 *
 * The resulting document largely conforms to the TraFF specification (currently version 0.8), but
 * may contain custom elements.
 *
 * The root element of the generated XML document is `feed`.
 *
 * @note Currently no custom elements are generated. Future versions may add the location decoded
 * into MWM IDs, feature IDs, directions and segments, along with their speed groups.
 *
 * @param feed The TraFF feed to encode.
 * @param document The XML document in which to store the messages.
 */
void GenerateTraff(TraffFeed const & feed, pugi::xml_document & document);

/**
 * @brief Generates XML from a map of TraFF messages.
 *
 * The resulting document largely conforms to the TraFF specification (currently version 0.8), but
 * may contain custom elements.
 *
 * The root element of the generated XML document is `feed`.
 *
 * @note Currently no custom elements are generated. Future versions may add the location decoded
 * into MWM IDs, feature IDs, directions and segments, along with their speed groups.
 *
 * @param messages A map whose values contain the TraFF messages to encode.
 * @param document The XML document in which to store the messages.
 */
void GenerateTraff(std::map<std::string, traffxml::TraffMessage> const & messages,
                   pugi::xml_document & document);

/**
 * @brief Generates a list of XML `filter` elements from a vector of rects representing bboxes.
 *
 * The resulting string can be placed inside a TraFF XML `filter_list` element or can be passed as
 * an extra to an Android intent.
 *
 * It will have one `filter` element for each element in `bboxRects`, although simplification may
 * be applied to reduce the number of rects (currently not implemented).
 *
 * The `min_road_class` attribute is not used.
 *
 * @param bboxRects The rectangles, each of which represents a bounding box.
 * @return A string of XML `filter` elements.
 */
std::string FiltersToXml(std::vector<m2::RectD> const & bboxRects);

/**
 * @brief Parses the response to a TraFF request.
 *
 * The response must comply with TraFF 0.8. The root element must be `response`.
 *
 * If a parsing error occurs, the response returned will have its `m_status` member set to
 * `ResponseStatus::Invalid`.
 *
 * @param responseXml The response, as a string in XML format.
 * @return The parsed response.
 */
TraffResponse ParseResponse(std::string const & responseXml);
}  // namespace traffxml
