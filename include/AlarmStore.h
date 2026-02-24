#pragma once
#include "AlarmEntry.h"
#include <vector>

class AlarmStore {
public:
    virtual ~AlarmStore() = default;
    virtual void load(std::vector<AlarmEntry>& alarms) = 0;
    virtual void save(const std::vector<AlarmEntry>& alarms) = 0;
};
