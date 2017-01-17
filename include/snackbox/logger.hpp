#ifndef SNACKBOX_LOGGER_H_
#define SNACKBOX_LOGGER_H_

#include "fmt/printf.h"
#include "fmt/format.h"

#include <unistd.h>
#include <sys/ioctl.h>

#include <string>
#include <map>
#include <regex>
#include <algorithm>
#include <iostream>

#include <chrono>

#define LOG_ENTRY_NO_TIME                                                      \
  snackbox::logger::trace_entry("{funcname}",                                  \
                                fmt::arg("funcname", __PRETTY_FUNCTION__))

#define LOG_EXIT_NO_TIME                                                       \
  snackbox::logger::trace_exit("{funcname}",                                   \
                               fmt::arg("funcname", __PRETTY_FUNCTION__))

#define LOG_ENTRY                                                              \
  snackbox::logger::trace_entry(                                               \
      "{funcname:<{padding}} ({repaint_time:.2f} ms)",                         \
      fmt::arg("funcname", __PRETTY_FUNCTION__))

#define LOG_EXIT                                                               \
  snackbox::logger::trace_exit(                                                \
      "{funcname:<{padding}} ({repaint_time:.2f} ms)",                         \
      fmt::arg("funcname", __PRETTY_FUNCTION__))

namespace snackbox {
namespace logger {

enum log_level {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  CRITICAL,
  _ENTER_FUNC,
  _EXIT_FUNC
};

enum format_type {
  reset,
  bold,
  dark,
  underline,
  blink,
  reverse,
  concealed,
  no_format
};

enum color_selector {
  grey,
  red,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  white,
  no_color
};

extern std::map<format_type, std::string> format_code;

extern std::map<color_selector, std::string> fg_colors;

/// Background colors
extern std::map<color_selector, std::string> bg_colors;

extern std::map<log_level, std::string> level_labels;

extern std::map<log_level, std::string> level_formats;

extern int time_padding;
extern int indent;

extern int old_persistent_length;

extern std::string persistent_msg;

extern double repaint_time;

template <typename... Args>
std::string _create_log(std::string msg, log_level level, Args... args) {
  int prew = level_formats[level].length();
  int labw = level_labels[level].length();
  int postw = format_code[reset].length();

  auto colored_level = fmt::format("{} {} {}", level_formats[level],
                                   level_labels[level], format_code[reset]);

  auto formatted_msg = fmt::format(msg, args...);

  auto colored_level_width = prew + 8 + postw;

  return fmt::format(" {:>{}} {}", colored_level, colored_level_width,
                     formatted_msg);
}

void _update_persistent_log();
std::string highlightFunctionName(std::string msg, std::string format_string);
void set_repaint_time(double micros);

template <typename... Args>
void _log(std::string msg, log_level level, Args... args) {
  fmt::print("{log_text:<130}\n",
             fmt::arg("log_text", _create_log(msg, level, args...)));
  // fmt::arg("len", old_persistent_length));
  _update_persistent_log();
}

template <typename... Args> void trace(std::string msg, Args... args) {
  _log(msg, TRACE, args...);
}
template <typename... Args> void debug(std::string msg, Args... args) {
  _log(msg, DEBUG, args...);
}
template <typename... Args> void info(std::string msg, Args... args) {
  _log(msg, INFO, args...);
}
template <typename... Args> void warn(std::string msg, Args... args) {
  _log(msg, WARN, args...);
}
template <typename... Args> void error(std::string msg, Args... args) {
  _log(msg, ERROR, args...);
}
template <typename... Args> void critical(std::string msg, Args... args) {
  _log(msg, CRITICAL, args...);
}

template <typename... Args> void trace_entry(std::string msg, Args... args) {
  // old_persistent_length = persistent_msg.length();
  persistent_msg = _create_log(
      fmt::format(highlightFunctionName(
          fmt::format(msg, fmt::arg("padding", time_padding),
                      fmt::arg("repaint_time", repaint_time), args...),
          format_code[underline])),
      _ENTER_FUNC);
  fmt::print("{:<130}\r", persistent_msg); //, old_persistent_length);
  std::cout.flush();
}

template <typename... Args> void trace_exit(std::string msg, Args... args) {
  // old_persistent_length = persistent_msg.length();
  persistent_msg = _create_log(
      fmt::format(highlightFunctionName(
          fmt::format(msg, fmt::arg("padding", time_padding),
                      fmt::arg("repaint_time", repaint_time), args...),
          format_code[underline])),
      _EXIT_FUNC);
  fmt::print("{:<130}\r", persistent_msg); //, old_persistent_length);
  std::cout.flush();
}

template <typename... Args> void printf(std::string msg, Args... args) {
  _log(fmt::sprintf(msg, args...), TRACE);
}
}
}

#endif
