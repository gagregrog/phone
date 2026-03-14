#pragma once
#include <stdint.h>
#include <string>
#include <vector>

struct PhoneBookHeader {
    std::string name;
    std::string value;
};

struct PhoneBookEntry {
    uint32_t id;
    std::string number;     // dialed number, e.g. "411"
    std::string name;       // human-readable label, e.g. "Kitchen lights"
    std::string url;        // full URL, e.g. "http://192.168.1.50/json/state"
    std::string method;     // "GET", "POST", "PUT", "DELETE"
    std::string body;       // request body (may be empty)
    std::vector<PhoneBookHeader> headers;
};
