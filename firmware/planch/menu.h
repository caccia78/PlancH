// PlancH: menu model shared by the real board and the test bench.
// Home Assistant builds the menu (package homeassistant/planch.yaml, sensor.planch_menu) as compact
// JSON; this class turns it into screens, reacts to the keys and computes the 13 LED colours.
// It has no ESPHome dependency except ArduinoJson, so it can be tested on a PC.
#pragma once

#include <ArduinoJson.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace planch {

enum Key { UP, DOWN, LEFT, RIGHT, OK, BACK, S1, S2, S3, S4, TOUCH };

struct Device {
  std::string entity, name, state;
  char type = 0;        // 'l' light, 'c' cover, 'h' climate (heating)
  float value = NAN;    // light brightness %, cover position %, climate target temperature
  float current = NAN;  // climate current temperature
  // Lights: supported features ('b' brightness, 'c' colour, 't' white temperature, 'e' effects),
  // current colour (hue, saturation), white temperature with its range, current effect
  std::string modes, effect;
  float hue = NAN, sat = NAN, kelvin = NAN, kmin = 2000, kmax = 6500;
};

struct Room {
  std::string name;
  std::vector<Device> devices;
};

// Colour of the palette defined in the Home Assistant package (hue 0-360, saturation 0-100)
struct Colour {
  std::string name;
  float hue = 0, sat = 0;
};

struct Scene {
  std::string entity, name;
  bool active = false;  // most recently activated of the four (computed by Home Assistant)
};

// Home Assistant action to perform; field empty = only entity_id. scene: key S1-S4 as 0-3 whose
// LED shows the outcome, -1 for menu actions (an error lights D1)
struct Call {
  std::string action, entity, field, value;
  int scene = -1;
};

struct Line {
  std::string left, right;
};

struct Rgb {
  uint8_t r = 0, g = 0, b = 0;
};

class Menu {
 public:
  static constexpr int ROWS = 4;               // rows under the title on the 128 x 64 OLED
  static constexpr uint32_t TIMEOUT_MS = 30000;  // screen off after inactivity
  static constexpr uint32_t OK_MS = 800;          // green flash on a scene LED when HA confirms
  static constexpr uint32_t ERROR_MS = 1500;      // red flash on a scene LED or on D1 on an error
  static constexpr uint32_t PENDING_MS = 3000;    // scene LED full while waiting for the answer
  static constexpr uint32_t SCENE_AWAKE_MS = 5000;  // scene LEDs on after a scene key, screen off
  static constexpr int LEDS = 13;

  // ---- data from Home Assistant -------------------------------------------------------------
  // The attributes start with a version prefix ("planch1:") that keeps them strings in HA
  static const char *json_start_(const std::string &s) {
    size_t p = s.find('{');
    return p == std::string::npos ? "" : s.c_str() + p;
  }

  void set_menu(const std::string &json) {
    JsonDocument doc;
    if (deserializeJson(doc, json_start_(json)) != DeserializationError::Ok) {
      rooms_.clear();
      valid_ = false;
      return;
    }
    std::vector<Room> rooms;
    for (JsonObject r : doc["r"].as<JsonArray>()) {
      Room room;
      room.name = r["n"] | "";
      for (JsonObject d : r["d"].as<JsonArray>()) {
        Device dev;
        dev.entity = d["e"] | "";
        dev.name = d["n"] | "";
        dev.state = d["s"] | "";
        const char *t = d["t"] | "";
        dev.type = t[0];
        if (!d["v"].isNull()) dev.value = d["v"].as<float>();
        if (!d["c"].isNull()) dev.current = d["c"].as<float>();
        dev.modes = d["m"] | "";
        if (d["hs"].is<JsonArray>()) {
          dev.hue = d["hs"][0].as<float>();
          dev.sat = d["hs"][1].as<float>();
        }
        if (!d["k"].isNull()) dev.kelvin = d["k"].as<float>();
        if (d["kr"].is<JsonArray>()) {
          dev.kmin = d["kr"][0] | 2000.0f;
          dev.kmax = d["kr"][1] | 6500.0f;
        }
        dev.effect = d["f"] | "";
        if (!dev.entity.empty() && dev.type) room.devices.push_back(dev);
      }
      if (!room.devices.empty()) rooms.push_back(room);
    }
    warning_ = (doc["w"] | 0) != 0;
    rooms_ = rooms;
    valid_ = true;
    restore_selection_();
  }

