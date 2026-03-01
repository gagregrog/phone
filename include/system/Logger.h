#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <functional>

class Logger {
public:
  void begin(uint16_t port = 23);
  void handle();

  // Severity-based (uncategorized) — use for startup messages and warnings
  void info(const char* msg);
  void warn(const char* msg);
  void error(const char* msg);

  void infof(const char* fmt, ...);
  void warnf(const char* fmt, ...);
  void errorf(const char* fmt, ...);

  // Categorized — all at info severity
  void hardware(const char* msg);
  void api(const char* msg);
  void phone(const char* msg);
  void scheduler(const char* msg);

  void hardwaref(const char* fmt, ...);
  void apif(const char* fmt, ...);
  void phonef(const char* fmt, ...);
  void schedulerf(const char* fmt, ...);

  void setOnLog(std::function<void(const char* level, const char* category, const char* msg)> cb);

private:
  void _log(const char* prefix, const char* level, const char* category, const char* msg);
  void _logf(const char* prefix, const char* level, const char* category, const char* fmt, va_list args);
  void _logc(const char* category, const char* msg);
  void _logcf(const char* category, const char* fmt, va_list args);

  std::function<void(const char*, const char*, const char*)> _onLog;
};

extern Logger logger;
