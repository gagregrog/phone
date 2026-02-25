#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <functional>

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

  void setOnLog(std::function<void(const char* level, const char* msg)> cb);

private:
  void _log(const char* prefix, const char* level, const char* msg);
  void _logf(const char* prefix, const char* level, const char* fmt, va_list args);
  std::function<void(const char*, const char*)> _onLog;
};

extern Logger logger;