  // {"p": [[name, hue, saturation], ...]}: the colours the Colour row steps through
  void set_palette(const std::string &json) {
    JsonDocument doc;
    palette_.clear();
    if (deserializeJson(doc, json_start_(json)) != DeserializationError::Ok) return;
    for (JsonArray c : doc["p"].as<JsonArray>()) {
      Colour col;
      col.name = c[0] | "";
      col.hue = c[1] | 0.0f;
      col.sat = c[2] | 0.0f;
      if (!col.name.empty()) palette_.push_back(col);
    }
  }

  void set_scenes(const std::string &json) {
    JsonDocument doc;
    for (auto &s : scenes_) s = Scene();
    if (deserializeJson(doc, json_start_(json)) != DeserializationError::Ok) return;
    for (int i = 0; i < 4; i++) {
      JsonObject s = doc[std::to_string(i + 1)];
      if (s.isNull()) continue;
      scenes_[i].entity = s["e"] | "";
      scenes_[i].name = s["n"] | "";
      scenes_[i].active = (s["a"] | 0) != 0;
    }
  }

  void set_connected(bool connected) { connected_ = connected; }

  void set_backlight(int channel, float level) { set_(backlight_, channel, level); }

  // D2 (n = 0) and D3 (n = 1): status lights driven by Home Assistant automations
  void set_status(int n, int channel, float level) {
    if (n == 0 || n == 1) set_(status_leds_[n], channel, level);
  }

  // Outcome of a Home Assistant action (on_success / on_error)
  void result(int scene, bool ok, uint32_t now) {
    if (scene >= 0 && scene < 4) {
      result_[scene] = now;
      result_ok_[scene] = ok;
      answered_[scene] = true;
    } else if (!ok) {
      error_ = now;
    }
  }

  // ---- keys --------------------------------------------------------------------------------
  std::vector<Call> key(Key k, uint32_t now) {
    std::vector<Call> calls;
    last_input_ = now;
    if (k >= S1 && k <= S4) {  // scene keys work with the screen off too
      const Scene &s = scenes_[k - S1];
      int i = k - S1;
      scene_wake_ = now;
      if (!s.entity.empty()) {
        Call c{scene_action_(s.entity), s.entity, "", ""};
        c.scene = i;
        calls.push_back(c);
        pressed_[i] = now;
        answered_[i] = false;
      }
      return calls;
    }
    if (!screen_on_ || k == TOUCH) {  // the first key only wakes the screen
      screen_on_ = true;
      return calls;
    }
    if (level_ == CONTROL) {
      control_key_(k, calls);
    } else {
      browse_key_(k);
    }
    return calls;
  }

  void tick(uint32_t now) {
    if (screen_on_ && now - last_input_ > TIMEOUT_MS) {
      screen_on_ = false;
      level_ = ROOMS;  // next wake starts from the rooms
      cursor_[ROOMS] = 0;
    }
  }

  // ---- outputs -----------------------------------------------------------------------------
  bool screen_on() const { return screen_on_; }

  // Title and up to ROWS rows; selected() is the highlighted row (-1 none)
  std::string title() const { return screen_().title; }
  std::vector<Line> rows() const { return screen_().rows; }
  int selected() const { return screen_().selected; }

  // Plain text of the screen, for the test bench and the log
  std::string text() const {
    if (!screen_on_) return "(screen off)";
    Screen s = screen_();
    std::string out = s.title;
    for (size_t i = 0; i < s.rows.size(); i++) {
      std::string line = (int(i) == s.selected ? "> " : "  ") + s.rows[i].left;
      if (!s.rows[i].right.empty()) {
        int pad = 21 - int(line.size()) - int(s.rows[i].right.size());
        line += std::string(pad > 1 ? pad : 1, ' ') + s.rows[i].right;
      }
      out += "\n" + line;
    }
    return out;
  }

