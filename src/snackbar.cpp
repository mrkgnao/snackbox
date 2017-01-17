// gcc `pkg-config --libs xcb` main.c -o main

#include "snackbox/snackbar.hpp"

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

#include <chrono>

// mkfifo
#include <sys/types.h>
#include <sys/stat.h>

using json = nlohmann::json;

namespace snackbox {
void Snackbar::setupEwmh() {
  LOG_ENTRY;
  _ewmh_cookie = xcb_ewmh_init_atoms(conn, &ewmh);
  if (!xcb_ewmh_init_atoms_replies(&ewmh, _ewmh_cookie, nullptr)) {
    logger::error("Failed to init ewmh.");
  } else {
    logger::info("EWMH connection initialized.");
  }
  LOG_EXIT;
}

void Snackbar::createWindow() {
  LOG_ENTRY;

  conn = xcb_connect(nullptr, &screen_nbr);
  logger::info("Connected to xcb.");

  screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
  logger::debug("Screen {}", screen_nbr);

  uint32_t mask;
  uint32_t values[2];

  mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  values[0] = back.getValue();
  values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS;

  window = xcb_generate_id(conn);
  cookie = xcb_create_window(conn,                 // the connection
                             XCB_COPY_FROM_PARENT, // depth
                             window,               // window id
                             screen->root,         // parent window
                             x, y,                 // origin
                             width, height,        // dimensions
                             0,                    // border width
                             XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                             mask, values);

  LOG_EXIT;
}

void Snackbar::setVisualType() {
  LOG_ENTRY;
  if (screen) {
    root_visual = screen->root_visual;
    xcb_depth_iterator_t depth_iter;

    depth_iter = xcb_screen_allowed_depths_iterator(screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
      xcb_visualtype_iterator_t visual_iter;

      visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
      for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
        if (screen->root_visual == visual_iter.data->visual_id) {
          visual_type = visual_iter.data;
          break;
        }
      }
    }
  }
  LOG_EXIT;
}

void Snackbar::setEwmhProperty(xcb_atom_t property, xcb_atom_t value,
                               bool replace = false) {
  LOG_ENTRY;
  auto mode = replace ? XCB_PROP_MODE_REPLACE : XCB_PROP_MODE_APPEND;
  xcb_change_property(conn, mode, window, property, XCB_ATOM_ATOM, 32, 1,
                      &value);
  LOG_EXIT;
}

void Snackbar::setWindowType(xcb_atom_t window_type, bool replace = false) {
  LOG_ENTRY;
  setEwmhProperty(ewmh._NET_WM_WINDOW_TYPE, window_type, replace);
  LOG_EXIT;
}

void Snackbar::setWindowState(xcb_atom_t window_state, bool replace = false) {
  LOG_ENTRY;
  setEwmhProperty(ewmh._NET_WM_STATE, window_state, replace);
  LOG_EXIT;
}

void Snackbar::setupWindowAtoms() {
  LOG_ENTRY;
  setWindowType(ewmh._NET_WM_WINDOW_TYPE_NOTIFICATION, true);
  setWindowType(ewmh._NET_WM_WINDOW_TYPE_UTILITY);

  setWindowState(ewmh._NET_WM_STATE_ABOVE, true);
  flushToScreen();
  LOG_EXIT;
}

void Snackbar::createCairoSurface() {
  LOG_ENTRY;

  surface = cairo_xcb_surface_create(conn, window, visual_type, x, y);
  logger::info("Created cairo surface at ({},{})", x, y);

  cairo_xcb_surface_set_size(surface, width, height);
  logger::info("Cairo surface size set to ({},{})", width, height);

  ctx = cairo_create(surface);
  logger::info("Created cairo context");

  // desc = pango_font_description_from_string (FONT);
  // pango_layout_set_font_description (layout, desc);
  // pango_font_description_free (desc);

  logger::info("Created pango layout");

  LOG_EXIT;
}

void Snackbar::initPangoLayout() {
  LOG_ENTRY;
  layout = pango_cairo_create_layout(ctx);
  LOG_EXIT;
}

xcb_void_cookie_t Snackbar::setOrigin(uint32_t x, uint32_t y) {
  LOG_ENTRY;
  const uint32_t values[] = {x, y};
  const uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y;

  return xcb_configure_window(conn, window, mask, values);
  LOG_EXIT;
}

xcb_void_cookie_t Snackbar::setSize(uint32_t width, uint32_t height) {
  LOG_ENTRY;
  const uint32_t values[] = {width, height};
  const uint16_t mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

  return xcb_configure_window(conn, window, mask, values);
  LOG_EXIT;
}

void Snackbar::setTitle(const std::string &title) {
  LOG_ENTRY;
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, title.length(), title.c_str());
  LOG_EXIT;
}

void Snackbar::setOpacity(int transparency) {
  LOG_ENTRY;
  unsigned long opacity = (unsigned long)(transparency * (0xffffffff / 100));
  xcb_intern_atom_reply_t *reply{xcb_intern_atom_reply(
      conn, xcb_intern_atom(conn, false, 22, "_NET_WM_WINDOW_OPACITY"),
      nullptr)};
  if (reply) {
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, reply->atom,
                        XCB_ATOM_CARDINAL, 32, 1, &opacity);
    xcb_flush(conn);
    free(reply);
  }
  LOG_EXIT;
}

