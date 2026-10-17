// Dash: an endless runner of PlancH Arcade (arcade.h). The runner goes by itself on the ground line
// of the blue part of the OLED; Up, OK or a scene key jump (held: higher), Down ducks (in the air:
// falls faster). Crates and posts from the start, holes in the ground from 150 m, birds from 300 m.
// The speed grows with the distance; one hit ends the run. Score: the metres run (10 px = 1 m).
// LEDs: D1-D3 the speed level, D4-D7 the progress to the next 100 m, the backlight goes from day to
// night every 500 m and flashes red on the crash.
#pragma once

#include "game.h"

namespace planch {

class Dash : public Game {
 public:
  enum State { TITLE, RUN, CRASH, OVER };

  const char *name() const override { return "DASH"; }

  void open(uint32_t now) override {
    state_ = TITLE;
    since_ = now;
    list_ = false;
    paused_ = false;
    render_(now);
  }

  bool wants_list() const override { return list_; }
  bool playing() const override { return state_ == RUN || state_ == CRASH; }
  State state() const { return state_; }
  bool paused() const { return paused_; }
  uint32_t metres() const { return uint32_t(dist_ / 10); }
  // For a player watching the panel: on the ground, and the next obstacle ahead (x, -1 none)
  bool on_ground() const { return !air_; }
  int next_obstacle(int *kind = nullptr, int *w = nullptr) const {
    int best = -1;
    for (const auto &o : obs_)
      if (o.w && o.x + o.w > RX && (best < 0 || o.x < obs_[best].x)) best = &o - obs_;
    if (best < 0) return -1;
    if (kind) *kind = obs_[best].kind;
    if (w) *w = obs_[best].w;
    return int(obs_[best].x);
  }

  void press(Key k, uint32_t now) override {
    if (k == BACK) {  // short Back: pause during a run, otherwise back to the list of games
      if (playing()) paused_ = !paused_;
      else list_ = true;
      if (!paused_) t_ = now;
      return;
    }
    if (k == OK) {
      if (paused_) {
        paused_ = false;
        t_ = now;
        return;
      }
      if (state_ == TITLE || (state_ == OVER && now - since_ > 1000)) return start_(now);
    }
    if (paused_ || state_ != RUN) return;
    if (k == UP || k == OK || (k >= S1 && k <= S4)) {
      jump_held_ = true;
      if (!air_ && !duck_) {
        air_ = true;
        vy_ = -JUMP_V;
        hold_ms_ = 0;
      }
    }
    if (k == DOWN) duck_ = true;
  }

  void release(Key k) override {
    if (k == UP || k == OK || (k >= S1 && k <= S4)) jump_held_ = false;
    if (k == DOWN) duck_ = false;
  }

  void update(uint32_t now) override {
    if (paused_) {
      t_ = now;
    } else if (state_ == RUN || state_ == CRASH) {
      // Fixed 10 ms steps: the same jump at any frame rate
      for (int n = 0; int32_t(now - t_) >= int32_t(STEP_MS) && n < 20; n++) {
        t_ += STEP_MS;
        step_();
      }
      if (int32_t(now - t_) >= int32_t(STEP_MS)) t_ = now;
      if (state_ == CRASH && now - since_ > 1200) {
        state_ = OVER;
        since_ = now;
        new_best_ = metres() > best_;
        if (new_best_) best_ = metres();
      }
    }
    render_(now);
  }

