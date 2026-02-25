#pragma once
#include "alarm/AlarmManager.h"

// Register alarm REST endpoints on the shared server (call after ringerAPIBegin).
void alarmAPIBegin(AlarmManager& mgr);