  // D1 problems, D2-D3 status lights from HA, D4-D7 scene keys S1-S4, D8-D13 backlight
  void leds(Rgb out[LEDS], uint32_t now) const {
    auto recent = [now](uint32_t t, uint32_t ms) { return t != 0 && now - t < ms; };
    for (int i = 0; i < LEDS; i++) out[i] = Rgb();
    // D1: off when everything is fine
    if (!connected_) out[0] = Rgb{60, 0, 0};                                        // no link to HA
    else if (!valid_ || warning_ || rooms_.empty()) out[0] = Rgb{60, 30, 0};      // menu missing, empty, cut
    if (recent(error_, ERROR_MS)) out[0] = Rgb{160, 0, 0};                        // menu action failed
    out[1] = rgb_(status_leds_[0]);
    out[2] = rgb_(status_leds_[1]);
    // D4-D7: on with the screen or just after a scene key
    bool awake = screen_on_ || recent(scene_wake_, SCENE_AWAKE_MS);
    for (int i = 0; i < 4; i++) {
      if (recent(result_[i], result_ok_[i] ? OK_MS : ERROR_MS)) {
        out[3 + i] = result_ok_[i] ? Rgb{0, 200, 0} : Rgb{200, 0, 0};
        continue;
      }
      if (!awake || scenes_[i].entity.empty()) continue;
      bool pending = recent(pressed_[i], PENDING_MS) && !answered_[i];
      out[3 + i] = (scenes_[i].active || pending) ? Rgb{255, 160, 60} : Rgb{25, 15, 5};
    }
    for (int i = 7; i < LEDS; i++) out[i] = rgb_(backlight_);
  }

  // "#rrggbb" x 13 separated by spaces, for the test bench
  std::string leds_text(uint32_t now) const {
    Rgb c[LEDS];
    leds(c, now);
    std::string out;
    char buf[9];
    for (int i = 0; i < LEDS; i++) {
      snprintf(buf, sizeof(buf), "%s#%02x%02x%02x", i ? " " : "", c[i].r, c[i].g, c[i].b);
      out += buf;
    }
    return out;
  }

 private:
  enum Level { ROOMS, TYPES, DEVICES, CONTROL };
  struct Screen {
    std::string title;
    std::vector<Line> rows;
    int selected = -1;
  };

  std::vector<Room> rooms_;
  std::vector<Colour> palette_;
  int row_ = 0;  // selected row in the screen of a light
  Scene scenes_[4];
  bool valid_ = false, warning_ = false, connected_ = false, screen_on_ = true;
  float backlight_[3] = {0, 0, 0}, status_leds_[2][3] = {{0, 0, 0}, {0, 0, 0}};
  uint32_t last_input_ = 0, error_ = 0, scene_wake_ = 0;   // times in ms, 0 = never
  uint32_t pressed_[4] = {0, 0, 0, 0}, result_[4] = {0, 0, 0, 0};
  bool result_ok_[4] = {false, false, false, false}, answered_[4] = {false, false, false, false};
  Level level_ = ROOMS;
  int cursor_[3] = {0, 0, 0};  // rooms, types, devices
  std::string sel_room_, sel_entity_;
  char sel_type_ = 0;

  static void set_(float *rgb, int channel, float level) {
    if (channel >= 0 && channel < 3) rgb[channel] = level < 0 ? 0 : level > 1 ? 1 : level;
  }

  static Rgb rgb_(const float *c) { return Rgb{uint8_t(c[0] * 255), uint8_t(c[1] * 255), uint8_t(c[2] * 255)}; }

  static const char *type_name_(char t) {
    return t == 'l' ? "Lights" : t == 'c' ? "Covers" : t == 'h' ? "Heating" : "?";
  }

  static std::string scene_action_(const std::string &entity) {
    std::string domain = entity.substr(0, entity.find('.'));
    if (domain == "scene" || domain == "script") return domain + ".turn_on";
    if (domain == "automation") return "automation.trigger";
    if (domain == "button" || domain == "input_button") return domain + ".press";
    return "homeassistant.turn_on";
  }

  static std::string fmt_(float v, const char *f) {
    char buf[16];
    snprintf(buf, sizeof(buf), f, v);
    return buf;
  }