  void leds(Rgb out[LEDS], uint32_t now) const override {
    for (int i = 0; i < LEDS; i++) out[i] = Rgb{};
    if (state_ == TITLE) {  // a light running down the scene keys
      out[3 + (now / 150) % 4] = Rgb{0, 220, 0};
      for (int i = 7; i < LEDS; i++) out[i] = dim_(DAY, 40 + int(60 * tri_(now, 3000)));
      return;
    }
    if (paused_) {
      for (int i = 7; i < LEDS; i++) out[i] = Rgb{0, 0, 30};
      return;
    }
    Rgb bl = sky_();
    if (state_ == RUN) {
      // D1-D3: speed level, green, yellow, red
      int level = speed_ < 100 ? 1 : speed_ < 135 ? 2 : 3;
      static constexpr Rgb LEVEL[3] = {Rgb{0, 220, 0}, Rgb{255, 190, 0}, Rgb{255, 0, 0}};
      for (int i = 0; i < level; i++) out[i] = LEVEL[level - 1];
      // D4-D7: the next 100 m, all white for a moment at every hundred
      uint32_t m = metres();
      if (m >= 100 && m % 100 < 4) {
        for (int i = 3; i < 7; i++) out[i] = Rgb{255, 255, 255};
      } else {
        for (int i = 0; i < int(m % 100) / 25; i++) out[3 + i] = Rgb{0, 160, 255};
      }
    }
    if (state_ == CRASH) {
      bool on = (now / 120) % 2;
      for (int i = 0; i < 3; i++) out[i] = on ? Rgb{255, 0, 0} : Rgb{};
      bl = on ? Rgb{255, 0, 0} : Rgb{60, 0, 0};
    }
    if (state_ == OVER) bl = dim_(bl, 80 + int(175 * tri_(now, 2000)));
    for (int i = 7; i < LEDS; i++) out[i] = bl;
  }

 private:
  enum Kind { CRATE, POST, HOLE, BIRD };
  struct Obstacle {
    float x = 0;
    int w = 0, h = 0, top = 0;  // w = 0: free slot
    Kind kind = CRATE;
  };
  static constexpr int GROUND = 58;  // row of the ground line; the runner's feet stand on row 57
  static constexpr int RX = 14;      // left column of the runner
  static constexpr uint32_t STEP_MS = 10;
  static constexpr float JUMP_V = 150, G_HOLD = 300, G = 900, G_DUCK = 1600, HOLD_MS = 180;
  static constexpr Rgb DAY{255, 140, 40}, NIGHT{30, 40, 160};

  static Rgb dim_(Rgb c, int level) {
    return Rgb{uint8_t(c.r * level / 255), uint8_t(c.g * level / 255), uint8_t(c.b * level / 255)};
  }
  // Backlight: day and night every 500 m, with a 50 m dusk
  Rgb sky_() const {
    uint32_t m = metres() % 1000;
    float k = m < 450 ? 0 : m < 500 ? (m - 450) / 50.0f : m < 950 ? 1 : (1000 - m) / 50.0f;
    auto mix = [&](uint8_t a, uint8_t b) { return uint8_t((a + (b - a) * k) * 0.35f); };
    return Rgb{mix(DAY.r, NIGHT.r), mix(DAY.g, NIGHT.g), mix(DAY.b, NIGHT.b)};
  }

  void start_(uint32_t now) {
    rng_ ^= now * 2654435761u;  // a different track every run
    state_ = RUN;
    t_ = since_ = now;
    dist_ = 0;
    speed_ = 70;
    y_ = GROUND;
    vy_ = 0;
    air_ = duck_ = jump_held_ = new_best_ = false;
    for (auto &o : obs_) o.w = 0;
    next_ = 60;
  }

  bool over_hole_() const {
    for (const auto &o : obs_)
      if (o.w && o.kind == HOLE && o.x <= RX + 2 && o.x + o.w >= RX + 6) return true;
    return false;
  }

  void crash_() {
    state_ = CRASH;
    since_ = t_;
    vy_ = air_ ? vy_ : 0;
  }

