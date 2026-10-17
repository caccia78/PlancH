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
- Switches (smart plugs) in the menu, under "Switches".
- More than one PlancH in the same home: the label "PlancH" for devices on every panel,
  "PlancH <name>" for one panel only; the name is set in the "Panel" field on the device page.
  Scene keys per panel with "PlancH <name> S1" … "S4". Panels installed from the browser get a
  unique name (planch-xxxxxx).
- Install from the browser: the Firmware page installs PlancH on its ESP32-S3; a separate page,
  "Try it without the hardware", installs the test bench on a classic ESP32.
- Home Assistant card `planch-card`: the panel drawn on its picture, with display, LEDs,
  backlight glow and keys to tap.
- Adoption in the ESPHome Device Builder: a panel (or the test bench) installed from the browser
  can be adopted and then updated over Wi-Fi, loading the firmware from this repository. The menu
  model is now an ESPHome external component (`firmware/components/planch/`).
- Wi-Fi on the board: power saving off and transmit power 8.5 dB. With the defaults the first
  assembled board kept dropping off the network; adopted panels get the change at the next
  Install.
- Touch area: measurement time 0.25 ms (with the ESPHome default the ESP32-S3 counter was stuck
  at its top value) and threshold 40 000, measured on the first board; setup mode off by default.
- Display: larger text (11 px), the title in the top 16 px (the yellow band of two-colour OLEDs),
  long names shortened with "…" so the state on the right stays readable. Same layout in the
  `planch-card` (version 1.2).

- Room climate: temperature next to each room in the room list, temperature and humidity next to
  the title inside a room, from the sensors of the room's area (averaged); planch-card 1.3.

### Documentation

- Usage examples: office, bedside, kitchen timer on the backlight, indicators on the status LEDs.
