#pragma once
#include <stdint.h>
#include <stdarg.h>

class Logger {
public:
  void begin(uint16_t port = 23);
  void handle();

  void info(const char* msg);
  void warn(const char* msg);
  void error(const char* msg);

  void infof(const char* fmt, ...);
  void warnf(const char* fmt, ...);
  void errorf(const char* fmt, ...);

private:
  void _log(const char* level, const char* msg);
  void _logf(const char* level, const char* fmt, va_list args);
};

extern Logger logger;
