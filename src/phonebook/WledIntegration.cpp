#include "phonebook/WledIntegration.h"

static PhoneBookExtension wledExt(const char* ext, const char* name, const std::string& body) {
    PhoneBookExtension x;
    x.ext = ext;
    x.name = name;
    x.body = body;
    return x;
}

void wledApplyDefaults(PhoneBookEntry& entry) {
    entry.url = "http://" + entry.wledHost + ".local/json/state";
    entry.method = "POST";
    entry.body.clear();
    entry.headers.clear();

    entry.extensions.clear();
    entry.extensions.push_back(wledExt("9", "On",  "{\"on\":true}"));
    entry.extensions.push_back(wledExt("0", "Off", "{\"on\":false}"));
    entry.extensions.push_back(wledExt("1", "M1",  "{\"ps\":1}"));
    entry.extensions.push_back(wledExt("2", "M2",  "{\"ps\":2}"));
    entry.extensions.push_back(wledExt("3", "M3",  "{\"ps\":3}"));
    entry.extensions.push_back(wledExt("4", "M4",  "{\"ps\":4}"));
}
