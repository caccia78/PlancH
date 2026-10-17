// PlancH Arcade: what every game has in common. A game draws into its own 128 x 64 bitmap (with a
// built-in 3 x 5 font) and computes the colours of the 13 LEDs; arcade.h shows the list of games and
// routes the keys. No ESPHome dependency: the games also run on a PC.
#pragma once

#include "menu.h"  // Key, Rgb

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

namespace planch {

// 1-bit picture of the OLED with simple drawing helpers
class Bitmap {
 public:
  static constexpr int W = 128, H = 64;

  // 1 bit per pixel, 16 bytes per row, bit 7 of each byte is the leftmost pixel
  bool pixel(int x, int y) const { return fb_[y * 16 + x / 8] & (0x80 >> (x & 7)); }
  const uint8_t *fb() const { return fb_; }

 protected:
  void clear_() { std::memset(fb_, 0, sizeof(fb_)); }
  void set_(int x, int y) {
    if (x >= 0 && x < W && y >= 0 && y < H) fb_[y * 16 + x / 8] |= 0x80 >> (x & 7);
  }
  void clr_(int x, int y) {
    if (x >= 0 && x < W && y >= 0 && y < H) fb_[y * 16 + x / 8] &= ~(0x80 >> (x & 7));
  }
  void fill_(int x, int y, int w, int h, bool on) {
    for (int yy = y; yy < y + h; yy++)
      for (int xx = x; xx < x + w; xx++) on ? set_(xx, yy) : clr_(xx, yy);
  }
  // Sprite: one string per row, '#' = pixel on
  void sprite_(int x, int y, const char *const *rows, int h) {
    for (int r = 0; r < h; r++)
      for (int c = 0; rows[r][c]; c++)
        if (rows[r][c] == '#') set_(x + c, y + r);
  }
  static int text_w_(const std::string &t, int scale) { return t.empty() ? 0 : (int(t.size()) * 4 - 1) * scale; }
  // Text in the 3 x 5 font; inverted: pixels cleared (on a filled bar)
  void text_(int x, int y, const std::string &t, int scale, bool inverted = false) {
    for (char ch : t) {
      const uint8_t *g = glyph_(ch);
      if (g)
        for (int r = 0; r < 5; r++)
          for (int c = 0; c < 3; c++)
            if (g[r] & (4 >> c))
              for (int sy = 0; sy < scale; sy++)
                for (int sx = 0; sx < scale; sx++) {
                  int px = x + c * scale + sx, py = y + r * scale + sy;
                  inverted ? clr_(px, py) : set_(px, py);
                }
      x += 4 * scale;
    }
  }
  void centre_text_(int y, const std::string &t, int scale) { text_((W - text_w_(t, scale)) / 2, y, t, scale); }
  // Framed box in the middle of the play area with a short message
  void centre_box_(const std::string &t) {
    int w = text_w_(t, 2) + 8, h = 18, x = (W - w) / 2, y = 30;
    fill_(x, y, w, h, false);
    for (int xx = x; xx < x + w; xx++) set_(xx, y), set_(xx, y + h - 1);
    for (int yy = y; yy < y + h; yy++) set_(x, yy), set_(x + w - 1, yy);
    text_(x + 4, y + 4, t, 2);
  }
  // Pause panel over the play area: what OK and Back do
  void pause_panel_() {
    int x = 14, y = 17, w = W - 28, h = 44;
    fill_(x, y, w, h, false);
    for (int xx = x; xx < x + w; xx++) set_(xx, y), set_(xx, y + h - 1);
    for (int yy = y; yy < y + h; yy++) set_(x, yy), set_(x + w - 1, yy);
    centre_text_(y + 6, "PAUSE", 2);
    centre_text_(y + 23, "OK: RESUME", 1);
    centre_text_(y + 32, "HOLD BACK: EXIT", 1);
  }
  static std::string num_(uint32_t v, int digits) {
    std::string s = std::to_string(v);
    while ((int) s.size() < digits) s = "0" + s;
    return s;
  }

  uint8_t fb_[W * H / 8] = {};

 private:
  // 3 x 5 font: digits, capitals and a few signs; 5 rows of 3 bits, bit 2 = left column
  static const uint8_t *glyph_(char c) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ=:?-./!";
    static const uint8_t g[][5] = {
        {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
        {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
        {2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7},
        {7, 4, 6, 4, 4}, {3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2},
        {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7}, {5, 7, 7, 5, 5}, {6, 5, 5, 5, 5}, {2, 5, 5, 5, 2},
        {6, 5, 6, 4, 4}, {2, 5, 5, 6, 3}, {6, 5, 6, 5, 5}, {3, 4, 2, 1, 6}, {7, 2, 2, 2, 2},
        {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5}, {5, 5, 2, 2, 2},
        {7, 1, 2, 4, 7}, {0, 7, 0, 7, 0}, {0, 2, 0, 2, 0}, {6, 1, 2, 0, 2}, {0, 0, 7, 0, 0},
        {0, 0, 0, 0, 2}, {1, 1, 2, 4, 4}, {2, 2, 2, 0, 2},
    };
    const char *p = std::strchr(chars, c);
    return (c && p) ? g[p - chars] : nullptr;
  }
};

// A game of the arcade. Back reaches the game only as a short press (on release): holding it for
// 3 s leaves the arcade, handled by arcade.h
class Game : public Bitmap {
 public:
  static constexpr int LEDS = 13;

  virtual ~Game() = default;
  virtual const char *name() const = 0;
  // Starts on the game's title screen
  virtual void open(uint32_t now) = 0;
  virtual void press(Key k, uint32_t now) = 0;
  virtual void release(Key k) = 0;
  // Runs the game up to now and redraws the bitmap
  virtual void update(uint32_t now) = 0;
  // Colours of the 13 LEDs, in chain order as planch::Menu::leds()
  virtual void leds(Rgb out[LEDS], uint32_t now) const = 0;
  // True during a match (no automatic exit), false on the title and game over screens
  virtual bool playing() const = 0;
  // Back pressed on the title or game over screen: the arcade shows the list again
  virtual bool wants_list() const = 0;

  uint32_t best() const { return best_; }
  void set_best(uint32_t b) { best_ = b; }

 protected:
  static float tri_(uint32_t now, uint32_t period) {
    float p = float(now % period) / period;
    return p < 0.5f ? 2 * p : 2 - 2 * p;
  }
  uint32_t rnd_() {
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return rng_;
  }

  uint32_t best_ = 0, rng_ = 0x2F6E2B1u;
};

}  // namespace planch