  void step_() {
    const float dt = STEP_MS / 1000.0f;
    if (state_ == CRASH) {  // a fall into a hole goes on; nothing scrolls
      if (y_ > GROUND) y_ += 80 * dt;
      return;
    }
    speed_ = std::min(160.0f, 70 + dist_ * 0.015f);
    float dx = speed_ * dt;
    dist_ += dx;

    // Runner: lower gravity while the jump key is held at the start of the jump
    bool hole = over_hole_();
    if (!air_ && hole) {
      air_ = true;
      vy_ = 0;
    }
    if (air_) {
      float g = duck_ ? G_DUCK : (vy_ < 0 && jump_held_ && hold_ms_ < HOLD_MS) ? G_HOLD : G;
      vy_ += g * dt;
      y_ += vy_ * dt;
      hold_ms_ += STEP_MS;
      if (vy_ > 0 && y_ >= GROUND) {
        if (!hole) {
          y_ = GROUND;
          vy_ = 0;
          air_ = false;
        } else if (y_ > GROUND + 3) {
          return crash_();
        }
      }
    }

    // Obstacles scroll; a new one when the previous is far enough
    for (auto &o : obs_)
      if (o.w && (o.x -= dx) + o.w < 0) o.w = 0;
    if ((next_ -= dx) <= 0) spawn_();

    // Hits: the runner's box (smaller ducking) against crates, posts and birds
    int h = duck_ && !air_ ? 6 : 10, w = duck_ && !air_ ? 9 : 6;
    int top = int(y_) - h + 1, left = RX + 1;
    for (const auto &o : obs_) {
      if (!o.w || o.kind == HOLE) continue;
      int ox = int(o.x) + 1, ow = o.w - 2;
      if (ox < left + w && ox + ow > left && o.top < int(y_) && o.top + o.h > top) return crash_();
    }
  }

  void spawn_() {
    Obstacle *o = nullptr;
    for (auto &s : obs_)
      if (!s.w) o = &s;
    if (!o) return;
    uint32_t m = metres();
    int kinds = m < 150 ? 2 : m < 300 ? 3 : 4;
    o->kind = Kind(rnd_() % kinds);
    o->x = W;
    switch (o->kind) {
      case CRATE:  // 7 or 11 px wide
        o->w = rnd_() % 2 ? 11 : 7;
        o->h = 7;
        break;
      case POST:
        o->w = 4;
        o->h = 9 + rnd_() % 4;
        break;
      case HOLE:
        o->w = 12 + rnd_() % (4 + int(speed_ / 12));
        o->h = 0;
        break;
      case BIRD:  // high: duck; low: jump
        o->w = 9;
        o->h = 6;
        break;
    }
    o->top = o->kind == BIRD ? (rnd_() % 2 ? GROUND - 14 : GROUND - 8) : GROUND - o->h;
    // Room for a landing and a new jump, more at higher speed; sometimes a pair close together
    float gap = speed_ * 0.55f + 22 + rnd_() % (20 + int(speed_ * 0.5f));
    next_ = o->w + gap;
  }

  void render_(uint32_t now) {
    clear_();
    std::string hi = "HI " + num_(best_, 4);
    if (state_ == TITLE) {
      centre_text_(5, "DASH", 1);
      centre_text_(20, "JUMP: UP OR SCENE KEY", 1);
      centre_text_(28, "HOLD FOR A HIGHER JUMP", 1);
      centre_text_(36, "DUCK: DOWN", 1);
      centre_text_(46, hi, 1);
      if ((now / 500) % 2) centre_text_(56, "PRESS OK", 1);
      return;
    }
    text_(0, 2, num_(metres(), 4), 2);
    text_(33, 7, "M", 1);
    text_(W - text_w_(hi, 1), 2, hi, 1);
    draw_world_(now);
    if (paused_) return pause_panel_();
    if (state_ == OVER) {
      int x = 14, y = 19, w = W - 28, h = 42;
      fill_(x, y, w, h, false);
      for (int xx = x; xx < x + w; xx++) set_(xx, y), set_(xx, y + h - 1);
      for (int yy = y; yy < y + h; yy++) set_(x, yy), set_(x + w - 1, yy);
      centre_text_(y + 4, "GAME OVER", 1);
      centre_text_(y + 13, std::to_string(metres()) + " M", 2);
      centre_text_(y + 26, new_best_ ? "NEW BEST" : "", 1);
      if (now - since_ > 1000) centre_text_(y + 34, "OK PLAY  BACK LIST", 1);
    }
  }

