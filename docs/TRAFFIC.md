# Traffic

## History

One of the predecessors of CoMaps, MapsWithMe, had support for traffic. For each MWM, a corresponding set of traffic data was available from the server, updated every few minutes. This set contained information known as **MWM coloring**, i.e. traffic information for select segments. The set of segments per MWM appears to have been static, with only traffic information for the segments changing. The traffic status was expressed through **speed groups**. There are several speed groups indicating that the expected speed on the segment is reduced by a certain percentage, including one indicating no disruptions (speed of flowing traffic close to the posted limit). Besides, there is a speed group indicating unknown traffic status, and another one indicating that the segment is temporarily closed.

The routing engine considers traffic information in its cost and time calculations. When traffic information changes, the route is recalculated to reflect any changes in traffic. The rendering engine renders traffic as colored overlays (shades of red/orange/yellow/green, or black for closures) on top of, or next to, segments.

This solution required minimal processing on the app side, at the expense of requiring a server that provided traffic information tailored not only to the MWM format, but to a particular version of each MWM. This also meant that users of outdated MWMs would eventually be left without traffic information once the server, once the server discontinued support for that versions. Users of custom/unofficial MWMs would not have been able to get traffic functionality at all – though this would not have been a major concern for a commercial app, which MapsWithMe initially was.

The server is no longer available (the code appears to have been proprietary anyway), and traffic functionality has been disabled in code, but, as of the CoMaps fork, almost all functionality (save for parts of the iOS UI) was still there and in principle working. Rolling back a few commits and generating fake server responses showed that the code was still workable.

CoMaps Traffic reuses many of these concepts, with only minimal changes: The rendering and routing engines still consume MWM coloring information in much the same form as MapsWithMe did. On the other hand, the way traffic information is acquired (and processed into MWM coloring) has been changed fundamentally – see **Functionality**.

## Functionality

