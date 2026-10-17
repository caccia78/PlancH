// PlancH Invaders: a small shooter, the first game of PlancH Arcade (arcade.h).
// Rows of invaders march down the blue part of the OLED; the cannon at the bottom moves with
// Left/Right and fires with Scene 4 (the bottom scene key). Score and the bonus ship use the top 16
// pixel rows (the yellow band of two-colour modules). LEDs: D1-D3 the lives, D4-D7 the four bunkers
// (green to red as they crumble), the backlight shows the wave colour and flashes on hits.
#pragma once

#include "game.h"

namespace planch {

class Invaders : public Game {
 public:
  static constexpr uint32_t TICK_MS = 30;  // game step: about 33 steps per second

  enum State { TITLE, PLAY, HIT, CLEAR, OVER };

  const char *name() const override { return "INVADERS"; }

  void open(uint32_t now) override {
    state_ = TITLE;
    since_ = now;
    last_ = now;
    list_ = false;
    paused_ = false;
    std::memset(held_, 0, sizeof(held_));
    render_(now);
  }

  bool wants_list() const override { return list_; }
  bool playing() const override { return state_ == PLAY || state_ == HIT || state_ == CLEAR; }
  State state() const { return state_; }
  uint32_t score() const { return score_; }

  // Scene 4 fires; OK starts, plays again and resumes; Back (short) pauses, or on the title and
  // game over screens goes back to the list of games
  void press(Key k, uint32_t now) override {
    held_[k] = true;
    bool fire = k == S4;
    switch (state_) {
      case TITLE:
        if (k == OK) start_(now);
        if (k == BACK) list_ = true;
        break;
      case OVER:
        if (now - since_ < 1000) break;  // a key held while dying does not restart at once
        if (k == OK) start_(now);
        if (k == BACK) list_ = true;
        break;
      default:
        if (k == BACK) paused_ = !paused_;
        else if (paused_ && k == OK) paused_ = false;
        else if (fire) fire_();
        break;
    }
  }

  void release(Key k) override { held_[k] = false; }

  // Runs the game up to now in steps of TICK_MS and redraws the bitmap
  void update(uint32_t now) override {
    if (now - last_ > 500) last_ = now - TICK_MS;  // after a pause of the caller, no fast-forward
    while (now - last_ >= TICK_MS) {
      last_ += TICK_MS;
      if (!paused_) step_(last_);
    }
    render_(now);
  }

  // Colours of the 13 LEDs, in chain order as planch::Menu::leds()
  void leds(Rgb out[LEDS], uint32_t now) const override {
    for (int i = 0; i < LEDS; i++) out[i] = Rgb{};
    bool playing = state_ == PLAY || state_ == HIT || state_ == CLEAR;
    // D1-D3: lives; the life just lost blinks red
    for (int i = 0; i < 3; i++) {
      if (playing && i < lives_) out[i] = Rgb{0, 150, 70};
      if (state_ == HIT && i == lives_ && (now / 120) % 2) out[i] = Rgb{255, 0, 0};
    }
    // D4-D7: bunkers, green when whole, red when almost gone, off when destroyed
    for (int b = 0; b < 4; b++) {
      if (!playing) break;
      float h = bunker_health_(b);
      if (h > 0.05f) out[3 + b] = Rgb{uint8_t(255 * (1 - h)), uint8_t(200 * h), 0};
    }
    // Backlight: wave colour, purple while the bonus ship flies, red on a hit, white on a cleared
    // wave, a slow red pulse after the game
    Rgb bl = wave_colour_(wave_, 50);
    if (state_ == TITLE) bl = wave_colour_(now / 1500, 40 + int(30 * tri_(now, 3000)));
    if (playing && ufo_) bl = Rgb{uint8_t(120 * tri_(now, 400) + 40), 0, uint8_t(160 * tri_(now, 400) + 60)};
    if (state_ == HIT && now - since_ < 500) bl = Rgb{255, 0, 0};
    if (state_ == CLEAR) {
      uint8_t v = uint8_t(255 * (1 - std::min(1.0f, (now - since_) / 1000.0f)));
      bl = Rgb{v, v, v};
    }
    if (playing && now - kill_ < 80 && state_ == PLAY) bl = Rgb{200, 200, 200};
    if (state_ == OVER) bl = Rgb{uint8_t(30 + 120 * tri_(now, 2000)), 0, 0};
    for (int i = 7; i < LEDS; i++) out[i] = bl;
  }

