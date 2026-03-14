#pragma once
#include "phonebook/PhoneBookEntry.h"
#include <vector>

class PhoneBookStore {
public:
    virtual ~PhoneBookStore() = default;
    virtual void load(std::vector<PhoneBookEntry>& entries) = 0;
    virtual void save(const std::vector<PhoneBookEntry>& entries) = 0;
};
