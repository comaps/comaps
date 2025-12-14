#pragma once

#include <array>
#include <set>
#include <string>
#include <tuple>
#include <utility>  // for std::pair
#include <vector>

// struct to store the representation of a charging station socket
struct ChargeSocketDescriptor
{
  std::string type;    // https://wiki.openstreetmap.org/wiki/Key:socket:*
                       // e.g. "type1"
  unsigned int count;  // number of sockets; 0 means socket present, but unknown count
                       // (eg, OSM tag for count set to 'yes')
  double power;        // power output, in kW. 0 means unknown.
};

typedef std::vector<ChargeSocketDescriptor> ChargeSocketDescriptors;

typedef std::vector<std::pair<std::string, std::string>> OSMKeyValues;

typedef std::set<std::tuple<std::string, std::string, std::string>> KeyValueDiffSet;

class ChargeSocketsHelper
{
public:
  ChargeSocketsHelper() : m_dirty(false) {}

  ChargeSocketsHelper(ChargeSocketDescriptors const & sockets) : m_chargeSockets(sockets), m_dirty(true) {}

  /** Create a ChargeSocketsHelper instance from an existing list of sockets
   * stored as "<type>|<nb>|[<power>];..."
   *
   * For instance:
   * "type2_combo|2|150;chademo|1|50;type2|4|"
   */
  ChargeSocketsHelper(std::string const & socketsList);

  void AddSocket(std::string const & type, unsigned int count, double power);

  /** Parse OSM attributes for socket types and add them to m_chargeSockets.
   *
   * Examples of (k,v) pairs:
   * ("socket:type2_combo", "2")
   * ("socket:type2_combo:output", "150 kW")
   * ("socket:chademo", "1")
   * ("socket:chademo:output", "50") // assumes kW
   */
  void AggregateChargeSocketKey(std::string const & key, std::string const & value);

  /** Returns the current list of sockets, ordered from the most powerful to the least.
   *
   * The list is guaranteed to be sorted first by socket type (same ordered as
   * SUPPORTED_TYPES), then by power (descending).
   *
   * Note that this method is not const as it may trigger a re-ordering of the
   * internal list of sockets.
   */
  ChargeSocketDescriptors GetSockets();

  /** Serialize the list of sockets into a string with format "<type>|<nb>|[<power>];..."
   *
   * For instance:
   * "type2_combo|2|150;chademo|1|50;type2|4|"
   *
   * The list is guaranteed to be sorted first by socket type (same ordered as
   * SUPPORTED_TYPES), then by power (descending).
   *
   * The same string can be use to re-create a the list of socket by creating a
   * new instance of ChargeSocketsHelper with the
   * ChargeSocketsHelper::ChargeSocketsHelper(std::string) ctor.
   *
   * Note that this method is not const as it may trigger a re-ordering of the
   * internal list of sockets.
   */
  std::string ToString();

  /** Returns a list of OpenStreetMap (key, value) corresponding to the list of sockets.
   *
   * The list is guaranteed to be sorted first by socket type (same ordered as
   * SUPPORTED_TYPES), then by power (descending).
   *
   * Note that this method is not const as it may trigger a re-ordering of the
   * internal list of sockets.
   */

  OSMKeyValues GetOSMKeyValues();

  static constexpr std::string_view UNKNOWN = "unknown";

  /** List of supported sockets, ~ordered from high-power to low-power.
   * This order can be used in the UIs.
   */
  static constexpr std::array<std::string_view, 14> SUPPORTED_TYPES = {
      "mcs",    "type2_combo", "chademo", "nacs",  "type1",  "gb_dc", "chaoji",
      "type3c", "type2_cable", "type2",   "gb_ac", "type3a", "typee", "schuko"};

  /** Return a list of OSM attributes that have changed between the current
   * list of sockets and the provided old list.
   *
   * Returns a set of (key, old_value, new_value) tuples.
   *
   * If old_value="", the attribute is new (ie has been created)
   * If new_value="", the attribute has been deleted
   *
   * Note that this method is not const as it may trigger a re-ordering of the
   * internal list of sockets.
   */
  KeyValueDiffSet Diff(ChargeSocketDescriptors const & oldSockets);

protected:
  ChargeSocketDescriptors m_chargeSockets;

private:
  /** sort sockets: first by type, then by power (descending).
   */
  void Sort();
  bool m_dirty;
};
