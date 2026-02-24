#pragma once
#include <stdint.h>
#include <string>

extern std::string _mock_telnet_output;
extern int _mock_telnet_available;

class TelnetStreamClass {
public:
  void begin(uint16_t port) {}
  void print(const char* s);
  void println(const char* s);
  int available() { return _mock_telnet_available; }
  char read() { if (_mock_telnet_available > 0) _mock_telnet_available--; return 0; }
};

extern TelnetStreamClass TelnetStream;