 private:
  static constexpr int COLS = 8, ROWS = 4;
  static constexpr int CELL_W = 11, CELL_H = 8;  // invader grid: 8 x 6 sprites
  static constexpr int PLAYER_Y = 59;            // cannon rows 59-62, ground at 63
  static constexpr int BUNKER_Y = 50;            // bunkers rows 50-54
  static constexpr int UFO_Y = 9;                // bonus ship in the yellow band, rows 9-12
  static constexpr int MAX_SHOTS = 3;            // invader bullets at the same time

  struct Shot {
    bool on = false;
    int x = 0, y16 = 0;  // y in 1/16 px
  };
  struct Boom {
    int x = 0, y = 0, ticks = 0;
    uint32_t points = 0;  // shown instead of the explosion (bonus ship)
  };

  // --- Bitmaps (one string per row, '#' = pixel on). Original drawings for PlancH.
  static const char *const *alien_(int type, int frame) {
    static const char *const a[3][2][6] = {
        {{"...##...", "..####..", ".##..##.", "########", "..#..#..", ".#.##.#."},
         {"...##...", "..####..", ".##..##.", "########", ".#....#.", "#......#"}},
        {{"#..##..#", ".######.", "##.##.##", "########", "..#..#..", ".##..##."},
         {"...##...", "#######.", "##.##.##", "########", ".#.##.#.", "#......#"}},
        {{".######.", "##.##.##", "########", "..#..#..", ".#.##.#.", "#......#"},
         {".######.", "##.##.##", "########", ".##..##.", "#......#", ".#....#."}},
    };
    return a[type][frame];
  }
  static const char *const *player_() {
    static const char *const p[4] = {"....#....", "...###...", ".#######.", "#########"};
    return p;
  }
  static const char *const *ufo_bitmap_() {
    static const char *const u[4] = {"...#####...", ".#########.", "##.#.#.#.##", "..##...##.."};
    return u;
  }
  static const char *const *boom_() {
    static const char *const b[6] = {"#..#..#.", ".#.#.#..", "..#.#.#.", "##.#.##.", "..#.#...", ".#..#.#."};
    return b;
  }
  static const char *const *bunker_init_() {
    static const char *const b[5] = {".##########.", "############", "############", "####....####", "###......###"};
    return b;
  }

  // --- Game

  void start_(uint32_t now) {
    score_ = 0;
    lives_ = 3;
    wave_ = 0;
    paused_ = false;
    new_wave_(now);
  }

  void new_wave_(uint32_t now) {
    for (int b = 0; b < 4; b++)  // bunkers rebuilt at every wave
      for (int r = 0; r < 5; r++) {
        bunker_[b][r] = 0;
        for (int c = 0; c < 12; c++)
          if (bunker_init_()[r][c] == '#') bunker_[b][r] |= 1 << (11 - c);
      }
    bunker_full_ = bunker_pixels_(0);
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++) alive_[r][c] = true;
    ax_ = (W - ((COLS - 1) * CELL_W + 8)) / 2;
    ay_ = 18 + std::min(wave_, 4) * 3;  // later waves start lower
    dir_ = 1;
    frame_ = 0;
    march_ = 0;
    px16_ = (W / 2) * 16;
    bullet_ = Shot{};
    for (auto &s : shots_) s = Shot{};
    for (auto &b : booms_) b = Boom{};
    ufo_ = false;
    ufo_wait_ = 400 + rnd_() % 300;
    state_ = PLAY;
    since_ = now;
  }

  void fire_() {
    if (state_ != PLAY || bullet_.on) return;
    bullet_.on = true;
    bullet_.x = px16_ / 16;
    bullet_.y16 = (PLAYER_Y - 1) * 16;
  }

