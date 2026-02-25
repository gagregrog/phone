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

void Logger::_log(const char* level, const char* msg) {
  Serial.print(level);
  Serial.println(msg);
  TelnetStream.print(level);
  TelnetStream.println(msg);
}

void Logger::_logf(const char* level, const char* fmt, va_list args) {
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, args);
  _log(level, buf);
}

void Logger::info(const char* msg)  { _log("[INFO] ",  msg); }
void Logger::warn(const char* msg)  { _log("[WARN] ",  msg); }
void Logger::error(const char* msg) { _log("[ERROR] ", msg); }

void Logger::infof(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[INFO] ", fmt, args);
  va_end(args);
}

void Logger::warnf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[WARN] ", fmt, args);
  va_end(args);
}

void Logger::errorf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  _logf("[ERROR] ", fmt, args);
  va_end(args);
}
