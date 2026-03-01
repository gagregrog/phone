#include "system/Logger.h"
#include <Arduino.h>
#include <TelnetStream.h>
#include <stdarg.h>

Logger logger;

void Logger::begin(uint16_t port) {
  TelnetStream.begin(port);
}

void Logger::handle() {
  // TelnetStream has no API to disable input. Drain the receive buffer each
  // loop iteration so incoming bytes from connected clients are discarded
  // rather than accumulating.
  while (TelnetStream.available()) TelnetStream.read();
}

void Logger::setOnLog(std::function<void(const char*, const char*, const char*)> cb) {
  _onLog = std::move(cb);
}

void Logger::_log(const char* prefix, const char* level, const char* category, const char* msg) {
  Serial.print(prefix);
  Serial.println(msg);
  TelnetStream.print(prefix);
  TelnetStream.println(msg);
  if (_onLog) _onLog(level, category, msg);
}

void Logger::_logf(const char* prefix, const char* level, const char* category, const char* fmt, va_list args) {
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  _log(prefix, level, category, buf);
}

// --- Severity-based (uncategorized) ---

void Logger::info(const char* msg)  { _log("[INFO] ",  "info",  "", msg); }
void Logger::warn(const char* msg)  { _log("[WARN] ",  "warn",  "", msg); }
void Logger::error(const char* msg) { _log("[ERROR] ", "error", "", msg); }

void Logger::infof(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[INFO] ", "info", "", fmt, args);
  va_end(args);
}

void Logger::warnf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[WARN] ", "warn", "", fmt, args);
  va_end(args);
}

void Logger::errorf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[ERROR] ", "error", "", fmt, args);
  va_end(args);
}

// --- Categorized ---

void Logger::_logc(const char* category, const char* msg) {
  char prefix[16];
  snprintf(prefix, sizeof(prefix), "[%s] ", category);
  _log(prefix, "info", category, msg);
}

void Logger::_logcf(const char* category, const char* fmt, va_list args) {
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  _logc(category, buf);
}

void Logger::hardware(const char* msg)  { _logc("HARDWARE",  msg); }
void Logger::api(const char* msg)       { _logc("API",       msg); }
void Logger::phone(const char* msg)     { _logc("PHONE",     msg); }
void Logger::scheduler(const char* msg) { _logc("SCHEDULER", msg); }

void Logger::hardwaref(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logcf("HARDWARE", fmt, args);
  va_end(args);
}

void Logger::apif(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logcf("API", fmt, args);
  va_end(args);
}

void Logger::phonef(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logcf("PHONE", fmt, args);
  va_end(args);
}

void Logger::schedulerf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logcf("SCHEDULER", fmt, args);
  va_end(args);
}
