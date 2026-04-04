#include "phonebook/PhoneBookManager.h"

PhoneBookManager::PhoneBookManager(PhoneBookStore& store)
    : _store(store), _nextId(1) {}

void PhoneBookManager::setOnCall(std::function<void(const PhoneBookEntry&)> cb) {
    _onCall = std::move(cb);
}

void PhoneBookManager::setOnBuiltinCall(std::function<void(const PhoneBookEntry&)> cb) {
    _onBuiltinCall = std::move(cb);
}

void PhoneBookManager::setOnNotFound(std::function<void(const char*)> cb) {
    _onNotFound = std::move(cb);
}

void PhoneBookManager::setOnCallWithExtensions(std::function<void(const PhoneBookEntry&)> cb) {
    _onCallWithExtensions = std::move(cb);
}

void PhoneBookManager::setOnExtensionNotFound(std::function<void(uint32_t, const char*)> cb) {
    _onExtensionNotFound = std::move(cb);
}

uint32_t PhoneBookManager::add(const PhoneBookEntry& entry) {
    PhoneBookEntry e = entry;
    e.id = _nextId++;
    if (e.method.empty()) e.method = "GET";
    _entries.push_back(e);
    save();
    return e.id;
}

bool PhoneBookManager::update(uint32_t id, const PhoneBookEntry& entry) {
    for (auto& e : _entries) {
        if (e.id == id) {
            e.number          = entry.number;
            e.name            = entry.name;
            e.type            = entry.type;
            e.url             = entry.url;
            e.method          = entry.method.empty() ? "GET" : entry.method;
            e.body            = entry.body;
            e.headers         = entry.headers;
            e.extensions      = entry.extensions;
            e.builtinFunction = entry.builtinFunction;
            e.pattern         = entry.pattern;
            e.cycles          = entry.cycles;
            e.callbackDelay   = entry.callbackDelay;
            save();
            return true;
        }
    }
    return false;
}

bool PhoneBookManager::remove(uint32_t id) {
    for (auto it = _entries.begin(); it != _entries.end(); ++it) {
        if (it->id == id) {
            _entries.erase(it);
            save();
            return true;
        }
    }
    return false;
}

void PhoneBookManager::removeAll() {
    _entries.clear();
    save();
}

const std::vector<PhoneBookEntry>& PhoneBookManager::getAll() const {
    return _entries;
}

const PhoneBookEntry* PhoneBookManager::findById(uint32_t id) const {
    for (const auto& e : _entries) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

const PhoneBookEntry* PhoneBookManager::findByNumber(const char* number) const {
    for (const auto& e : _entries) {
        if (e.number == number) return &e;
    }
    return nullptr;
}

void PhoneBookManager::init() {
    _store.load(_entries);
    for (const auto& e : _entries) {
        if (e.id >= _nextId) {
            _nextId = e.id + 1;
        }
    }
}

void PhoneBookManager::dial(const char* number) {
    const PhoneBookEntry* e = findByNumber(number);
    if (e) {
        if (!e->extensions.empty()) {
            if (_onCallWithExtensions) _onCallWithExtensions(*e);
        } else if (e->type == "builtin") {
            if (_onBuiltinCall) _onBuiltinCall(*e);
        } else {
            if (_onCall) _onCall(*e);
        }
    } else {
        if (_onNotFound) _onNotFound(number);
    }
}

bool PhoneBookManager::dialExtension(uint32_t entryId, const char* ext) {
    const PhoneBookEntry* base = findById(entryId);
    if (!base) {
        if (_onExtensionNotFound) _onExtensionNotFound(entryId, ext);
        return false;
    }

    for (const auto& x : base->extensions) {
        if (x.ext == ext) {
            PhoneBookEntry merged;
            merged.id      = base->id;
            merged.number  = base->number;
            merged.name    = x.name.empty() ? base->name : x.name;

            if (x.type == "builtin") {
                merged.type            = "builtin";
                merged.builtinFunction = x.builtinFunction;
                merged.pattern         = x.pattern;
                merged.cycles          = x.cycles;
                merged.callbackDelay   = x.callbackDelay;
                if (_onBuiltinCall) _onBuiltinCall(merged);
            } else {
                merged.url     = base->url + x.path;
                merged.method  = x.method.empty() ? base->method : x.method;
                merged.body    = x.body.empty() ? base->body : x.body;
                merged.headers = base->headers;
                if (_onCall) _onCall(merged);
            }
            return true;
        }
    }

    if (_onExtensionNotFound) _onExtensionNotFound(entryId, ext);
    return false;
}

bool PhoneBookManager::hasExtensions(uint32_t id) const {
    const PhoneBookEntry* e = findById(id);
    return e && !e->extensions.empty();
}

uint8_t PhoneBookManager::extensionLength(uint32_t id) const {
    const PhoneBookEntry* e = findById(id);
    if (!e || e->extensions.empty()) return 0;
    size_t len = e->extensions[0].ext.size();
    for (size_t i = 1; i < e->extensions.size(); i++) {
        if (e->extensions[i].ext.size() != len) return 0;
    }
    return (uint8_t)len;
}

void PhoneBookManager::save() {
    _store.save(_entries);
}
