#include "phonebook/PhoneBookManager.h"

PhoneBookManager::PhoneBookManager(PhoneBookStore& store)
    : _store(store), _nextId(1) {}

void PhoneBookManager::setOnCall(std::function<void(const PhoneBookEntry&)> cb) {
    _onCall = std::move(cb);
}

void PhoneBookManager::setOnNotFound(std::function<void(const char*)> cb) {
    _onNotFound = std::move(cb);
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
            e.number  = entry.number;
            e.name    = entry.name;
            e.url     = entry.url;
            e.method  = entry.method.empty() ? "GET" : entry.method;
            e.body    = entry.body;
            e.headers = entry.headers;
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
        if (_onCall) _onCall(*e);
    } else {
        if (_onNotFound) _onNotFound(number);
    }
}

void PhoneBookManager::save() {
    _store.save(_entries);
}
