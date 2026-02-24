#include "Arduino.h"
#include "TelnetStream.h"
#include <string>

std::string _mock_serial_output;
std::string _mock_telnet_output;
int _mock_telnet_available = 0;

HardwareSerial Serial;
TelnetStreamClass TelnetStream;

void HardwareSerial::print(const char* s)    { _mock_serial_output += s; }
void HardwareSerial::println(const char* s)  { _mock_serial_output += s; _mock_serial_output += "\n"; }
void TelnetStreamClass::print(const char* s)   { _mock_telnet_output += s; }
void TelnetStreamClass::println(const char* s) { _mock_telnet_output += s; _mock_telnet_output += "\n"; }
