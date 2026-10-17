// Echo: a memory game of PlancH Arcade (arcade.h), in the spirit of the classic colour-sequence toys.
// The panel plays a sequence on the scene LEDs D4-D7 (and on four pads of the display, at the height
// of the keys S1-S4); the player repeats it with the scene keys. One more step and a faster sequence
// at every round; a wrong key or 5 s without keys end the match. Score: rounds completed.
#pragma once

#include "game.h"

namespace planch {

class Echo : public Game {
 public:
  enum State { TITLE, SHOW, INPUT, GOOD, FAIL, OVER };

  const char *name() const override { return "ECHO"; }

  void open(uint32_t now) override {
    state_ = TITLE;
    since_ = now;
    list_ = false;
    lit_ = -1;
    render_(now);
  }

  bool wants_list() const override { return list_; }
  bool playing() const override { return state_ != TITLE && state_ != OVER; }
  State state() const { return state_; }
  int round() const { return len_ - 1; }  // rounds completed so far
  // Pad lit now (0-3 = S1-S4), -1 none: what a player watching the panel sees
  int lit() const { return lit_; }

  void press(Key k, uint32_t now) override {
    if (k == BACK) {  // short Back: back to the list of games
      list_ = true;
      return;
    }
    if (k == OK && (state_ == TITLE || (state_ == OVER && now - since_ > 1000))) {
      start_(now);
      return;
    }
    if (state_ != INPUT || k < S1 || k > S4) return;
    int pad = k - S1;
    lit_ = pad;
    lit_until_ = now + 250;
    pressed_at_ = now;
    if (pad != seq_[pos_]) {
      wrong_ = pad;
      state_ = FAIL;
      since_ = now;
      return;
    }
    if (++pos_ == len_) {
      if (uint32_t(len_) > best_) best_ = len_;
      state_ = GOOD;
      since_ = now;
    }
  }

  void release(Key k) override {
    if (k >= S1 && k <= S4 && lit_ == k - S1 && state_ == INPUT) lit_until_ = 1;  // off at the next update
  }

  void update(uint32_t now) override {
    switch (state_) {
      case SHOW: {
        // Each step: the pad on for on_ms, then a short gap
        if (int32_t(now - since_) < 0) {  // short pause before the sequence
          lit_ = -1;
          break;
        }
        uint32_t t = now - since_, step = on_ms_() + GAP_MS;
        int i = t / step;
        if (i >= len_) {
          state_ = INPUT;
          pos_ = 0;
          lit_ = -1;
          pressed_at_ = now;
          break;
        }
        lit_ = (t % step) < on_ms_() ? seq_[i] : -1;
        break;
      }
      case INPUT:
        if (lit_ >= 0 && lit_until_ && now >= lit_until_) lit_ = -1;
        if (now - pressed_at_ > TIMEOUT_MS) {
          wrong_ = -1;
          state_ = FAIL;
          since_ = now;
        }
        break;
      case GOOD:
        if (now - since_ > 800) {
          if (len_ < MAX_LEN) len_++;
          show_(now);
        }
        break;
      case FAIL:
        lit_ = (now / 150) % 2 ? seq_[pos_] : -1;  // the key that was expected blinks
        if (now - since_ > 1800) {
          state_ = OVER;
          since_ = now;
          lit_ = -1;
        }
        break;
      default:
        break;
    }
    render_(now);
  }

  void leds(Rgb out[LEDS], uint32_t now) const override {
    for (int i = 0; i < LEDS; i++) out[i] = Rgb{};
    // D1-D3: a medal every 5 rounds
    if (playing() || state_ == OVER)
      for (int i = 0; i < 3; i++)
        if (round() >= 5 * (i + 1)) out[i] = Rgb{200, 140, 0};
    // D4-D7: the pad lit in full; during the player's turn the four colours dimly, as a reminder
    for (int p = 0; p < 4; p++) {
      if (lit_ == p) out[3 + p] = PAD[p];
      else if (state_ == INPUT || state_ == TITLE) out[3 + p] = dim_(PAD[p], 12);
    }
    // Backlight: green on a round passed, red on a mistake, the four colours in turn on the title
    Rgb bl{};
    if (state_ == GOOD) bl = dim_(Rgb{0, 255, 0}, int(255 * (1 - std::min(1.0f, (now - since_) / 800.0f))));
    if (state_ == FAIL) bl = (now / 150) % 2 ? Rgb{255, 0, 0} : Rgb{60, 0, 0};
    if (state_ == TITLE) bl = dim_(PAD[(now / 1200) % 4], 40 + int(40 * tri_(now, 1200)));
    if (state_ == OVER) bl = Rgb{uint8_t(30 + 120 * tri_(now, 2000)), 0, 0};
    for (int i = 7; i < LEDS; i++) out[i] = bl;
  }

