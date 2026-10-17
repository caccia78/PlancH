// Reflex: a reaction-time game of PlancH Arcade (arcade.h). After a random wait one of the scene keys
// lights up (LEDs D4-D7, colours of Echo, and a pad of the display at the height of the key); the
// player presses it as fast as possible. 10 rounds; a key during the wait repeats the round, a wrong
// key or 1 s without keys counts 1000 ms. Score: the average time, best = the lowest.
#pragma once

#include "game.h"

namespace planch {

class Reflex : public Game {
 public:
  enum State { TITLE, WAIT, GO, HIT, MISS, EARLY, OVER };
  static constexpr int ROUNDS = 10;

  const char *name() const override { return "REFLEX"; }
  std::string score_text() const override { return best_ ? num_(best_, 0) + "MS" : "---"; }

  void open(uint32_t now) override {
    state_ = TITLE;
    since_ = now;
    list_ = false;
    paused_ = false;
    render_(now);
  }

  bool wants_list() const override { return list_; }
  bool playing() const override { return state_ != TITLE && state_ != OVER; }
  State state() const { return state_; }
  bool paused() const { return paused_; }
  // Key to press now (0-3 = S1-S4), -1 none: what a player watching the panel sees
  int target() const { return state_ == GO && !paused_ ? target_ : -1; }
  uint32_t average() const { return average_; }

  void press(Key k, uint32_t now) override {
    if (k == BACK) {  // short Back: pause during a match, otherwise back to the list of games
      if (playing()) paused_ = !paused_;
      else list_ = true;
      if (!paused_ && playing()) resume_(now);
      return;
    }
    if (k == OK) {
      if (paused_) {
        paused_ = false;
        resume_(now);
      } else if (state_ == TITLE || (state_ == OVER && now - since_ > 1000)) {
        start_(now);
      }
      return;
    }
    if (paused_ || k < S1 || k > S4) return;
    int pad = k - S1;
    if (state_ == WAIT) {
      state_ = EARLY;
      since_ = now;
    } else if (state_ == GO) {
      wrong_ = pad != target_;
      record_(wrong_ ? MISS_MS : std::max<uint32_t>(1, now - lit_at_), now);
    }
  }

  void release(Key k) override {}

  void update(uint32_t now) override {
    if (!paused_) switch (state_) {
      case WAIT:
        if (int32_t(now - go_at_) >= 0) {
          state_ = GO;
          lit_at_ = now;
        }
        break;
      case GO:
        if (now - lit_at_ >= MISS_MS) {
          wrong_ = false;
          record_(MISS_MS, now);
        }
        break;
      case HIT:
      case MISS:
        if (now - since_ > 1000) {
          if (round_ < ROUNDS) {
            wait_(now);
          } else {
            uint32_t sum = 0;
            for (uint32_t t : times_) sum += t;
            average_ = (sum + ROUNDS / 2) / ROUNDS;
            new_best_ = !best_ || average_ < best_;
            if (new_best_) best_ = average_;
            state_ = OVER;
            since_ = now;
          }
        }
        break;
      case EARLY:
        if (now - since_ > 1200) wait_(now);
        break;
      default:
        break;
    }
    render_(now);
  }

  void leds(Rgb out[LEDS], uint32_t now) const override {
    for (int i = 0; i < LEDS; i++) out[i] = Rgb{};
    if (state_ == TITLE) {  // the four colours in turn
      int p = (now / 400) % 4;
      out[3 + p] = PAD[p];
      for (int i = 7; i < LEDS; i++) out[i] = dim_(PAD[p], 40);
      return;
    }
    if (paused_) {
      for (int i = 7; i < LEDS; i++) out[i] = Rgb{0, 0, 30};
      return;
    }
    Rgb bl{};
    if (state_ == GO) out[3 + target_] = PAD[target_];
    if (state_ == HIT) {
      out[3 + target_] = dim_(PAD[target_], 60);
      // D1-D3: how fast the round was
      int bars = last_ < 250 ? 3 : last_ < 350 ? 2 : last_ < 500 ? 1 : 0;
      for (int i = 0; i < bars; i++) out[i] = Rgb{0, 220, 0};
      float fade = 1 - std::min(1.0f, (now - since_) / 1000.0f);
      bl = dim_(speed_colour_(last_), int(255 * fade));
    }
    if (state_ == MISS) {
      if ((now / 150) % 2) out[3 + target_] = PAD[target_];  // the key that was expected blinks
      bl = (now / 150) % 2 ? Rgb{255, 0, 0} : Rgb{60, 0, 0};
    }
    if (state_ == EARLY) bl = (now / 150) % 2 ? Rgb{255, 0, 0} : Rgb{60, 0, 0};
    if (state_ == OVER) bl = dim_(speed_colour_(average_), 40 + int(110 * tri_(now, 2000)));
    for (int i = 7; i < LEDS; i++) out[i] = bl;
  }

