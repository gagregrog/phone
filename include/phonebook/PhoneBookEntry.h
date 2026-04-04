#pragma once
#include <stdint.h>
#include <string>
#include <vector>

struct PhoneBookHeader {
    std::string name;
    std::string value;
};

struct PhoneBookExtension {
    std::string ext;              // extension number, e.g. "1", "2", "32"
    std::string name;             // human-readable label, e.g. "On", "Off", "Red"
    std::string type;             // "http" (default) or "builtin"
    // HTTP fields
    std::string path;             // appended to base URL, e.g. "/json/state"
    std::string method;           // optional override (empty = use base)
    std::string body;             // optional override (empty = use base)
    // Builtin fields
    std::string builtinFunction;  // e.g. "ring_callback"
    std::string pattern;          // ring pattern name, e.g. "us"
    uint16_t cycles = 0;          // ring cycles (0 = infinite)
    uint16_t callbackDelay = 0;   // seconds to wait after hang-up
};

struct PhoneBookEntry {
    uint32_t id;
    std::string number;             // dialed number, e.g. "411"
    std::string name;               // human-readable label, e.g. "Kitchen lights"
    std::string type;               // "http" (default) or "builtin"
    // HTTP fields
    std::string url;                // full URL, e.g. "http://192.168.1.50/json/state"
    std::string method;             // "GET", "POST", "PUT", "DELETE"
    std::string body;               // request body (may be empty)
    std::vector<PhoneBookHeader> headers;
    std::vector<PhoneBookExtension> extensions;
    // Builtin fields
    std::string builtinFunction;    // e.g. "ring_callback"
    std::string pattern;            // ring pattern name, e.g. "us"
    uint16_t cycles = 0;            // ring cycles (0 = infinite)
    uint16_t callbackDelay = 0;     // seconds to wait after hang-up
};