 private:
  static constexpr int MAX_LEN = 99;
  static constexpr uint32_t GAP_MS = 120, TIMEOUT_MS = 5000;
  // Colours of the scene keys S1-S4: green, red, yellow, blue
  static constexpr Rgb PAD[4] = {Rgb{0, 220, 0}, Rgb{255, 0, 0}, Rgb{255, 190, 0}, Rgb{0, 70, 255}};

  // Shorter flashes as the sequence grows: 520 ms down to 220 ms
  uint32_t on_ms_() const { return uint32_t(std::max(220, 520 - 25 * (len_ - 1))); }

  static Rgb dim_(Rgb c, int level) {
    return Rgb{uint8_t(c.r * level / 255), uint8_t(c.g * level / 255), uint8_t(c.b * level / 255)};
  }

  void start_(uint32_t now) {
    best_at_start_ = best_;
    rng_ ^= now * 2654435761u;  // a different sequence every match
    for (int i = 0; i < MAX_LEN; i++) seq_[i] = rnd_() % 4;
    len_ = 1;
    show_(now);
  }

  void show_(uint32_t now) {
    state_ = SHOW;
    since_ = now + 500;  // a short pause before the sequence
    lit_ = -1;
    pos_ = 0;
  }

  void render_(uint32_t now) {
    clear_();
    if (state_ == TITLE) {
      centre_text_(5, "ECHO", 1);
      centre_text_(20, "REPEAT THE COLOURS", 1);
      centre_text_(28, "WITH THE SCENE KEYS", 1);
      centre_text_(40, "BEST " + num_(best_, 2), 1);
      if ((now / 500) % 2) centre_text_(54, "PRESS OK", 1);
      return;
    }
    text_(0, 1, "ROUND " + num_(std::max(0, state_ == OVER ? round() : round() + 1), 2), 1);
    std::string hi = "BEST " + num_(best_, 2);
    text_(W - text_w_(hi, 1), 1, hi, 1);
    if (state_ == OVER) {
      centre_text_(22, "GAME OVER", 2);
      centre_text_(40, uint32_t(round()) > best_at_start_ ? "NEW BEST" : "ROUNDS " + num_(round(), 0), 1);
      if (now - since_ > 1000) centre_text_(54, "OK PLAY   BACK LIST", 1);
      return;
    }
    // Four pads at the height of the keys S1-S4: full when lit, outlined otherwise
    for (int p = 0; p < 4; p++) {
      int y = 17 + p * 12;
      if (lit_ == p) {
        fill_(2, y, 82, 10, true);
        text_(6, y + 3, std::to_string(p + 1), 1, true);
      } else {
        for (int x = 2; x < 84; x++) set_(x, y), set_(x, y + 9);
        for (int yy = y; yy < y + 10; yy++) set_(2, yy), set_(83, yy);
        text_(6, y + 3, std::to_string(p + 1), 1);
      }
    }
    // Message on the right
    if (state_ == SHOW) {
      text_(90, 30, "WATCH", 1);
    } else if (state_ == INPUT) {
      text_(94, 26, "YOUR", 1);
      text_(94, 34, "TURN", 1);
      text_(98, 46, num_(pos_, 0) + "/" + num_(len_, 0), 1);
    } else if (state_ == GOOD) {
      text_(94, 30, "GOOD", 1);
    } else if (state_ == FAIL) {
      text_(90, 26, wrong_ < 0 ? "TIME" : "OOPS", 1);
      text_(90, 34, wrong_ < 0 ? "OUT" : "", 1);
    }
  }

  State state_ = TITLE;
  bool list_ = false;
  uint8_t seq_[MAX_LEN] = {};
  int len_ = 1, pos_ = 0, lit_ = -1, wrong_ = -1;
  uint32_t since_ = 0, lit_until_ = 0, pressed_at_ = 0, best_at_start_ = 0;
};

}  // namespace planch
