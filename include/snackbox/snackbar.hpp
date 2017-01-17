#ifndef SNACKBAR_H_
#define SNACKBAR_H_

#include "snackbox/logger.hpp"
#include "librgb/RGBColor.h"
#include <nlohmann/json.hpp>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include <cairo-xcb.h>
#include <pango/pangocairo.h>

#include <signal.h>
#include <unistd.h>

#include <thread>
#include <chrono>

#include <iostream>
#include <ios>
#include <string>
#include <fstream>
#include <regex>

// mkfifo
#include <sys/types.h>
#include <sys/stat.h>

namespace snackbox {
class Snackbar {
private:
  const int THREAD_DELAY_US = 200;

  xcb_connection_t *conn;
  xcb_screen_t *screen;
  xcb_window_t window;
  xcb_void_cookie_t cookie;

  xcb_ewmh_connection_t ewmh;
  xcb_intern_atom_cookie_t *_ewmh_cookie = nullptr;

  xcb_visualid_t root_visual = {0};
  xcb_visualtype_t *visual_type = nullptr;

  int screen_nbr;

  cairo_surface_t *surface;
  cairo_t *ctx;

  PangoLayout *layout;

  std::string barName;
  int x, y, width, height;

  RGBColor back, fore;

  void setupEwmh();

  void createWindow();

  void setVisualType();

  void setEwmhProperty(xcb_atom_t property, xcb_atom_t value, bool replace);

  void setWindowType(xcb_atom_t window_type, bool replace);

  void setWindowState(xcb_atom_t window_state, bool replace);

  void setupWindowAtoms();

  void createCairoSurface();

  void initPangoLayout();

  xcb_void_cookie_t setOrigin(uint32_t x, uint32_t y);

  xcb_void_cookie_t setSize(uint32_t width, uint32_t height);

  void setTitle(const std::string &title);

  void setOpacity(int transparency);

  xcb_void_cookie_t hideBar();

  xcb_void_cookie_t showBar();

  void setTextFont(const std::string &font_desc);

  void setText(const std::string &bar_text);

public:
  enum command {
    set_font,
    set_text,
    set_origin,
    set_size,
    set_background,
    show_bar,
    hide_bar,
    quit_program,
    unknown_command
  };
  std::map<std::string, command> command_table = {
      {"quit", quit_program}, {"set_text", set_text},
      {"set_font", set_font}, {"set_origin", set_origin},
      {"set_size", set_size}, {"show", show_bar},
      {"hide", hide_bar},     {"set_bg", set_background}};

  template <typename MAP>
  const typename MAP::mapped_type &
  get_with_default(const MAP &m, const typename MAP::key_type &key,
                   const typename MAP::mapped_type &defval) {
    typename MAP::const_iterator it = m.find(key);
    if (it == m.end())
      return defval;

    return it->second;
  }

  void init();

  void mainloop();

  void setBackground(int r, int g, int b);

  void process(const nlohmann::json &j);

  void repaint();

  void flushToScreen();

  Snackbar() = delete;

  Snackbar(std::string barName, int x, int y, int width, int height);

  void cleanup();

  ~Snackbar();
};
}

#endif