  int alive_count_() const {
    int n = 0;
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++) n += alive_[r][c];
    return n;
  }

  void step_(uint32_t now) {
    for (auto &b : booms_)
      if (b.ticks > 0) b.ticks--;
    if (state_ == HIT) {
      if (now - since_ > 1300) {
        if (lives_ <= 0) over_(now);
        else {
          state_ = PLAY;
          since_ = now;
          px16_ = (W / 2) * 16;
        }
      }
      return;
    }
    if (state_ == CLEAR) {
      if (now - since_ > 1600) {
        wave_++;
        new_wave_(now);
      }
      return;
    }
    if (state_ != PLAY) return;

    // Cannon: Left/Right held, 1.5 px per step
    if (held_[LEFT] && !held_[RIGHT]) px16_ = std::max(4 * 16, px16_ - 24);
    if (held_[RIGHT] && !held_[LEFT]) px16_ = std::min((W - 5) * 16, px16_ + 24);
    if (held_[S4]) fire_();

    move_bullet_(now);
    march_invaders_(now);
    if (state_ != PLAY) return;
    invader_shots_(now);
    if (state_ != PLAY) return;
    move_ufo_();
  }

  void move_bullet_(uint32_t now) {
    if (!bullet_.on) return;
    for (int i = 0; i < 4 && bullet_.on; i++) {  // 4 px per step, checked pixel by pixel
      bullet_.y16 -= 16;
      int x = bullet_.x, y = bullet_.y16 / 16;
      if (y < 0) {
        bullet_.on = false;
        break;
      }
      if (hit_bunker_(x, y, -1)) {
        bullet_.on = false;
        break;
      }
      if (ufo_ && y >= UFO_Y && y < UFO_Y + 4 && x >= ufo_x16_ / 16 && x < ufo_x16_ / 16 + 11) {
        static const uint32_t pts[] = {50, 100, 150, 300};
        uint32_t p = pts[rnd_() % 4];
        score_ += p;
        add_boom_(ufo_x16_ / 16 + 1, UFO_Y - 1, p);
        ufo_ = false;
        ufo_wait_ = 500 + rnd_() % 400;
        bullet_.on = false;
        kill_ = now;
        break;
      }
      for (int r = 0; r < ROWS && bullet_.on; r++)
        for (int c = 0; c < COLS && bullet_.on; c++) {
          if (!alive_[r][c]) continue;
          int x0 = ax_ + c * CELL_W, y0 = ay_ + r * CELL_H;
          if (x >= x0 && x < x0 + 8 && y >= y0 && y < y0 + 6) {
            alive_[r][c] = false;
            score_ += r == 0 ? 30 : r == 1 ? 20 : 10;
            add_boom_(x0, y0, 0);
            bullet_.on = false;
            kill_ = now;
          }
        }
    }
    if (alive_count_() == 0) {
      state_ = CLEAR;
      since_ = now;
      bullet_.on = false;
      for (auto &s : shots_) s.on = false;
    }
  }

  // The block moves 2 px at a time, faster as it thins out; at the edge it drops 3 px and turns
  void march_invaders_(uint32_t now) {
    int n = alive_count_();
    if (n == 0) return;
    int every = 2 + n / 2 - std::min(wave_, 3);
    if (++march_ < std::max(2, every)) return;
    march_ = 0;
    frame_ ^= 1;
    int left = COLS, right = -1, bottom = -1;
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++)
        if (alive_[r][c]) {
          left = std::min(left, c);
          right = std::max(right, c);
          bottom = std::max(bottom, r);
        }
    int x_left = ax_ + left * CELL_W, x_right = ax_ + right * CELL_W + 7;
    if ((dir_ > 0 && x_right + 2 > W - 1) || (dir_ < 0 && x_left - 2 < 0)) {
      ay_ += 3;
      dir_ = -dir_;
    } else {
      ax_ += 2 * dir_;
    }
    // Invaders crush the bunkers they pass through, and win when they reach the cannon
    int y_bottom = ay_ + bottom * CELL_H + 5;
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++)
        if (alive_[r][c])
          for (int dy = 0; dy < 6; dy++)
            for (int dx = 0; dx < 8; dx++) clear_bunker_(ax_ + c * CELL_W + dx, ay_ + r * CELL_H + dy);
    if (y_bottom >= PLAYER_Y) {
      lives_ = 0;
      hit_player_(now);
    }
  }

  void invader_shots_(uint32_t now) {
    // A new shot from the lowest invader of a random column, more often in later waves
    int every = std::max(12, 45 - wave_ * 6);
    if (rnd_() % every == 0) {
      int c = rnd_() % COLS;
      for (int r = ROWS - 1; r >= 0; r--)
        if (alive_[r][c]) {
          for (auto &s : shots_)
            if (!s.on) {
              s.on = true;
              s.x = ax_ + c * CELL_W + 3 + rnd_() % 2;
              s.y16 = (ay_ + r * CELL_H + 6) * 16;
              break;
            }
          break;
        }
    }
    int px = px16_ / 16;
    for (auto &s : shots_) {
      if (!s.on) continue;
      int before = s.y16 / 16;
      s.y16 += 20 + std::min(wave_, 4) * 3;  // 1.25 px per step and more
      for (int y = before + 1; y <= s.y16 / 16 && s.on; y++) {
        if (y >= H - 1) s.on = false;
        else if (hit_bunker_(s.x, y, 1)) s.on = false;
        else if (y >= PLAYER_Y && y < PLAYER_Y + 4 && s.x >= px - 4 && s.x <= px + 4) {
          s.on = false;
          hit_player_(now);
          return;
        } else if (bullet_.on && s.x == bullet_.x && std::abs(y - bullet_.y16 / 16) < 2) {
          s.on = bullet_.on = false;  // two bullets cancel out
        }
      }
    }
  }

  void move_ufo_() {
    if (!ufo_) {
      if (ufo_wait_ > 0) ufo_wait_--;
      else if (alive_count_() >= 6) {
        ufo_ = true;
        ufo_dir_ = rnd_() % 2 ? 1 : -1;
        ufo_x16_ = ufo_dir_ > 0 ? -11 * 16 : W * 16;
      }
      return;
    }
    ufo_x16_ += ufo_dir_ * 14;
    if (ufo_x16_ > W * 16 || ufo_x16_ < -11 * 16) {
      ufo_ = false;
      ufo_wait_ = 500 + rnd_() % 400;
    }
  }

  void hit_player_(uint32_t now) {
    if (lives_ > 0) lives_--;
    bullet_.on = false;
    for (auto &s : shots_) s.on = false;
    state_ = HIT;
    since_ = now;
  }

  void over_(uint32_t now) {
    state_ = OVER;
    since_ = now;
    if (score_ > best_) best_ = score_;
  }

  void add_boom_(int x, int y, uint32_t points) {
    for (auto &b : booms_)
      if (b.ticks == 0) {
        b = Boom{x, y, points ? 40 : 8, points};
        return;
      }
  }

  // Bunkers: 4 bitmaps of 12 x 5 px, centred under the four quarters of the screen
  static int bunker_x_(int b) { return 10 + b * 32; }
  // dir: -1 bullet going up, 1 going down
  bool hit_bunker_(int x, int y, int dir) {
    if (y < BUNKER_Y || y >= BUNKER_Y + 5) return false;
    for (int b = 0; b < 4; b++) {
      int c = x - bunker_x_(b);
      if (c < 0 || c >= 12) continue;
      if (!(bunker_[b][y - BUNKER_Y] & (1 << (11 - c)))) return false;
      clear_bunker_(x, y);  // a small crater: the pixel hit, the next one and a random neighbour
      clear_bunker_(x, y + dir);
      clear_bunker_(x + (rnd_() % 2 ? 1 : -1), y);
      return true;
    }
    return false;
  }
  void clear_bunker_(int x, int y) {
    if (y < BUNKER_Y || y >= BUNKER_Y + 5) return;
    for (int b = 0; b < 4; b++) {
      int c = x - bunker_x_(b);
      if (c >= 0 && c < 12) bunker_[b][y - BUNKER_Y] &= ~(1 << (11 - c));
    }
  }
  int bunker_pixels_(int b) const {
    int n = 0;
    for (int r = 0; r < 5; r++)
      for (int c = 0; c < 12; c++) n += (bunker_[b][r] >> c) & 1;
    return n;
  }
  float bunker_health_(int b) const { return bunker_full_ ? float(bunker_pixels_(b)) / bunker_full_ : 0; }

  // --- Drawing

  void render_(uint32_t now) {
    clear_();
    switch (state_) {
      case TITLE: title_screen_(now); return;
      case OVER: over_screen_(now); return;
      default: break;
    }
    // Yellow band: score, best, bonus ship
    text_(0, 1, "SCORE " + num_(score_, 4), 1);
    std::string hi = "HI " + num_(std::max(best_, score_), 4);
    text_(W - text_w_(hi, 1), 1, hi, 1);
    if (ufo_) sprite_(ufo_x16_ / 16, UFO_Y, ufo_bitmap_(), 4);
    // Blue band: invaders, bunkers, cannon, bullets, ground
    for (int r = 0; r < ROWS; r++)
      for (int c = 0; c < COLS; c++)
        if (alive_[r][c]) sprite_(ax_ + c * CELL_W, ay_ + r * CELL_H, alien_(r == 0 ? 0 : r == 1 ? 1 : 2, frame_), 6);
    for (int b = 0; b < 4; b++)
      for (int r = 0; r < 5; r++)
        for (int c = 0; c < 12; c++)
          if (bunker_[b][r] & (1 << (11 - c))) set_(bunker_x_(b) + c, BUNKER_Y + r);
    if (state_ != HIT) sprite_(px16_ / 16 - 4, PLAYER_Y, player_(), 4);
    else if (now - since_ < 900 && (now / 90) % 2) sprite_(px16_ / 16 - 4, PLAYER_Y - 2, boom_(), 6);
    if (bullet_.on)
      for (int i = 0; i < 3; i++) set_(bullet_.x, bullet_.y16 / 16 + i);
    for (auto &s : shots_)
      if (s.on)
        for (int i = 0; i < 3; i++) set_(s.x + ((s.y16 / 16 / 2 + i) % 2), s.y16 / 16 - i);
    for (auto &b : booms_) {
      if (b.ticks == 0) continue;
      if (b.points) text_(b.x, b.y + 2, num_(b.points, 0), 1);
      else sprite_(b.x, b.y, boom_(), 6);
    }
    for (int x = 0; x < W; x++)
      if (x % 4 != 3) set_(x, H - 1);
    if (state_ == CLEAR) centre_box_("WAVE " + std::to_string(wave_ + 2));
    if (paused_) centre_box_("PAUSE");
  }

  void title_screen_(uint32_t now) {
    text_((W - text_w_("PLANCH INVADERS", 1)) / 2, 5, "PLANCH INVADERS", 1);
    for (int i = 0; i < 3; i++) {
      int y = 19 + i * 9;
      sprite_(40, y, alien_(i, (now / 500) % 2), 6);
      text_(52, y + 1, std::string("= ") + (i == 0 ? "30" : i == 1 ? "20" : "10"), 1);
    }
    sprite_(38, 46, ufo_bitmap_(), 4);
    text_(52, 46, "= ?", 1);
    if ((now / 2000) % 2) centre_text_(57, "FIRE: SCENE 4", 1);
    else if ((now / 500) % 2) centre_text_(57, "PRESS OK", 1);
    text_(84, 27, "BEST", 1);
    text_(84, 34, num_(best_, 4), 1);
  }

  void over_screen_(uint32_t now) {
    text_(0, 1, "SCORE " + num_(score_, 4), 1);
    std::string hi = "HI " + num_(best_, 4);
    text_(W - text_w_(hi, 1), 1, hi, 1);
    text_((W - text_w_("GAME OVER", 2)) / 2, 22, "GAME OVER", 2);
    if (score_ && score_ == best_) text_((W - text_w_("NEW BEST", 1)) / 2, 37, "NEW BEST", 1);
    if (now - since_ > 1000) centre_text_(54, "OK PLAY   BACK LIST", 1);
  }

  // --- Helpers

  // Wave colours at a given brightness (0-255): blue, cyan, green, yellow, orange, magenta
  static Rgb wave_colour_(uint32_t wave, int level) {
    static const uint8_t c[6][3] = {{0, 60, 255}, {0, 200, 200}, {0, 220, 40}, {220, 200, 0}, {255, 90, 0}, {220, 0, 180}};
    const uint8_t *k = c[wave % 6];
    return Rgb{uint8_t(k[0] * level / 255), uint8_t(k[1] * level / 255), uint8_t(k[2] * level / 255)};
  }

  bool held_[TOUCH + 1] = {};
  State state_ = TITLE;
  bool paused_ = false, list_ = false;
  uint32_t last_ = 0, since_ = 0, kill_ = 0;
  uint32_t score_ = 0;
  int lives_ = 3, wave_ = 0;
  bool alive_[ROWS][COLS] = {};
  int ax_ = 0, ay_ = 0, dir_ = 1, frame_ = 0, march_ = 0;
  int px16_ = W / 2 * 16;
  Shot bullet_, shots_[MAX_SHOTS];
  Boom booms_[6];
  bool ufo_ = false;
  int ufo_x16_ = 0, ufo_dir_ = 1, ufo_wait_ = 0;
  uint16_t bunker_[4][5] = {};
  int bunker_full_ = 0;
};

}  // namespace planch