xcb_void_cookie_t Snackbar::hideBar() {
  LOG_ENTRY;
  auto ret = xcb_unmap_window(conn, window);
  LOG_EXIT;

  return ret;
}

xcb_void_cookie_t Snackbar::showBar() {
  LOG_ENTRY;
  auto ret = xcb_map_window(conn, window);
  LOG_EXIT;

  return ret;
}

void Snackbar::setTextFont(const std::string &font_desc) {
  LOG_ENTRY;

  PangoFontDescription *desc;
  desc = pango_font_description_from_string(font_desc.c_str());
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);

  LOG_EXIT;
}

void Snackbar::setText(const std::string &bar_text) {
  LOG_ENTRY;

  char *text;
  GError *err;
  PangoAttrList *attr;
  pango_parse_markup(bar_text.c_str(), -1, 0, &attr, &text, NULL, &err);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_attributes(layout, attr);
  logger::info("Setting bar text to {}", bar_text);
  // free(text);
  // free(err);

  LOG_EXIT;
}

void Snackbar::init(void) {
  LOG_ENTRY;

  createWindow();
  setVisualType();
  createCairoSurface();
  initPangoLayout();

  setupEwmh();
  setupWindowAtoms();

  setOpacity(80);

  LOG_EXIT;
}

void Snackbar::mainloop() {
  LOG_ENTRY;

  setTitle("snackbar " + barName);
  pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
  setTextFont("Iosevka 11");

  showBar();

  int iter_count = 0;
  float bg = 0;

  repaint();

  json msg;
  const std::string path = "/tmp/" + barName + "_fifo";

  mkfifo(path.c_str(), 0666);
  logger::info("FIFO made.");

  // TODO explore zeromq
  std::fstream fifo(path); // this keeps the socket open!
  logger::info("Connected to FIFO.");

  std::string buffer;

  while (std::getline(fifo, buffer, '\0')) {
    iter_count++;

    msg = json::parse(buffer);
    process(msg);

    repaint();
    std::this_thread::sleep_for(std::chrono::microseconds(THREAD_DELAY_US));
  }

  LOG_EXIT;
}

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

void Snackbar::setBackground(int r, int g, int b) {
  LOG_ENTRY;
  back.r = r;
  back.g = g;
  back.b = b;
  LOG_EXIT;
}

void Snackbar::process(const json &j) {
  logger::debug("Received JSON value"); //:");
  // logger::debug("{}", j.dump(2));

  bool failed = false;

  if (j.find("type") == j.end()) {
    logger::error("No command type specified!");
    failed = true;
  }

  if (j.find("bar") == j.end()) {
    logger::error("No target bar specified!");
    failed = true;
  }

  if (!failed) {
    try {
      std::string type = j["type"];
      switch (get_with_default(command_table, type, unknown_command)) {
      case set_text:
        setText(j["text"]);
        break;
      case set_background:
        setBackground(j["red"], j["green"], j["blue"]);
        break;
      case set_font:
        setTextFont(j["font_desc"]);
        break;
      case set_origin:
        setOrigin(j["x"], j["y"]);
        break;
      case set_size:
        setSize(j["width"], j["height"]);
        break;
      // visibility
      case show_bar:
        showBar();
        break;
      case hide_bar:
        hideBar();
        break;

      // other things
      case quit_program:
        logger::warn("Quit program!");
        hideBar();
        cleanup();
        break;
      case unknown_command:
        logger::error("Unknown command type {}", type);
        break;
      default:
        logger::error("Unreachable state");
      }
    } catch (std::exception const &err) {
      logger::error("JSON error");
      logger::error("{}", err.what());
    }
  }
}

void Snackbar::repaint() {
  std::chrono::high_resolution_clock::time_point repaint_start =
      std::chrono::high_resolution_clock::now();

  cairo_push_group(ctx);

  cairo_set_source_rgb(ctx, back.rf(), back.gf(), back.bf());
  cairo_paint(ctx);

  cairo_set_source_rgb(ctx, fore.rf(), fore.gf(), fore.bf());
  cairo_move_to(ctx, 2, 2);
  pango_cairo_show_layout(ctx, layout);

  cairo_pop_group_to_source(ctx);
  cairo_paint(ctx);

  flushToScreen();

  std::chrono::high_resolution_clock::time_point repaint_end =
      std::chrono::high_resolution_clock::now();
  logger::set_repaint_time(std::chrono::duration<double, std::micro>(
                                                                     (repaint_end - repaint_start).count() / 1000.0).count());
}

void Snackbar::flushToScreen() {
  cairo_surface_flush(surface);
  xcb_flush(conn);
}

Snackbar::Snackbar(std::string barName, int x, int y, int width, int height)
    : barName(barName), x(x), y(y), width(width), height(height),
      back(RGB(0, 0, 0)), fore(RGB(255, 255, 255)) {

  logger::info("Created bar {}:", barName);
  logger::debug("({:>4}, {:>4}), ({:>4}, {:>4})", x, y, x + width, y);
  logger::debug("({:>4}, {:>4}), ({:>4}, {:>4})", x, y + height, x + width,
                y + height);
  init();
}

void Snackbar::cleanup() {
  LOG_ENTRY;

  cairo_destroy(ctx);
  cairo_surface_destroy(surface);
  xcb_disconnect(conn);

  LOG_EXIT;
}

Snackbar::~Snackbar() { cleanup(); }
}

// LOG_ENTRY;

// LOG_EXIT;