  void draw_world_(uint32_t now) {
    int scroll = int(dist_);
    // Two clouds, slower than the ground
    for (int c = 0; c < 2; c++) {
      int cx = W - 1 - ((scroll / 4 + c * 70) % (W + 20)) + 10, cy = 21 + c * 6;
      sprite_(cx, cy, CLOUD, 4);
    }
    // Ground line with holes, pebbles fixed to the ground
    for (int x = 0; x < W; x++) {
      bool hole = false;
      for (const auto &o : obs_)
        if (o.w && o.kind == HOLE && x >= int(o.x) && x < int(o.x) + o.w) hole = true;
      if (hole) continue;
      set_(x, GROUND);
      uint32_t hsh = uint32_t(scroll + x) * 2654435761u;
      if ((hsh >> 24) % 11 == 0) set_(x, GROUND + 2 + (hsh >> 20) % 4);
    }
    for (const auto &o : obs_) {
      if (!o.w) continue;
      int x = int(o.x);
      if (o.kind == CRATE) {  // outlined box with a cross
        for (int xx = x; xx < x + o.w; xx++) set_(xx, o.top), set_(xx, o.top + o.h - 1);
        for (int yy = o.top; yy < o.top + o.h; yy++) set_(x, yy), set_(x + o.w - 1, yy);
        for (int i = 1; i < o.h - 1; i++) {
          int dx = i * (o.w - 2) / (o.h - 2);
          set_(x + dx, o.top + i), set_(x + o.w - 1 - dx, o.top + i);
        }
      } else if (o.kind == POST) {
        fill_(x, o.top, o.w, o.h, true);
        clr_(x, o.top), clr_(x + o.w - 1, o.top);
      } else if (o.kind == BIRD) {
        sprite_(x, o.top, (now / 150) % 2 ? BIRD_A : BIRD_B, 6);
      }
    }
    // The runner: blinking after the crash
    if (state_ == CRASH && (now / 120) % 2) return;
    int y = int(y_);
    if (state_ == CRASH || state_ == OVER) {
      sprite_(RX, y - 9, HIT, 10);
    } else if (duck_ && !air_) {
      sprite_(RX, y - 5, (scroll / 6) % 2 ? DUCK_A : DUCK_B, 6);
    } else if (air_) {
      sprite_(RX, y - 9, JUMP, 10);
    } else {
      sprite_(RX, y - 9, (scroll / 6) % 2 ? RUN_A : RUN_B, 10);
    }
  }

  static constexpr const char *RUN_A[10] = {"..###...", "..###...", "...#....", ".#####..", "#.###.#.",
                                             "..###...", "..#.#...", ".#...#..", ".#...#..", "#.....#."};
  static constexpr const char *RUN_B[10] = {"..###...", "..###...", "...#....", ".#####..", "#.###.#.",
                                             "..###...", "...#....", "...##...", "..#.#...", "..#..#.."};
  static constexpr const char *JUMP[10] = {"..###...", "..###...", "#..#..#.", ".#####..", "..###...",
                                            "..###...", ".#...#..", "#.....#.", "........", "........"};
  static constexpr const char *HIT[10] = {"..#.#...", "...#....", "..#.#...", "#..#..#.", ".#####..",
                                           "..###...", "..###...", ".#...#..", "#.....#.", "#.....#."};
  static constexpr const char *DUCK_A[6] = {".......##", "..######.", ".#######.", "#.#####..", "..#..#...",
                                             ".#...#..."};
  static constexpr const char *DUCK_B[6] = {".......##", "..######.", ".#######.", "#.#####..", "...##....",
                                             "..#..#..."};
  static constexpr const char *BIRD_A[6] = {"#.......#", ".##...##.", "..#####..", "...###...", "....#....",
                                             "........."};
  static constexpr const char *BIRD_B[6] = {".........", "...###...", "..#####..", ".##...##.", "#...#...#",
                                             "........."};
  static constexpr const char *CLOUD[4] = {"....####....", "..##....##..", ".#........#.", "############"};

  State state_ = TITLE;
  bool list_ = false, paused_ = false, air_ = false, duck_ = false, jump_held_ = false, new_best_ = false;
  Obstacle obs_[6];
  float dist_ = 0, speed_ = 70, y_ = GROUND, vy_ = 0, next_ = 60;
  uint32_t since_ = 0, t_ = 0, hold_ms_ = 0;
};

}  // namespace planch
