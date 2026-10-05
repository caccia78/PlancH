---
hide:
  - navigation
  - toc
---

# PlancH

**A desk button panel for Home Assistant.** Ten tactile buttons, a small OLED display, a touch
area and thirteen addressable RGB LEDs on a black PCB that sits on your desk, on its own rubber
feet or on a 3D-printed stand. Powered by a Waveshare ESP32-S3-Zero running
[ESPHome](https://esphome.io), with native Home Assistant integration.

<video class="hero-video" controls muted playsinline preload="metadata" poster="media/planch-video-poster.png">
  <source src="media/planch-video.webm" type="video/webm">
  <source src="media/planch-video.mp4" type="video/mp4">
</video>

!!! warning "Rev. 1: prototypes on order"
    The design is complete and passes ERC and DRC, but the first boards have not been
    assembled and tested yet. Build one if you like to tinker, and expect changes.

<div class="grid cards" markdown>

-   :material-gesture-tap-button: **10 buttons**

    ---

    Directional pad with OK, Back, and four scene buttons for your favourite routines.

-   :material-monitor: **OLED display**

    ---

    0.96" SSD1306, 128 × 64 white pixels for menus and feedback, woken by a touch on the top
    "ear".

-   :material-led-strip-variant: **13 RGB LEDs**

    ---

    3 status LEDs, 4 scene LEDs shining through windows milled in the PCB, and 6 backlight
    LEDs that light up the desk.

-   :material-home-assistant: **ESPHome inside**

    ---

    Waveshare ESP32-S3-Zero over USB-C, native Home Assistant API, no cloud.

-   :material-soldering-iron: **Hand-solderable**

    ---

    No parts with contacts under the body, passives 0805 or larger. About 2–3 hours for the
    first board.

-   :material-printer-3d-nozzle: **Accessories**

    ---

    A 3D-printed desk stand, with more mounts to come.

</div>

| Front | Back |
| --- | --- |
| ![Front of the board](media/render-front.png) | ![Back of the board](media/render-back.png) |

## Build your own

<div class="grid cards" markdown>

-   :material-format-list-checks: **[Components](build/components.md)**

    Bill of materials with shop links.

-   :material-chip: **[Ordering the PCB](build/ordering.md)**

    Gerber files and fab settings.

-   :material-soldering-iron: **[Assembly](build/assembly.md)**

    Step-by-step soldering guide with a test at every stage.

-   :material-download: **[Firmware](firmware/index.md)**

    Flash ESPHome and connect to Home Assistant.

</div>

All design files (KiCad sources, Gerbers, 3D models, firmware) are in the
[GitHub repository](https://github.com/caccia78/PlancH). PlancH is
[open source hardware](about/license.md).