The goal was to keep traffic information in a format that is independent from a particular map version, or even map format, allowing it to be used with any MWM version or even by other navigation apps. This is being achieved by relying on the TraFF data format and API (http://traffxml.org) for acquiring data.

The server runs [TraFF Server](https://gitlab.com/traffxml/traff-server), which is a separate project. Documentation for the installation used by CoMaps can be found at https://codeberg.org/comaps/traffic-server.

### TraFF Format

TraFF is a map-agnostic data format for traffic information. Traffic is assumed to be normal on every segment, unless a **message** indicates otherwise. A message is the unit of information in TraFF, about one or more **events**  at a certain **location** at a given time.

Events are expressed by a set of predefined **event codes** and, optionally, **supplementary information** codes and **quantifiers**.

Locations are expressed by a set of points with WGS84 coordinates:
* `at` for point locations, in conjunction with `from` and/or `to` for directionality information
* `from` and `to` for line locations, optionally with a `via` point for path disambiguation

In addition, locations can carry a set of attributes:
* directionality (unidirectional or bidirectional)
* fuzziness (are the points precise coordinates or approximate)
* road class (corresponding to OSM highway types), road ref, road name
* whether the location refers to a ramp
* cardinal direction, country/territory/town, origin and destination names
* for points, junction names, junction refs and kilometric points

These can be used to display messages with their locations, without having to match them to a map first, as well as for disambiguation when matching a location to map data.

### TraFF API

The TraFF API is based on the concept of a **subscription**. A subscription is essentially a session. The lifespan of a subscription is generally defined by the client, potentially with restrictions imposed by the server. In CoMaps, a subscription is generally created on starting the app and terminated when closing the app, or navigating away from it.

A subscription is anonymous. Upon subscribing, the subscriber receives a random ID from the server with which it identifies itself in any subsequent requests relative to the subscription. When a subscription is terminated by the subscriber or the server, the subscriber can set up a new subscription, which will then receive a new ID. TraFF does not provide for any mechanisms to identify users, or to link different subscriptions together. That said, fingerprinting technologies (IP addresses, HTTP user agent strings, other patterns in HTTP/HTTPS or TCP/IP communication unique to a specific software configuration, areas/road classes requested in the subscription) could still provide insight.

When setting up a subscription, the subscriber sets a **filter** consisting of one or more **filter items**. Each filter item specifies a bounding box of an area the subscriber is interested in, and optionally a minimum road class. The subscriber can change the filter for a subscription at any time while the subscription is active.

The server then delivers traffic information in **feeds** containing any number of messages. The first feed may be sent directly in response to a new subscription or a subscription change. The subscriber may poll for a new feed at any time. The first feed for a subscription contains all the messages matching the filter that the server currently knows about. Subsequent feeds omit messages that have already been transmitted in a previous feed and have not changed since.

On Android, TraFF functionality can also be provided by other apps instead of or in addition to a network server. This requires a companion app providing this functionality. Of particular interest is Qz, which works with a hardware TMC receiver. In areas supported by Qz, this allows for live traffic information without a network connection, and provides information that may not be available from the server. This functionality is currently not available on other platforms.

### From TraFF to MWM coloring

All traffic functionality is opt-in, disabled by default. After enabling live traffic information (optionally changing the default URL to a different one), CoMaps will subscribe to the server, with a filter corresponding to the set of active MWMs. Active MWMs are these currently being displayed, the ones around the current location and, if a route is active, the MWMs used for route calculation. Whenever the set of active MWMs changes, CoMaps will change the subscription to reflect that.

CoMaps then polls the server for traffic messages and places the messages in a queue. It then decodes the messages in the queue, one by one, i.e. it converts the TraFF event and location information into MWM coloring.

Decoding a TraFF location into a set of segments internally uses the router, with some tweaks to the algorithm. Segments are chosen not based on expected travel time, but on how well they match the location. The main cost factor is the length of the segment, with a penalty factor applied based on how well the attributes of the segment match those specified in the location, consistency between candidate segments and possibly more. When decoding the location for a closure event (which may already be in the map if it is a long-term closure), the router may ignore access or oneway restrictions.

This results in one or more MWM colorings per message (multiple MWM colorings occur in messages that cross MWM boundaries, one for each MWM affected). These are eventually merged together. When multiple colorings refer to the same segment in the same MWM, the most serious restriction wins:

* Closures win over anything else.
* Unknown (if it occurs at all) always loses (it is set only if all colorings consistently have it for that segment).
* Otherwise, the speed group indicating the lowest speed wins.

Since decoding a message can be relatively costly (decoding a message can take around 0.5–1 second), we cut some corners to speed this up:
* decoded segments are maintained in a persistent cache and reused between sessions
* when a new message is decoded and its location matches one that has already been decoded, the segments from that location are being reused and only the speed groups are updated
* messages in the queue are prioritized:
  * messages with a start time in the future are decoded after currently active messages, sorted by start time
  * messages whose location matches that of another, already-decoded message, are processed first
  * messages which are closer to the current location or to the viewport location are decoded first

A traffic update is announced to the rendering engine, routing engine and the cache periodically while messages are being decoded (queue is not empty), as well as after the last message in the queue is processed. The update interval has been chosen to maintain a balance between responsiveness and avoiding to overload the renderer and router.

The router ignores traffic information during initial route calculation, to avoid delaying the process. All updates received during that time will be processed after the initial route has been calculated.

### Conditional coloring

One change to the MapsWithMe coloring logic was needed to support start/end times for traffic messages (#2833): introducing conditional coloring.

Originally, coloring was considered valid immediately and until further notice. Conditional coloring has an explicit start/end time. There can be multiple conditions, each with their own coloring, alongside unconditional coloring.

To implement this, a `Condition` class was introduced. It represents a start and/or end time for one or more traffic messages.

`Coloring`, as it was known before, was renamed to `SimpleColoring`. The definition remained the same: `std::map<RoadSegmentId, SpeedGroup>`, i.e. a map of segments and their speed group.

`ConditionalColoring` was added. It is a `std::map<Condition, SimpleColoring>`, i.e. a map of conditions to the coloring that applies during that time.

`Coloring` was redefined to be a `struct` of two members: `SimpleColoring m_unconditional` holds unconditional coloring, i.e. the information that was formerly represented by `Coloring`. `ConditionalColoring m_conditional` holds conditional coloring for that MWM.

Processing logic was updated to deal with the new structure:

* Most of the code that just moves `Coloring` without examining it did not require any changes.
* When assembling coloring from individual TraFF mesages, information from messages without a start or end time is added to unconditional coloring. Information from messages with a start and/or end time is added to conditional coloring. Where two messages have identical start/end times, their colorings are merged under the same condition.
* Copy logic for `Coloring` was adapted to handle both unconditional and conditional coloring.
* The renderer renders conditional coloring that is currently active, along with unconditional coloring. As elsewhere, conflicts are resolved in favor of the most restrictive speed group.
* The routing engine evaluates conditional coloring based on the time at which we are expecting to reach the segment in question. This may mean avoiding traffic situations that are not yet visible on the map, or going through situations on the map (they show because they are currentley active, but are expected to end before we get there).

## Code structure

The `traffic` branch adds two folders to the overall code structure:

* `libs/traffxml`: Everything needed to process data in TraFF format, and talk to a TraFF API:
  * `traff_model`: Classes for the TraFF data model, including CoMaps-specific extensions
  * `traff_model_xml`: XML serialization/deserialization for the classes in `traff_model`
  * `traff_source`: Abstract base class for TraFF data sources (from which TraFF data can be obtained), and implementation of a HTTP traff source
  * `traff_decoder`: Provides TraFF location decoding functionality; this includes an abstract base class (allowing for multiple decoder implementations), an OpenLR-based implementation (experimental and currently commented out) and an implementation using the routing engine
  * `traff_storage`: Implements a persistent cache for TraFF data, including per-message MWM coloring
* `tools/traff_assessment_tool`: a diagnostic tool for TraFF messages, see **TraFF Assessment Tool**

It also introduces a number of modifications to existing code. Most of them concern the traffic manager (which has been mostly rewritten) and the router (re-enable traffic, add logic for conditional traffic information, also allow it to be used for location decoding):

* `libs/drape_frontend`: Add missing element to `kAverageSegmentsCount` (bug fix), update handling of `Coloring`
* `libs/geometry`: In `parametrized_segment`, extend `ClosestPointTo()` with an optional `snapToEnds` argument. If true, it constrains the result to one of the segment endpoints, false (default) maintains existing behavior
* `libs/indexer`: In `ftypes_matcher`, construction types were added to `HighwayClass()`, and a new `IsMotorVehicleAccessRestrictedChecker` was introduced
* `libs/map`:
  * In `framework`, update code for traffic manager initialization (including a newly introduced test mode), provide the configuration API for traffic and a implement a `?clear-traffic` debug command in the search UI
  * `traffic_manager` has been mostly rewritten: It now controls TraFF sources, retrieves messages from them and provides a thread for the TraFF decoder
* `libs/routing`: Update handling of `Coloring`, and introduce changes so the router can be used for TraFF decoding:
  * Update any uses of `Coloring` to work with the redesigned structure (existing logic now uses `Coloring::m_unconditional` instead of `Coloring`; copy conditional coloring along with unconditional coloring)
  * In `index_graph`, pass weight from `IndexGraph::CalculateEdgeWeight()` down to `EdgeEstimator::CalcSegmentWeight()`
  * In `edge_estimator`:
    * Modify `EdgeEstimator::CalcSegmentWeight()` and overrides to accept weight as an additional, optional argument
    * Pass down weight from `CarEstimator::CalcSegmentWeight()` to `TrafficStash::GetSpeedGroup()`
  * In `index_router`, in `IndexRouter::RedressRoute()`, call `TrafficStash::GetSpeedGroup()` with ETA argument
  * In `absent_regions_finder`, change thread synchronization (for TraFF decoding, the router runs on the decoder thread, not on its own thread)
  * In `async_router`, allow running without an absent regions finder (in regular routing, this is used to prompt the user to download additional maps; this is not desired for the traffic decoder, it works with the maps it has)
  * In `directions_engine`, populate `RoadNameInfo::m_onRoundabout` (indicating if the segment is part of a roundabout, needed by the TraFF decoder)
  * In `edge_estimator`:
    * Introduce router modes: `Navigation` for user-initiated routing, `Decoding` for TraFF location decoding
    * Make `CalcOffroad` virtual: traffic uses its own `EdgeEstimator` subclass with its own `CalcOffroad` implementation
    * Introduce a virtual `GetTransitionPenalty() method`: the default implementation returns zero, but the traffic estimator overrides it, adding penalties where road attributes change
    * Introduce a virtual `IsAccessIgnored()` method: the default implementation returns false, but subclasses (traffic) may return true to indicate access restrictions should be ignored (needed when decoding closures which are already on the map)
  * In `fake_ending`: Amend `MakeFakeEnding()` and `CalcProjectionToSegment()` with an optional `snapToEnds` argument (see `libs/geometry`, it is just being passed through here).
  * In `features_road_graph`: Amend `FindClosestEdges()` and `NearestEdgeFinder()` with an optional `snapToEnds` argument (see above)
  * In `index_graph`, implement support for ignoring access and applying transition penalty (see above)
  * In `index_router`:
    * Introduce router modes as in `edge_estimator`
    * Add a constructor for a user-supplied `EdgeEstimator` and no `TrafficCache`, intended for use with the TraFF decoder
    * Introduce a virtual `GetRoutingOptions()` method: the default implementation returns user-configured options, but subclasses (traffic) may change this behavior
    * Introduce a virtual `IsSnapToEndsActive()` function, with a default implementation returning false; traffic overrides this (see above for `snapToEnds`)
    * Make `RoadsToNearestEdges()` non-static (needed so we can use the non-static `m_router.IsSnapToEndsActive()`)
    * Bring `CreateTrafficStash()` back to life (needed for the router to consider traffic information)
    * Prevent the router from returning `RouterResultCode::NeedMoreMaps` when in decoder mode, ensuring it can run without user interaction
    * Modify `PointsOnEdgesSnapping::FindBestEdges()` so it will not filter out edges “fenced off” by other edges when in decoder mode – this is needed on multi-carriageway roads, when the reference point is separated from what would be the correct carriageway by the opposite carriageway (or an auxiliary carriageway)
  * In `nearest_edge_finder`, amend constructor with an optional `snapToEnds` argument and add `M-snapToEnds` member (see above)
  * In `road_graph`, amend `FindClosestEdges()` with an optional `snapToEnds` argument (see above)
  * In `route`, amend `RoadNameInfo` with a `m_onRoundabout` member indicating whether the road is part of a roundabout (needed by traffic)
  * In `routing_session`, add a `GetAllRegions()` method
* `libs_routing_common`:
  * `decoder_model`: Vehicle model for the TraFF decoder; this is already present on `main` as a stub, allowing us to generate maps with TraFF decoder support (including routable construction types), the changes make it fully functional for TraFF location decoding
  * In `vehicle_model`, add support for three-level feature types (e.g. `highway-construction-primary`) in `GetHighwayType()` (falling back to the two-level type if nothing is found)
* `libs/traffic`:
  * Add `iso_time` with a new `IsoTime` class, which parses time in ISO format and keeps it as a timestamp relative to UTC
  * In `speed_groups`, add a function to merge two speed groups referring to the same segment
  * In `traffic_stash`, rewrite `TrafficStash::GetSpeedGroup()` to take an optional ETA argument and, if present, evaluate conditional coloring against it
  * In `traffic_info`, remove obsolete code and rewrite for use with TraFF data sources
  * `traffic_tests` has tests that probably no longer work, but have been left there for the moment, in case some TraFF tests can be derived from the code
* Android:
  * Preferences UI for traffic
  * UI controls for the traffic map layer
  * A TraFF data source relying on Android Binder intents (allowing TraFF information to be retrieved from companion apps)
* iPhone: Settings UI for traffic and UI controls for the traffic data layer (currently broken since introduction of mode UI, see #5166)
* Qt:
  * Expose traffic settings in preferences dialog
  * And add UI controls for traffic layer. **Note:** this already considers the Modes UI, hence the principles of this implementation can also be used for iOS. Right now, traffic can be enabled as a layer in any mode; there has been some discussion if it should be kept that way or moved into car mode
* `generator`: Removed `traffic_generator` and all references to it
* `tools`:
  * `openlr`: Some changes to the OpenLR decoder, allowing it to be used for TraFF decoding. This has worked for a proof of concept, but the router-based decoder was found to be about 10× as fast, so the OpenLR-based decoder has been abandoned and might have been broken by code refactorings (the introduction of `libs` and `tools`) since.
  * `python/maps?generator/generator`: Remove `generate_traffic_keys`, which no longer works, is no longer needed and has not been used since the days of MapsWithMe.
* `xcode`: XCode changes related to new files in the code tree

Additionally, Doxygen comments were added and/or improved in many source files. Most of these have been submitted in advance as PR #5443, except for those that refer to newly introduced code or were difficult to separate from the `traffic` branch.

In `data/test_data`, a `traff` folder has been added with various TraFF feeds for testing purposes.

## TraFF Assessment Tool

The TraFF Assessment Tool is a dignostic tool for TraFF messages. It is available in `tools`.

The tool is loosely based on the OpenLR assessment tool, which appears to have been used by the MapsWithMe developers in conjunction with their traffic functionality. It can load TraFF feeds into a CoMaps GUI and analyze them. Messages are displayed in a list. Clicking a message zooms to its location and displays markers for the reference points, as well as the decoded segments, as an overlay on the map, allowing for a direct comparison between TraFF data and the decoded location.

### Building

Since 17fd87a4, the TraFF assessment tool can be built with `build_omim.sh` in the usual way.

Should you need to build an earlier version, either cherry-pick 17fd87a4 or run the following commands from the CoMaps source dir:

    mkdir build
    cmake . -B build ; cmake --build build --target traff_assessment_tool

### Usage prerequisites

Before using the TraFF Assessent tool, download the MWMs needed with the CoMaps desktop app. (The TraFF Assessment Tool lacks map download functionality so far.)

### Usage

From the CoMaps source dir, run:

    ./build/traff_assessment_tool

From the menu, choose **File > Open sample** to open a TraFF feed. (Some sample TraFF feeds can be found in `data/test_data/traff`, or can be downloaded from the TraFF server. **TODO** document one-shot URL)

The TraFF Assessment Tool does not read from, or write to, the CoMaps traffic cache. It will start on a blank slate, and messages loaded in the tool will not be persisted to cache to avoid overwriting live information with test data.

After the feed has finished loading, you will see its messages in the list. (Cancellation messages, or messages that contain no useful events, will be ignored.) When importing messages into the tool, their timestamps are shifted so that update time is current time, and the durations since the first report, until expiration and, if applicable, until or since start and end, are maintained.

You can now click a message in the list to zoom to examine its location.

TraFF reference points for the selected message are displayed as markers:

* `from` in green
* `via` in orange
* `to` in red
* `at` in blue
* `not_via` in purple (ignored by the decoder)

The decoded segments for the selected message are displayed as a blue overlay on the map. Right-clicking on the map will give you the usual information on map items. Left-clicking currently (October 2026) does nothing.

Individual messages can be removed from the list by selecting them and pressing `Del`, or by using the context menu.

The TraFF Assessment Tool will not delete expired messages automatically. To do so manually, select **File > Purge expired messages** from the menu.

To delete all messages from the cache, select **File > Clear TraFF cache** from the menu.

To save the entire cache of TraFF messages to a file, select **File > Save sample**. To save individual mesages, use the context menu from the list.

## Possible future extensions

### Static traffic information

For people who do not like the idea of polling a server regularly, or do not have permanent network access, it has been requested to support downloading traffic information in static form.

The TraFF server already supports subscriptionless, “one-shot” requests, although this is not officially specified yet as of TraFF 0.8. A one-shot request specifies exactly one filter item (bounding box and, optionally, minimum road class). The server does not assign a subscription ID and responds with a feed of all messages matching the filter.

Since the TraFF server potentially has information on long-term events (depending on its sources), this would give the user the entire set of planned events that the server has at this point. For how long they are valid depends on the sources.

Forecasts of the traffic situation (e.g. regular traffic jams during peak hours) could theoretically also be provided over this channel, although doing so efficiently may require some extensions to the data model and API. The biggest part in this is to obtain/generate a somewhat reliable traffic forecast.

CoMaps currently (`traffic` branch as of October 2026) does not implement functionality to issue this type of request to a TraFF server, though such functionality would be relatively easy to add, see #1252.

### Safety relevant traffic information

CoMaps is currently limited to traffic information that affects travel times, or passability of ways (this limitation is inherited from MapsWithMe). TraFF supports various other event types, notably the `HAZARD` class, which roughly corresponds to safety-relevant traffic information (SRTM). This includes events which have an impact on safety but not necessarily on travel times, such as objects/people/animals on the road, dense fog, ice, aquaplaning and similar. These are currently ignored by CoMaps. Future versions could evaluate those and display an warning to drivers as they approach a segment for which such an event is active.

Doing so would require extensions to the data model, as well as possibly fully developed tracking functionality (determining which road the driver is currently on, even when no route is active).