  static std::string status_(const Device &d) {
    if (d.state == "unavailable" || d.state == "unknown") return "n/a";
    switch (d.type) {
      case 'l':
        if (d.state != "on") return "off";
        return std::isnan(d.value) ? "on" : fmt_(d.value, "%.0f%%");
      case 'c':
        if (d.state == "opening" || d.state == "closing") return d.state;
        if (!std::isnan(d.value) && d.value >= 0 && d.value <= 100) {
          return d.value == 0 ? "closed" : d.value == 100 ? "open" : fmt_(d.value, "%.0f%%");
        }
        return d.state;
      case 'h':
        if (d.state == "off") return "off";
        return std::isnan(d.value) ? d.state : fmt_(d.value, "%.1fC");
    }
    return d.state;
  }

  std::vector<char> types_(const Room &room) const {
    std::vector<char> out;
    for (char t : {'l', 'c', 'h'})
      for (const Device &d : room.devices)
        if (d.type == t) {
          out.push_back(t);
          break;
        }
    return out;
  }

  std::vector<const Device *> devices_(const Room &room, char type) const {
    std::vector<const Device *> out;
    for (const Device &d : room.devices)
      if (d.type == type) out.push_back(&d);
    return out;
  }

  const Room *room_() const {
    return cursor_[ROOMS] < int(rooms_.size()) ? &rooms_[cursor_[ROOMS]] : nullptr;
  }

  char type_() const {
    const Room *r = room_();
    if (!r) return 0;
    auto t = types_(*r);
    return cursor_[TYPES] < int(t.size()) ? t[cursor_[TYPES]] : 0;
  }

  Device *device_() {
    const Room *r = room_();
    if (!r) return nullptr;
    auto d = devices_(*r, type_());
    return cursor_[DEVICES] < int(d.size()) ? const_cast<Device *>(d[cursor_[DEVICES]]) : nullptr;
  }
  const Device *device_() const { return const_cast<Menu *>(this)->device_(); }

  int count_(Level l) const {
    const Room *r = room_();
    if (l == ROOMS) return rooms_.size();
    if (!r) return 0;
    if (l == TYPES) return types_(*r).size();
    return devices_(*r, type_()).size();
  }

  void remember_() {
    const Room *r = room_();
    sel_room_ = r ? r->name : "";
    sel_type_ = type_();
    const Device *d = device_();
    sel_entity_ = d ? d->entity : "";
  }

  // After new data from HA: keep room, type and device by name, not by position
  void restore_selection_() {
    cursor_[ROOMS] = cursor_[TYPES] = cursor_[DEVICES] = 0;
    for (size_t i = 0; i < rooms_.size(); i++)
      if (rooms_[i].name == sel_room_) cursor_[ROOMS] = i;
    if (const Room *r = room_()) {
      auto t = types_(*r);
      for (size_t i = 0; i < t.size(); i++)
        if (t[i] == sel_type_) cursor_[TYPES] = i;
      auto d = devices_(*r, type_());
      for (size_t i = 0; i < d.size(); i++)
        if (d[i]->entity == sel_entity_) cursor_[DEVICES] = i;
    }
    if (rooms_.empty() || (level_ == CONTROL && device_() && device_()->entity != sel_entity_)) level_ = ROOMS;
    if (const Device *d = device_()) row_ = std::max(0, std::min(row_, int(params_(*d).size()) - 1));
    remember_();
  }

  void browse_key_(Key k) {
    int n = count_(level_);
    int &c = cursor_[level_];
    switch (k) {
      case UP:
        if (n) c = (c + n - 1) % n;
        break;
      case DOWN:
        if (n) c = (c + 1) % n;
        break;
      case OK:
      case RIGHT:
        if (!n) break;
        if (level_ == ROOMS) {
          level_ = TYPES;
          cursor_[TYPES] = cursor_[DEVICES] = 0;
          if (count_(TYPES) == 1) level_ = DEVICES;  // skip a list with a single type
        } else if (level_ == TYPES) {
          level_ = DEVICES;
          cursor_[DEVICES] = 0;
        } else if (level_ == DEVICES) {
          level_ = CONTROL;
          row_ = 0;
        }
        break;
      case BACK:
      case LEFT:
        if (level_ == DEVICES) level_ = count_(TYPES) == 1 ? ROOMS : TYPES;
        else if (level_ == TYPES) level_ = ROOMS;
        break;
      default:
        break;
    }
    remember_();
  }

