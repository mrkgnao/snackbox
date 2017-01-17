#include "snackbox/logger.hpp"

#include <map>
#include <string>

namespace snackbox {
namespace logger {

  std::string persistent_msg = "";

std::map<format_type, std::string> format_code = {
    {reset, "\033[00m"},    {bold, "\033[1m"},  {dark, "\033[2m"},
    {underline, "\033[4m"}, {blink, "\033[5m"}, {reverse, "\033[7m"},
    {concealed, "\033[8m"}, {no_format, ""}};

std::map<color_selector, std::string> fg_colors = {
    {grey, "\033[30m"},   {red, "\033[31m"},   {green, "\033[32m"},
    {yellow, "\033[33m"}, {blue, "\033[34m"},  {magenta, "\033[35m"},
    {cyan, "\033[36m"},   {white, "\033[37m"}, {no_color, ""}};

/// Background colors
std::map<color_selector, std::string> bg_colors = {
    {grey, "\033[40m"},   {red, "\033[41m"},   {green, "\033[42m"},
    {yellow, "\033[43m"}, {blue, "\033[44m"},  {magenta, "\033[45m"},
    {cyan, "\033[46m"},   {white, "\033[47m"}, {no_color, ""}};

std::map<log_level, std::string> level_labels = {
    {TRACE, "TRACE"},      {DEBUG, "DEBUG"},    {INFO, "INFO"},
    {WARN, "WARN"},        {ERROR, "ERROR"},    {CRITICAL, "CRITICAL"},
    {_ENTER_FUNC, "--->"}, {_EXIT_FUNC, "<---"}};

std::map<log_level, std::string> level_formats = {
    {TRACE, bg_colors[no_color]},
    {DEBUG, bg_colors[no_color] + fg_colors[green] + format_code[bold]},
    {INFO, bg_colors[no_color] + fg_colors[cyan] + format_code[bold]},
    {WARN, bg_colors[no_color] + fg_colors[yellow]},
    {ERROR, bg_colors[red] + fg_colors[white] + format_code[bold] +
                format_code[no_format]},
    {CRITICAL, bg_colors[no_color] + fg_colors[red] + format_code[reverse]},
    {_ENTER_FUNC, bg_colors[no_color] + fg_colors[green] + format_code[bold]},
    {_EXIT_FUNC, bg_colors[no_color] + fg_colors[red] + format_code[bold]}};

int time_padding = 90;
int indent = 4;

  int old_persistent_length = 0;

std::string highlightFunctionName(std::string msg, std::string format_string) {
  std::regex functionWithArgs("::([a-zA-Z]*)\\(([a-z])");
  std::regex voidFunction("::([a-zA-Z]*)\\(\\)");
  std::string ret;

  try {
    ret = std::regex_replace(msg, voidFunction, format_code[reset] + "::" +
                                                    format_string + "$1" +
                                                    format_code[reset] + "()");
    ret = std::regex_replace(ret, functionWithArgs,
                             format_code[reset] + "::" + format_string + "$1" +
                                 format_code[reset] + "($2");
  } catch (std::exception const &err) {
    error(err.what());
  }
  return ret;
}


  void _update_persistent_log() {
    fmt::print(persistent_msg + "\r");
    std::cout.flush();
  }
}
}
