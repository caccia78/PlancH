# Firmware

PlancH runs [ESPHome](https://esphome.io). The current configuration,
[`firmware/planch.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch.yaml), is a
**bring-up firmware**: it exposes every button, the touch area, the display and the three LED
groups to Home Assistant, so you can test the board and build your own automations. Menus on
the display and ready-made scenes will come in later versions.

!!! info "Coming soon: install from the browser"
    A one-click installer (ESP Web Tools) is planned: plug the board into your computer, open
    this page in Chrome or Edge and press *Install*, without installing ESPHome.

## Flash with ESPHome

You need [ESPHome](https://esphome.io/guides/installing_esphome) on your computer
(`pip install esphome`, or `uvx esphome` with [uv](https://docs.astral.sh/uv/)), or the ESPHome
Device Builder add-on in Home Assistant.

1. Download the repository (or just the `firmware/` folder).
2. Copy `firmware/secrets.yaml.example` to `firmware/secrets.yaml` and fill in your Wi-Fi name
   and password. This file stays on your computer.
3. Connect the board with a USB-C **data** cable.
4. Run:

    ```bash
    esphome run firmware/planch.yaml
    ```

    The first time, choose the USB serial port. If the board is not detected, hold the **BOOT**
    button on the ESP32-S3-Zero while plugging in the cable.

5. After the first flash, later updates can go over Wi-Fi (OTA): run the same command and pick
   the network address.

## What the firmware exposes

| Entity | Type | Notes |
| --- | --- | --- |
| Up, Down, Left, Right, OK, Back | Binary sensor | `on` while pressed |
| Scene 1 – Scene 4 | Binary sensor | `on` while pressed |
| Touch | Binary sensor | Capacitive area on the ear, calibrate the threshold |
| Status LEDs | Light (RGB) | D1–D3 on the ear |
| Scene LEDs | Light (RGB) | D4–D7, next to the scene buttons |
| Backlight | Light (RGB) | D8–D13, towards the desk |

The display shows "PlancH". LED brightness is limited to 40% in the firmware to keep the USB
current low.

## Calibrating the touch area

The touch threshold depends on your board and on the stand. With `setup_mode: true` (the
default in `planch.yaml`) the ESPHome log prints the raw touch value:

1. Read the value at rest and with a finger on the ear.
2. Set `threshold` about halfway between the two.
3. Set `setup_mode: false` and flash again.

## Pinout

| GPIO | Function | GPIO | Function |
| --- | --- | --- | --- |
| 1 | Up | 9 | Scene 1 |
| 2 | Down | 10 | LED data (via 330 Ω) |
| 3 | Left (strapping pin, input only) | 11 | I²C SDA |
| 4 | Right | 12 | I²C SCL |
| 5 | OK | 13 | Touch |
| 6 | Scene 4 | 44 (RX) | Back |
| 7 | Scene 3 | 43 (TX) | not used (boot log) |
| 8 | Scene 2 | 21 | RGB LED on the module |
