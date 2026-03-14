#pragma once
#include "phonebook/PhoneBookStore.h"

class NVSPhoneBookStore : public PhoneBookStore {
public:
    void load(std::vector<PhoneBookEntry>& entries) override;
    void save(const std::vector<PhoneBookEntry>& entries) override;
};