 private:
  static constexpr uint32_t MISS_MS = 1000;
  // Colours of the scene keys S1-S4, as in Echo: green, red, yellow, blue
  static constexpr Rgb PAD[4] = {Rgb{0, 220, 0}, Rgb{255, 0, 0}, Rgb{255, 190, 0}, Rgb{0, 70, 255}};

  static Rgb dim_(Rgb c, int level) {
    return Rgb{uint8_t(c.r * level / 255), uint8_t(c.g * level / 255), uint8_t(c.b * level / 255)};
  }
  // Green under 250 ms, yellow under 350, orange above
  static Rgb speed_colour_(uint32_t ms) {
    return ms < 250 ? Rgb{0, 255, 0} : ms < 350 ? Rgb{255, 200, 0} : Rgb{255, 80, 0};
  }

  void start_(uint32_t now) {
    rng_ ^= now * 2654435761u;  // different waits and keys every match
    round_ = 0;
    fastest_ = 0;
    new_best_ = false;
    wait_(now);
  }

  // Next round: 1 to 3 s of wait, then a random key
  void wait_(uint32_t now) {
    state_ = WAIT;
    since_ = now;
    go_at_ = now + 1000 + rnd_() % 2000;
    target_ = rnd_() % 4;
  }

  // Back from the pause: a round that was waiting or lit starts again with a new wait
  void resume_(uint32_t now) {
    if (state_ == WAIT || state_ == GO) wait_(now);
    else since_ = now;
  }

  void record_(uint32_t ms, uint32_t now) {
    last_ = ms;
    times_[round_++] = ms;
    if (ms < MISS_MS && (!fastest_ || ms < fastest_)) fastest_ = ms;
    state_ = ms < MISS_MS ? HIT : MISS;
    since_ = now;
  }

  void render_(uint32_t now) {
    clear_();
    std::string hi = "BEST " + (best_ ? num_(best_, 0) : std::string("---"));
    if (state_ == TITLE) {
      centre_text_(5, "REFLEX", 1);
      centre_text_(20, "PRESS THE SCENE KEY", 1);
      centre_text_(28, "THAT LIGHTS UP", 1);
      centre_text_(40, best_ ? hi + " MS" : hi, 1);
      if ((now / 500) % 2) centre_text_(54, "PRESS OK", 1);
      return;
    }
    text_(0, 1, "ROUND " + num_(std::min(round_ + (state_ == WAIT || state_ == GO || state_ == EARLY), ROUNDS), 2) + "/" +
                    num_(ROUNDS, 2), 1);
    text_(W - text_w_(hi, 1), 1, hi, 1);
    if (state_ == OVER) {
      centre_text_(18, "AVERAGE", 1);
      centre_text_(26, num_(average_, 0) + " MS", 2);
      centre_text_(42, new_best_ ? "NEW BEST" : fastest_ ? "FASTEST " + num_(fastest_, 0) + " MS" : "", 1);
      if (now - since_ > 1000) centre_text_(54, "OK PLAY   BACK LIST", 1);
      return;
    }
    // Four pads at the height of the keys S1-S4: full when lit, outlined otherwise
    int lit = state_ == GO || state_ == HIT ? target_ : state_ == MISS && (now / 150) % 2 ? target_ : -1;
    for (int p = 0; p < 4; p++) {
      int y = 17 + p * 12;
      if (lit == p) {
        fill_(2, y, 82, 10, true);
        text_(6, y + 3, std::to_string(p + 1), 1, true);
      } else {
        for (int x = 2; x < 84; x++) set_(x, y), set_(x, y + 9);
        for (int yy = y; yy < y + 10; yy++) set_(2, yy), set_(83, yy);
        text_(6, y + 3, std::to_string(p + 1), 1);
      }
    }
    // Message on the right, centred in x 86-127
    auto right = [&](int y, const std::string &t, int scale) { text_(86 + (42 - text_w_(t, scale)) / 2, y, t, scale); };
    if (paused_) {
      pause_panel_();
    } else if (state_ == WAIT) {
      right(30, std::string("WAIT") + std::string((now - since_) / 400 % 4, '.'), 1);
    } else if (state_ == GO) {
      right(30, "NOW!", 1);
    } else if (state_ == HIT) {
      right(24, num_(last_, 0), 2);
      right(40, "MS", 1);
    } else if (state_ == MISS) {
      right(24, wrong_ ? "WRONG" : "SLOW", 1);
      right(34, "1000 MS", 1);
    } else if (state_ == EARLY) {
      right(26, "TOO", 1);
      right(34, "SOON", 1);
    }
  }

  State state_ = TITLE;
  bool list_ = false, paused_ = false, wrong_ = false, new_best_ = false;
  int round_ = 0, target_ = 0;
  uint32_t times_[ROUNDS] = {};
  uint32_t since_ = 0, go_at_ = 0, lit_at_ = 0, last_ = 0, fastest_ = 0, average_ = 0;
};

}  // namespace planch
