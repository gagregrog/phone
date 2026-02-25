#pragma once
#include "alarm/AlarmStore.h"

class NVSAlarmStore : public AlarmStore {
public:
    void load(std::vector<AlarmEntry>& alarms) override;
    void save(const std::vector<AlarmEntry>& alarms) override;
};
