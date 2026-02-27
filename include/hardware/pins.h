#pragma once

// Motor driver (H-bridge)
#define PIN_MOTOR_IN1 25
#define PIN_MOTOR_IN2 32
#define PIN_MOTOR_ENA 27

// Button trigger
#define PIN_TRIGGER 14

// Rotary dial
#define PIN_DIAL_ACTIVE 26  // LOW = dial is off normal (dialing in progress)
#define PIN_DIAL_PULSE  33  // pulses LOW once per digit as dial returns

// Handset hook switch
#define PIN_HANDSET 13  // LOW = off hook (handset lifted), HIGH = on hook
