// PlancH Invaders: a small shooter, the bonus game of the PlancH panel.
// Rows of invaders march down the blue part of the OLED; the cannon at the bottom moves with
// Left/Right and fires with OK or a scene key. Score and the bonus ship use the top 16 pixel rows
// (the yellow band of two-colour modules). LEDs: D1-D3 the lives, D4-D7 the four bunkers (green to
// red as they crumble), the backlight shows the wave colour and flashes on hits.
// It draws into its own 128 x 64 bitmap with a built-in 3 x 5 font and has no ESPHome dependency,
// so it also runs on a PC (frames for the preview GIF).
#pragma once

#include "menu.h"  // Key, Rgb

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace planch {

class Invaders {
 public:
  static constexpr int W = 128, H = 64;
  static constexpr int LEDS = 13;
  static constexpr uint32_t TICK_MS = 30;  // game step: about 33 steps per second

  enum State { TITLE, PLAY, HIT, CLEAR, OVER };

  // Starts on the title screen
  void open(uint32_t now) {
    state_ = TITLE;
    since_ = now;
    last_ = now;
    exit_ = false;
    paused_ = false;
    std::memset(held_, 0, sizeof(held_));
    render_(now);
  }

  // Back on the title or game over screen: the panel goes back to the menu
  bool exit_requested() const { return exit_; }
  State state() const { return state_; }
  uint32_t score() const { return score_; }
  uint32_t best() const { return best_; }
  void set_best(uint32_t b) { best_ = b; }

  // 1 bit per pixel, 16 bytes per row, bit 7 of each byte is the leftmost pixel
  bool pixel(int x, int y) const { return fb_[y * 16 + x / 8] & (0x80 >> (x & 7)); }

  void press(Key k, uint32_t now) {
    held_[k] = true;
    bool fire = k == OK || k == S1 || k == S2 || k == S3 || k == S4;
    switch (state_) {
      case TITLE:
        if (k == OK) start_(now);
        if (k == BACK) exit_ = true;
        break;
      case OVER:
        if (now - since_ < 1000) break;  // a key held while dying does not restart at once
        if (k == OK) start_(now);
        if (k == BACK) exit_ = true;
        break;
      default:
        if (k == BACK) paused_ = !paused_;
        else if (paused_ && k == OK) paused_ = false;
        else if (fire) fire_();
        break;
    }
  }

  void release(Key k) { held_[k] = false; }

  // Runs the game up to now in steps of TICK_MS and redraws the bitmap
  void update(uint32_t now) {
    if (now - last_ > 500) last_ = now - TICK_MS;  // after a pause of the caller, no fast-forward
    while (now - last_ >= TICK_MS) {
      last_ += TICK_MS;
      if (!paused_) step_(last_);
    }
    render_(now);
  }

  // Colours of the 13 LEDs, in chain order as planch::Menu::leds()
  void leds(Rgb out[LEDS], uint32_t now) const {
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

  // 3 x 5 font: digits, capitals and a few signs; 5 rows of 3 bits, bit 2 = left column
  static const uint8_t *glyph_(char c) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ=:?-./";
    static const uint8_t g[][5] = {
        {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
        {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
        {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
        {7, 4, 6, 4, 4}, {3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2},
        {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7}, {5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {2, 5, 5, 5, 2},
        {6, 5, 6, 4, 4}, {2, 5, 5, 6, 3}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2},
        {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5}, {5, 5, 2, 2, 2},
        {7, 1, 2, 4, 7}, {0, 7, 0, 7, 0}, {0, 2, 0, 2, 0}, {6, 1, 2, 0, 2}, {0, 0, 7, 0, 0},
        {0, 0, 0, 0, 2}, {1, 1, 2, 4, 4},
    };
    const char *p = std::strchr(chars, c);
    return (c && p) ? g[p - chars] : nullptr;
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
    if (held_[OK] || held_[S1] || held_[S2] || held_[S3] || held_[S4]) fire_();

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
    std::memset(fb_, 0, sizeof(fb_));
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
    if ((now / 500) % 2) text_((W - text_w_("PRESS OK", 1)) / 2, 57, "PRESS OK", 1);
    text_(84, 27, "BEST", 1);
    text_(84, 34, num_(best_, 4), 1);
  }

  void over_screen_(uint32_t now) {
    text_(0, 1, "SCORE " + num_(score_, 4), 1);
    std::string hi = "HI " + num_(best_, 4);
    text_(W - text_w_(hi, 1), 1, hi, 1);
    text_((W - text_w_("GAME OVER", 2)) / 2, 22, "GAME OVER", 2);
    if (score_ && score_ == best_) text_((W - text_w_("NEW BEST", 1)) / 2, 37, "NEW BEST", 1);
    if (now - since_ > 1000) text_((W - text_w_("OK PLAY   BACK EXIT", 1)) / 2, 54, "OK PLAY   BACK EXIT", 1);
  }

  void centre_box_(const std::string &t) {
    int w = text_w_(t, 2) + 8, h = 18, x = (W - w) / 2, y = 30;
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) clr_(xx, yy);
    for (int xx = x; xx < x + w; xx++) set_(xx, y), set_(xx, y + h - 1);
    for (int yy = y; yy < y + h; yy++) set_(x, yy), set_(x + w - 1, yy);
    text_(x + 4, y + 4, t, 2);
  }

  static std::string num_(uint32_t v, int digits) {
    std::string s = std::to_string(v);
    while ((int) s.size() < digits) s = "0" + s;
    return s;
  }
  static int text_w_(const std::string &t, int scale) { return t.empty() ? 0 : (int(t.size()) * 4 - 1) * scale; }
  void text_(int x, int y, const std::string &t, int scale) {
    for (char ch : t) {
      const uint8_t *g = glyph_(ch);
      if (g)
        for (int r = 0; r < 5; r++)
          for (int c = 0; c < 3; c++)
            if (g[r] & (4 >> c))
              for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) set_(x + c * scale + sx, y + r * scale + sy);
      x += 4 * scale;
    }
  }
  void sprite_(int x, int y, const char *const *rows, int h) {
    for (int r = 0; r < h; r++)
      for (int c = 0; rows[r][c]; c++)
        if (rows[r][c] == '#') set_(x + c, y + r);
  }
  void set_(int x, int y) {
    if (x >= 0 && x < W && y >= 0 && y < H) fb_[y * 16 + x / 8] |= 0x80 >> (x & 7);
  }
  void clr_(int x, int y) {
    if (x >= 0 && x < W && y >= 0 && y < H) fb_[y * 16 + x / 8] &= ~(0x80 >> (x & 7));
  }

  // --- Helpers

  uint32_t rnd_() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
  }
  static float tri_(uint32_t now, uint32_t period) {
    float p = float(now % period) / period;
    return p < 0.5f ? 2 * p : 2 - 2 * p;
  }
  // Wave colours at a given brightness (0-255): blue, cyan, green, yellow, orange, magenta
  static Rgb wave_colour_(uint32_t wave, int level) {
    static const uint8_t c[6][3] = {{0, 60, 255}, {0, 200, 200}, {0, 220, 40}, {220, 200, 0}, {255, 90, 0}, {220, 0, 180}};
    const uint8_t *k = c[wave % 6];
    return Rgb{uint8_t(k[0] * level / 255), uint8_t(k[1] * level / 255), uint8_t(k[2] * level / 255)};
  }

  uint8_t fb_[W * H / 8] = {};
  bool held_[TOUCH + 1] = {};
  State state_ = TITLE;
  bool paused_ = false, exit_ = false;
  uint32_t last_ = 0, since_ = 0, kill_ = 0, rng_ = 0x2F6E2B1u;
  uint32_t score_ = 0, best_ = 0;
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

// Routes the keys to the menu or to the game. OK and Back held together for 3 s open the game, only
// where the board reports the key releases (enable()); Back on the title or game over screen, or a
// minute without keys outside a game, go back to the menu.
class Arcade {
 public:
  static constexpr uint32_t HOLD_MS = 3000, IDLE_MS = 60000;

  void enable() { enabled_ = true; }
  bool active() const { return active_; }
  const Invaders &game() const { return game_; }
  uint32_t best() const { return game_.best(); }
  void set_best(uint32_t b) { game_.set_best(b); }

  // Key pressed: in the menu it returns the Home Assistant actions to perform
  std::vector<Call> press(Key k, uint32_t now) {
    held_[k] = true;
    last_ = now;
    if (active_) {
      game_.press(k, now);
      return {};
    }
    return menu().key(k, now);
  }

  void release(Key k) {
    held_[k] = false;
    if (active_) game_.release(k);
  }

  // Called every few ms: opens and closes the game, runs it; true while the game is on screen
  bool loop(uint32_t now) {
    if (!active_) {
      if (enabled_ && held_[OK] && held_[BACK]) {
        if (!both_) both_ = now | 1;
        else if (now - both_ >= HOLD_MS) {
          active_ = true;
          both_ = 0;
          last_ = now;
          game_.open(now);
        }
      } else {
        both_ = 0;
      }
      return active_;
    }
    game_.update(now);
    bool waiting = game_.state() == Invaders::TITLE || game_.state() == Invaders::OVER;
    if (game_.exit_requested() || (waiting && now - last_ > IDLE_MS)) {
      active_ = false;
      menu().key(TOUCH, now);  // back to the menu with the screen on
    }
    return true;
  }

  void leds(Rgb out[Invaders::LEDS], uint32_t now) const {
    if (active_) game_.leds(out, now);
    else menu().leds(out, now);
  }

 private:
  Invaders game_;
  bool enabled_ = false, active_ = false;
  bool held_[TOUCH + 1] = {};
  uint32_t both_ = 0, last_ = 0;
};

inline Arcade &arcade() {
  static Arcade a;
  return a;
}

}  // namespace planch
