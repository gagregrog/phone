#pragma once

class PhoneBookManager;
class PhoneController;
class Ringer;

void phoneBookBuiltinsBegin(PhoneBookManager& mgr, PhoneController& phoneCtrl, Ringer& ringer);
void phoneBookBuiltinsTick(unsigned long now);
