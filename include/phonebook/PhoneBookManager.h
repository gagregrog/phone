#pragma once
#include "phonebook/PhoneBookEntry.h"
#include "phonebook/PhoneBookStore.h"
#include <functional>
#include <vector>

class PhoneBookManager {
public:
    PhoneBookManager(PhoneBookStore& store);

    uint32_t add(const PhoneBookEntry& entry);   // returns assigned id
    bool     update(uint32_t id, const PhoneBookEntry& entry);
    bool     remove(uint32_t id);
    void     removeAll();
    const std::vector<PhoneBookEntry>& getAll() const;
    const PhoneBookEntry* findById(uint32_t id) const;
    const PhoneBookEntry* findByNumber(const char* number) const;

    void setOnCall(std::function<void(const PhoneBookEntry&)> cb);
    void setOnBuiltinCall(std::function<void(const PhoneBookEntry&)> cb);
    void setOnNotFound(std::function<void(const char*)> cb);
    void setOnCallWithExtensions(std::function<void(const PhoneBookEntry&)> cb);
    void setOnExtensionNotFound(std::function<void(uint32_t, const char*)> cb);

    void init();                  // load from store
    void dial(const char* number); // lookup + fire callback
    bool dialExtension(uint32_t entryId, const char* ext); // lookup extension, merge, fire callback; returns true if found
    bool hasExtensions(uint32_t id) const;

    // Returns the uniform extension length if all extensions have the same
    // digit count, or 0 if lengths vary or there are no extensions.
    uint8_t extensionLength(uint32_t id) const;

private:
    PhoneBookStore& _store;
    std::vector<PhoneBookEntry> _entries;
    uint32_t _nextId;
    std::function<void(const PhoneBookEntry&)> _onCall;
    std::function<void(const PhoneBookEntry&)> _onBuiltinCall;
    std::function<void(const char*)> _onNotFound;
    std::function<void(const PhoneBookEntry&)> _onCallWithExtensions;
    std::function<void(uint32_t, const char*)> _onExtensionNotFound;

    void save();
};
