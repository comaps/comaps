#pragma once

#include <string_view>

// Keys that should be removed when a place in OSM is replaced, copied from
// https://github.com/mnalis/StreetComplete-taginfo-categorize/blob/master/sc_to_remove.txt

// Changes to the list: don't remove 'wheelchair' and addresses in the 'contact:' style

inline constexpr std::string_view kKeysToRemove[] = {
    "shop_?[1-9]?(:.*)?", "craft_?[1-9]?", "amenity_?[1-9]?", "club_?[1-9]?", "old_amenity",
    "old_shop", "information", "leisure", "office_?[1-9]?", "tourism",
    // popular shop=* / craft=* subkeys
    "marketplace", "household", "swimming_pool", "laundry", "golf", "sports", "ice_cream",
    "scooter", "music", "retail", "yes", "ticket", "newsagent", "lighting", "truck", "car_repair",
    "car_parts", "video", "fuel", "farm", "car", "tractor", "hgv", "ski", "sculptor",
    "hearing_aids", "surf", "photo", "boat", "gas", "kitchen", "anime", "builder", "hairdresser",
    "security", "bakery", "bakehouse", "fishing", "doors", "kiosk", "market", "bathroom", "lamps",
    "vacant", "insurance(:.*)?", "caravan", "gift", "bicycle", "bicycle_rental", "insulation",
    "communication", "mall", "model", "empty", "wood", "hunting", "motorcycle", "trailer",
    "camera", "water", "fireplace", "outdoor", "blacksmith", "electronics", "fan", "piercing",
    "stationery", "sensory_friendly(:.*)?", "street_vendor", "sells(:.*)?", "safety_equipment",
    // obsoleted information
    "(demolished|abandoned|disused)(:(?!bui).+)?", "was:.*", "not:.*", "damage", "created_by",
    "check_date", "opening_date", "last_checked", "checked_exists:date", "pharmacy_survey",
    "old_ref", "update", "import_uuid", "review", "fixme:atp",
    // classifications / links to external databases
    "fhrs:.*", "old_fhrs:.*", "fvst:.*", "ncat", "nat_ref", "gnis:.*", "winkelnummer",
    "type:FR:FINESS", "type:FR:APE", "kvl_hro:amenity", "ref:DK:cvr(:.*)?", "certifications?",
    "transiscope", "opendata:type", "local_ref", "official_ref",
    // names and identifications
    "name_?[1-9]?(:.*)?", ".*_name_?[1-9]?(:.*)?", "noname", "branch(:.*)?", "brand(:.*)?",
    "not:brand(:.*)?", "network(:.*)?", "operator(:.*)?", "operator_type", "ref", "ref:vatin",
    "designation", "SEP:CLAVEESC", "identifier", "ref:FR:SIRET", "ref:FR:SIREN", "ref:FR:NAF",
    "(old_)?ref:FR:prix-carburants",
    // contacts
    "contact_person", "phone(:.*)?", "phone_?[1-9]?", "emergency:phone", "emergency_telephone_code",
    "contact:(?!housenumber$|street$|place$|postcode$|city$|country$|pobox$|unit$).*",
    "mobile", "fax", "facebook", "instagram", "twitter", "youtube", "telegram", "tiktok", "email",
    "website_?[1-9]?(:.*)?", "app:.*", "ownership",
    "url", "url:official", "source_ref:url", "owner",
    // payments
    "payment(:.*)?", "payment_multi_fee", "currency(:.*)?", "cash_withdrawal(:.*)?", "fee",
    "charge", "charge_fee", "money_transfer", "donation:compensation", "paypoint",
    // generic shop/craft attributes
    "seasonal", "time", "opening_hours(:.*)?", "check_(in|out)", "wifi", "internet",
    "internet_access(:.*)?", "second_hand", "self_service", "automated", "license:.*",
    "bulk_purchase", ".*:covid19", "language:.*", "baby_feeding", "description(:.*)?",
    "description[0-9]", "min_age", "max_age", "supermarket(:.*)?", "social_facility(:.*)?",
    "functional", "trade", "wholesale", "sale", "smoking(:outside)?", "zero_waste", "origin",
    "attraction", "strapline", "dog", "showroom", "toilets?(:.*)?", "sanitary_dump_station",
    "changing_table(:.*)?", "blind", "company(:.*)?", "stroller", "walk-in",
    "webshop", "operational_status.*", "status", "drive_through", "surveillance(:.*)?",
    "outdoor_seating", "indoor_seating", "colour", "access_simple", "floor", "product_category",
    "guide", "source_url", "category", "kids_area", "kids_area:indoor", "resort", "since", "state",
    "temporary", "self_checkout", "audio_loop", "related_law(:.*)?", "official_status(:.*)?",
    // food and drink details
    "bar", "cafe", "coffee", "microroasting", "microbrewery", "brewery", "real_ale", "taproom",
    "training", "distillery", "drink(:.*)?", "cocktails", "alcohol", "wine([:_].*)?",
    "happy_hours", "diet:.*", "cuisine", "ethnic", "tasting", "breakfast", "lunch", "organic",
    "produced_on_site", "restaurant", "food", "pastry", "pastry_shop", "product", "produce",
    "chocolate", "fair_trade", "butcher", "reservation(:.*)?", "takeaway(:.*)?", "delivery(:.*)?",
    "caterer", "real_fire", "flour_fortified", "highchair", "fast_food", "pub", "snack",
    "confectionery", "drinking_water:refill",
    // related to repair shops/crafts
    "service(:.*)?", "motorcycle:.*", "repair", ".*:repair", "electronics_repair(:.*)?",
    "workshop",
    // shop=hairdresser, shop=clothes
    "unisex", "male", "female", "gender", "gender_simple", "lgbtq(:.*)?", "gay", "female:signed",
    "male:signed",
    // healthcare
    "healthcare(:.*)?", "healthcare_.*", "health", "health_.*", "speciality", "medical_.*",
    "emergency_ward", "facility(:.*)?", "activities", "healthcare_facility(:.*)?",
    "laboratory(:.*)?", "blood(:.*)?", "blood_components", "infection(:.*)?", "disease(:.*)?",
    "covid19(:.*)?", "COVID_.*", "CovidVaccineCenterId", "coronaquarantine", "hospital(:.*)?",
    "hospital_type_id", "emergency_room", "sample_collection(:.*)?", "bed_count", "capacity:beds",
    "part_time_beds", "personnel:count", "staff_count(:.*)?", "admin_staff", "doctors",
    "doctors_num", "nurses_num", "counselling_type", "testing_centres", "toilets_number",
    "urgent_care", "vaccination", "clinic", "hospital", "pharmacy", "alternative", "laboratory",
    "sample_collection", "provided_for(:.*)?", "social_facility_for", "ambulance", "ward",
    "HSE_(code|hgid|hgroup|region)", "collection_centre", "design", "AUTORIZATIE", "reg_id",
    "post_addr", "scope", "ESTADO", "NIVSOCIO", "NO", "EMP_EST", "COD_HAB", "CLA_PERS", "CLA_PRES",
    "snis_code:.*", "hfac_bed", "hfac_type", "nature", "moph_code", "IJSN:.*", "massgis:id",
    "OGD-Stmk:.*", "paho:.*", "panchayath", "pbf_contract", "pcode", "pe:minsa:.*", "who:.*",
    "pharmacy:category", "tactile_paving", "HF_(ID|TYPE|N_EN)", "RoadConn", "bin", "hiv(:.*)?",
    // accommodation & layout
    "rooms", "stars", "accommodation", "beds", "capacity(:persons)?", "laundry_service",
    "guest_house",
    // amenity=place_of_worship
    "deanery", "subject:(wikidata|wikipedia|wikimedia_commons)", "church", "church:type",
    // schools
    "capacity:(pupils|teachers)", "grades", "population:pupils(:.*)?",
    "school:(FR|gender|trust|type|type_idn|group:type)", "primary",
    // clubs
    "animal(_breeding|_training)?", "billiards(:.*)?", "board_game", "sport_1", "sport:boating",
    "boat:type", "canoe(_rental|:service)?", "kayak(_rental|:service)?",
    "sailboat(_rental|:service)?", "horse_riding", "rugby", "boules", "callsign", "card_games",
    "car_service", "catastro:ref", "chess(:.*)?", "children", "climbing(:.*)?", "club(:.*)?",
    "communication(:amateur_radio.*)", "community_centre:for", "dffr:network", "dormitory",
    "education_for:ages", "electrified", "esperanto", "events_venue", "family", "federation",
    "free_flying(:.*)?", "freemasonry(:.*)?", "free_refill", "gaelic_games(:.*)?", "membership",
    "military_service", "model_aerodrome(:.*)?", "mode_of_organisation(:.*)?", "snowmobile",
    "social_centre(:for)?", "source_dat", "tennis", "old_website", "organisation", "school_type",
    "scout(:type)?", "fraternity", "live_music", "lockable", "playground(:theme)?", "nudism",
    "music_genre", "length", "fire_station:type:FR", "cadet", "observatory:type", "tower:type",
    "zoo", "shooting", "commons", "groomer", "group_only", "hazard", "identity", "interaction",
    "logo", "maxheight", "provides", "regional", "scale", "site", "plots", "allotments",
    "local_food", "monitoring:pedestrian", "recording:automated", "yacht", "background_music",
    "url:spaceapi", "openfire", "fraternity(:.*)?",
    // misc specific attributes
    "clothes", "shoes", "tailor", "beauty", "tobacco", "carpenter", "furniture", "lottery",
    "sport", "dispensing", "tailor:.*", "gambling", "material", "raw_material", "stonemason",
    "studio", "scuba_diving(:.*)?", "polling_station", "collector", "books", "agrarian",
    "musical_instrument", "massage", "parts", "post_office(:.*)?", "religion", "denomination",
    "rental", ".*:rental", "tickets:.*", "public_transport", "goods_supply", "pet", "appliance",
    "artwork_type", "charity", "company", "crop", "dry_cleaning", "factory", "feature",
    "air_conditioning", "atm", "vending", "vending_machine", "recycling_type", "museum",
    "license_classes", "dance:.*", "isced:level", "school", "preschool", "university",
    "research_institution", "research", "member_of", "topic", "townhall:type", "parish", "police",
    "government", "thw:(lv|rb|ltg)", "office", "administration", "administrative", "association",
    "transport", "utility", "consulting", "Commercial", "commercial", "private", "taxi",
    "admin_level", "official_status", "target", "liaison", "diplomatic(:.*)?", "embassy",
    "consulate", "aeroway", "department", "faculty", "aerospace:product", "boundary", "population",
    "diocese", "depot", "cargo", "function", "game", "party", "political_party.*",
    "telecom(munication)?", "service_times", "kitchen:facilities", "it:(type|sales)",
    "cannabis:cbd", "bath:type", "bath:(open_air|sand_bath)", "animal_boarding", "animal_shelter",
    "mattress", "screen", "monitoring:weather", "public", "theatre", "culture", "library",
    "cooperative(:.*)?", "winery", "curtain", "lawyer(:.*)?", "local_authority(:.*)?", "equipment",
    "hackerspace",
    "camp_site", "camping", "bbq", "static_caravans", "emergency(:.*)?", "evacuation_cent(er|re)",
    "education", "engineering", "forestry", "foundation", "lawyer", "logistics", "military",
    "community_centre", "bank", "operational", "users_(PLWD|boy|elderly|female|girl|men)",
    "Comments?", "comments?", "entrance:(width|step_count|kerb:height)", "fenced", "motor_vehicle",
    "shelter",
};
