#pragma once
#include "phonebook/PhoneBookManager.h"

class PhoneController;

// Wire PhoneController::setOnDialComplete → PhoneBookManager::dial,
// and PhoneBookManager callbacks to HTTP execution and event publishing.
// Call once in setup() after phoneBookMgr.init() and phoneCtrl.begin().
void phoneBookCallerBegin(PhoneBookManager& mgr, PhoneController& phoneCtrl);

// Execute the HTTP request for a given entry (used by both dial and test routes).
// Publishes phonebook/called or phonebook/error events. Returns HTTP status code.
int phoneBookCallerExec(const PhoneBookEntry& entry);
