# Changelog

All notable changes to PlancH are listed here. Hardware revisions match the "Rev." printed on
the back of the board.

## [Unreleased]

### Hardware — Rev. 1

- First public release of the design: 120 × 52 mm PCB, Waveshare ESP32-S3-Zero, SSD1306 OLED,
  10 buttons, capacitive touch area, 13 SK6812 RGB LEDs.
- Production files: Gerber, drill, BOM, pick-and-place, interactive BOM, schematic PDF, STEP.

### Accessories

- Desk stand: 3D-printed frame that holds the board at 20°, with light slots for the backlight.
- Wall mount: ring bracket glued behind the board and a base stuck to the wall, held by magnets;
  the backlight shines on the wall around it.

### Firmware

- Menu built automatically from Home Assistant: devices chosen with the label "PlancH", grouped
  by room and type (lights, covers, thermostats); scene keys assigned with the labels
  "PlancH S1"–"PlancH S4". Home Assistant package in `homeassistant/planch.yaml`.
- LEDs: D1 shows problems, D2–D3 and the backlight are lights for Home Assistant automations,
  the scene LEDs show the active scene and confirm each press.
- Wi-Fi set after flashing (web.esphome.io or the board's hotspot): no password in the files.
- Assembly test firmware `planch-bringup.yaml` (buttons, touch, display, LED groups) and a test
  bench `planch-test.yaml` that runs the menu on any ESP32 board.
- Colour lights: rows Brightness, Colour (palette editable in the package), White and Effect on the
  screen of a light; Up/Down scroll through the rows and on to the next light.
- The menu follows label and area changes by itself (automation in the package).
- Install from the browser: the Firmware page installs PlancH on its ESP32-S3; a separate page,
  "Try it without the hardware", installs the test bench on a classic ESP32.
- Home Assistant card `planch-card`: the panel drawn on its picture, with display, LEDs,
  backlight glow and keys to tap.
