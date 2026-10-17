// PlancH Arcade: the hidden games of the panel.
// OK and Back held together for 3 s open the list of games (only where the board reports the key
// releases: enable()); Up/Down choose, OK starts. Back held for 3 s leaves from anywhere, with a bar
// filling the yellow band; a short Back reaches the game on release (pause, or back to the list).
// Outside the arcade every key goes to the menu, as before.
#pragma once

#include "echo.h"
#include "game.h"
#include "invaders.h"
#include "menu.h"

#include <vector>

namespace planch {

class Arcade : public Bitmap {
 public:
  static constexpr uint32_t HOLD_MS = 3000;   // OK + Back to open, Back to leave
  static constexpr uint32_t IDLE_MS = 60000;  // no keys outside a match: back to the menu
  static constexpr int GAMES = 2;

  void enable() { enabled_ = true; }
  bool active() const { return mode_ != OFF; }
  int games() const { return GAMES; }
  Game &game(int i) { return *games_[i]; }
  uint32_t best(int i) const { return games_[i]->best(); }
  void set_best(int i, uint32_t b) { games_[i]->set_best(b); }

  // Key pressed: in the menu it returns the Home Assistant actions to perform
  std::vector<Call> press(Key k, uint32_t now) {
    held_[k] = true;
    since_[k] = now;
    last_ = now;
    if (mode_ == OFF) return menu().key(k, now);
    if (k == BACK) return {};  // handled on release (short) or in loop() (held)
    if (mode_ == LIST) list_key_(k, now);
    else games_[current_]->press(k, now);
    return {};
  }

  void release(Key k, uint32_t now) {
    bool was_held = held_[k];
    held_[k] = false;
    if (mode_ == OFF || !was_held) return;
    if (k == BACK) {
      if (now - since_[BACK] < HOLD_MS && mode_ == GAME) {  // short press: the game decides
        games_[current_]->press(BACK, now);
        games_[current_]->release(BACK);
      }
      return;
    }
    if (mode_ == GAME) games_[current_]->release(k);
  }

  // Called every few ms: opens and closes the arcade, runs the game; true while it is on screen
  bool loop(uint32_t now) {
    if (mode_ == OFF) {
      if (enabled_ && held_[OK] && held_[BACK]) {
        if (!both_) both_ = now | 1;
        else if (now - both_ >= HOLD_MS) {
          both_ = 0;
          mode_ = LIST;
          last_ = now;
          held_[BACK] = false;  // the Back of the opening does not count as a hold to leave
          held_[OK] = false;
        }
      } else {
        both_ = 0;
      }
      if (mode_ == OFF) return false;
    }
    if (held_[BACK] && now - since_[BACK] >= HOLD_MS) return leave_(now);
    if (mode_ == GAME) {
      Game &g = *games_[current_];
      g.update(now);
      if (g.wants_list()) mode_ = LIST;
      else if (!g.playing() && now - last_ > IDLE_MS) return leave_(now);
    } else if (now - last_ > IDLE_MS) {
      return leave_(now);
    }
    render_(now);
    return true;
  }

  void leds(Rgb out[Game::LEDS], uint32_t now) const {
    if (mode_ == GAME) return games_[current_]->leds(out, now);
    if (mode_ == OFF) return menu().leds(out, now);
    // List: a slow rainbow on the backlight, the scene LEDs off
    for (int i = 0; i < Game::LEDS; i++) out[i] = Rgb{};
    float h = (now % 6000) / 1000.0f;  // 0-6
    auto ch = [&](float o) {
      float d = h - o;
      while (d < 0) d += 6;
      float v = d < 1 ? d : d < 3 ? 1 : d < 4 ? 4 - d : 0;
      return uint8_t(60 * v);
    };
    Rgb bl{ch(0), ch(2), ch(4)};
    for (int i = 7; i < Game::LEDS; i++) out[i] = bl;
  }

 private:
  enum Mode { OFF, LIST, GAME };

  void list_key_(Key k, uint32_t now) {
    if (k == UP) cursor_ = (cursor_ + GAMES - 1) % GAMES;
    if (k == DOWN) cursor_ = (cursor_ + 1) % GAMES;
    if (k == OK) {
      current_ = cursor_;
      mode_ = GAME;
      games_[current_]->open(now);
    }
  }

  bool leave_(uint32_t now) {
    mode_ = OFF;
    for (auto &h : held_) h = false;
    menu().key(TOUCH, now);  // back to the menu with the screen on
    return false;
  }

  void render_(uint32_t now) {
    if (mode_ == GAME) {
      std::memcpy(fb_, games_[current_]->fb(), sizeof(fb_));
    } else {
      clear_();
      centre_text_(5, "PLANCH ARCADE", 1);
      for (int i = 0; i < GAMES; i++) {
        int y = 18 + i * 9;
        bool sel = i == cursor_;
        if (sel) fill_(0, y - 2, W, 9, true);
        text_(4, y, games_[i]->name(), 1, sel);
        std::string b = num_(games_[i]->best(), 4);
        text_(W - 4 - text_w_(b, 1), y, b, 1, sel);
      }
      centre_text_(57, (now / 2500) % 2 ? "HOLD BACK: EXIT" : "OK: PLAY", 1);
    }
    // Back held: a bar filling the bottom of the yellow band, then the arcade closes
    if (held_[BACK] && now - since_[BACK] > 300) {
      int w = int(W * std::min<uint32_t>(now - since_[BACK], HOLD_MS) / HOLD_MS);
      fill_(0, 14, W, 2, false);
      fill_(0, 14, w, 2, true);
    }
  }

  // Order of the list; the best scores are kept by position: add new games at the end
  Invaders invaders_;
  Echo echo_;
  Game *games_[GAMES] = {&invaders_, &echo_};
  Mode mode_ = OFF;
  bool enabled_ = false;
  bool held_[TOUCH + 1] = {};
  uint32_t since_[TOUCH + 1] = {};
  uint32_t both_ = 0, last_ = 0;
  int cursor_ = 0, current_ = 0;
};

inline Arcade &arcade() {
  static Arcade a;
  return a;
}

}  // namespace planch