  void control_key_(Key k, std::vector<Call> &calls) {
    Device *d = device_();
    if (k == BACK || !d) {
      level_ = DEVICES;
      return;
    }
    const std::string &e = d->entity;
    switch (d->type) {
      case 'l':
        light_key_(*d, k, calls);
        break;
      case 'c':
        if (k == UP) calls.push_back({"cover.open_cover", e, "", ""});
        else if (k == DOWN) calls.push_back({"cover.close_cover", e, "", ""});
        else if (k == OK) calls.push_back({"cover.stop_cover", e, "", ""});
        break;
      case 'h':
        if (k == OK) {
          calls.push_back({"climate.toggle", e, "", ""});
        } else if ((k == LEFT || k == RIGHT) && !std::isnan(d->value)) {
          d->value += k == LEFT ? -0.5f : 0.5f;
          calls.push_back({"climate.set_temperature", e, "temperature", fmt_(d->value, "%.1f")});
        } else {
          move_device_(k);
        }
        break;
    }
  }

  // Rows of the screen of a light: 'b' brightness (or 's' just on/off), then colour, white
  // temperature and effect when the light supports them
  static std::vector<char> params_(const Device &d) {
    if (d.type != 'l') return {};
    std::vector<char> out{d.modes.find('b') != std::string::npos ? 'b' : 's'};
    for (char m : {'c', 't', 'e'})
      if (d.modes.find(m) != std::string::npos) out.push_back(m);
    return out;
  }

  // Palette colour closest to the current colour of the light (-1 if unknown)
  int nearest_colour_(const Device &d) const {
    if (palette_.empty() || std::isnan(d.hue) || std::isnan(d.sat)) return -1;
    auto xy = [](float h, float s, float &x, float &y) {
      x = s * std::cos(h * 3.14159265f / 180);
      y = s * std::sin(h * 3.14159265f / 180);
    };
    float x, y;
    xy(d.hue, d.sat, x, y);
    int best = 0;
    float dist = 1e9;
    for (size_t i = 0; i < palette_.size(); i++) {
      float px, py;
      xy(palette_[i].hue, palette_[i].sat, px, py);
      float dd = (px - x) * (px - x) + (py - y) * (py - y);
      if (dd < dist) dist = dd, best = i;
    }
    return best;
  }

  // Light: Up/Down through the rows and on to the previous/next light (continuous scrolling),
  // Left/Right change the selected row, OK on/off
  void light_key_(Device &d, Key k, std::vector<Call> &calls) {
    const std::string &e = d.entity;
    auto rows = params_(d);
    row_ = std::max(0, std::min(row_, int(rows.size()) - 1));
    if (k == OK) {
      calls.push_back({"light.toggle", e, "", ""});
      d.state = d.state == "on" ? "off" : "on";  // optimistic, HA confirms
      return;
    }
    if (k == DOWN) {
      if (row_ < int(rows.size()) - 1) {
        row_++;
      } else {
        move_device_(DOWN);
        row_ = 0;
      }
      return;
    }
    if (k == UP) {
      if (row_ > 0) {
        row_--;
      } else {
        move_device_(UP);
        const Device *p = device_();
        row_ = p ? int(params_(*p).size()) - 1 : 0;
      }
      return;
    }
    if (k != LEFT && k != RIGHT) return;
    int dir = k == LEFT ? -1 : 1;
    switch (rows[row_]) {
      case 's':
        calls.push_back({dir > 0 ? "light.turn_on" : "light.turn_off", e, "", ""});
        d.state = dir > 0 ? "on" : "off";
        break;
      case 'b':
        calls.push_back({"light.turn_on", e, "brightness_step_pct", dir < 0 ? "-10" : "10"});
        if (!std::isnan(d.value)) d.value = std::fmax(1, std::fmin(100, d.value + 10 * dir));
        d.state = "on";
        break;
      case 'c': {
        if (palette_.empty()) break;
        int n = palette_.size(), i = nearest_colour_(d);
        i = i < 0 ? (dir > 0 ? 0 : n - 1) : (i + dir + n) % n;
        calls.push_back({"light.turn_on", e, "hs_color", fmt_(palette_[i].hue, "%.0f") + "," + fmt_(palette_[i].sat, "%.0f")});
        d.hue = palette_[i].hue;
        d.sat = palette_[i].sat;
        d.state = "on";
        break;
      }
      case 't': {
        float kv = std::isnan(d.kelvin) ? 2700 : d.kelvin;
        kv = std::fmax(d.kmin, std::fmin(d.kmax, kv + 500 * dir));
        calls.push_back({"light.turn_on", e, "color_temp_kelvin", fmt_(kv, "%.0f")});
        d.kelvin = kv;
        d.state = "on";
        break;
      }
      case 'e':
        calls.push_back({"light.turn_on", e, "effect_step", dir < 0 ? "-1" : "1"});   // HA picks the effect
        d.state = "on";
        break;
    }
  }

