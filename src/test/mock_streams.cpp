#include "Arduino.h"
#include <string>

std::string _mock_serial_output;

HardwareSerial Serial;

void HardwareSerial::print(const char* s)    { _mock_serial_output += s; }
void HardwareSerial::println(const char* s)  { _mock_serial_output += s; _mock_serial_output += "\n"; }