  // Up/Down on a light or a thermostat: previous/next device of the same type
  void move_device_(Key k) {
    int n = count_(DEVICES);
    if (!n || (k != UP && k != DOWN)) return;
    cursor_[DEVICES] = (cursor_[DEVICES] + (k == UP ? n - 1 : 1)) % n;
    remember_();
  }

  // Scroll so that the cursor stays visible
  static void page_(Screen &s, int cursor, int total) {
    int first = cursor < ROWS ? 0 : cursor - ROWS + 1;
    std::vector<Line> rows;
    for (int i = first; i < total && i < first + ROWS; i++) rows.push_back(s.rows[i]);
    s.rows = rows;
    s.selected = cursor - first;
  }

  Screen screen_() const {
    Screen s;
    if (!valid_) {
      s.title = "PlancH";
      s.rows = {{connected_ ? "Waiting for" : "Not connected", ""}, {connected_ ? "Home Assistant" : "to Home Assistant", ""}};
      return s;
    }
    if (rooms_.empty()) {
      s.title = "PlancH";
      s.rows = {{"No devices:", ""}, {"add the label", ""}, {"PlancH in HA", ""}};
      return s;
    }
    const Room *r = room_();
    switch (level_) {
      case ROOMS:
        s.title = warning_ ? "Rooms (limit!)" : "Rooms";
        for (const Room &room : rooms_) s.rows.push_back({room.name, std::to_string(room.devices.size())});
        page_(s, cursor_[ROOMS], rooms_.size());
        break;
      case TYPES:
        s.title = r->name;
        for (char t : types_(*r)) s.rows.push_back({type_name_(t), std::to_string(devices_(*r, t).size())});
        page_(s, cursor_[TYPES], s.rows.size());
        break;
      case DEVICES: {
        s.title = r->name + " / " + type_name_(type_());
        auto list = devices_(*r, type_());
        for (const Device *d : list) s.rows.push_back({d->name, status_(*d)});
        page_(s, cursor_[DEVICES], list.size());
        break;
      }
      case CONTROL: {
        const Device *d = device_();
        if (!d) break;
        s.title = d->name;
        if (d->type == 'l') {
          for (char p : params_(*d)) {
            if (p == 'b') s.rows.push_back({"Brightness", status_(*d)});
            if (p == 's') s.rows.push_back({"State", status_(*d)});
            if (p == 'c') {
              int i = nearest_colour_(*d);
              s.rows.push_back({"Colour", i < 0 ? "-" : palette_[i].name});
            }
            if (p == 't') s.rows.push_back({"White", std::isnan(d->kelvin) ? "-" : fmt_(d->kelvin, "%.0fK")});
            if (p == 'e') s.rows.push_back({"Effect", d->effect.empty() ? "none" : d->effect.substr(0, 11)});
          }
          page_(s, row_, s.rows.size());
          break;
        }
        s.rows.push_back({"State", status_(*d)});
        if (d->type == 'c') {
          s.rows.push_back({"Up/Down", "open/close"});
          s.rows.push_back({"OK", "stop"});
        } else if (d->type == 'h') {
          if (!std::isnan(d->current)) s.rows.push_back({"Now", fmt_(d->current, "%.1fC")});
          s.rows.push_back({"OK", "on/off"});
          s.rows.push_back({"< >", "-/+ 0.5C"});
        }
        break;
      }
    }
    return s;
  }
};

inline Menu &menu() {
  static Menu m;
  return m;
}

}  // namespace planch
