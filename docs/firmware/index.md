# Firmware

PlancH runs [ESPHome](https://esphome.io). The devices to control are not written in the
firmware: you choose them in Home Assistant with a label, and PlancH builds its menu by itself,
by room and by type. See [Home Assistant](../home-assistant/index.md) for the setup.

| File | Use |
| --- | --- |
| [`firmware/planch.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch.yaml) | **The firmware**: menu on the display, keys, LEDs |
| [`firmware/planch/`](https://github.com/caccia78/PlancH/tree/main/firmware/planch) | Shared logic (`core.yaml`, `menu.h`): keep it next to `planch.yaml` |
| [`firmware/planch-bringup.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch-bringup.yaml) | Assembly test: buttons and LED groups on a web page, see the [assembly guide](../build/assembly.md) |
| [`firmware/planch-test.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch-test.yaml) | Test bench on any ESP32 board, without the PlancH hardware |

## Install from the browser

The quickest way, with nothing to install: the firmware is built from this repository and
written by the browser (Chrome or Edge on a computer).

<script type="module" src="https://unpkg.com/esp-web-tools@10.4.0/dist/web/install-button.js?module"></script>

<p>
  <esp-web-install-button manifest="/PlancH/firmware/install/manifest.json">
    <span slot="unsupported">This browser cannot talk to USB devices: open the page in Chrome or Edge on a computer.</span>
    <span slot="not-allowed">The installer needs a secure (https) page.</span>
  </esp-web-install-button>
</p>

1. Connect the board with a USB-C **data** cable and press **Connect** above.
2. Choose the serial port of the board. If it does not appear, hold the **BOOT** button of the
   ESP32-S3-Zero while plugging in the cable, then try again.
3. Choose **Install PlancH**. Erasing the board is fine for a first installation.
4. At the end choose **Configure Wi-Fi**, pick your network and type its password: it stays on
   the board, not in any file.
5. Add the board to Home Assistant as described in [Home Assistant](../home-assistant/index.md).

The installer picks the firmware from the chip it finds: on an ESP32-S3 it installs PlancH, on a
classic ESP32 it installs the [test bench](#test-bench-without-the-hardware).

## Flash with ESPHome

You need [ESPHome](https://esphome.io/guides/installing_esphome) on your computer
(`pip install esphome`, or `uvx esphome` with [uv](https://docs.astral.sh/uv/)), or the ESPHome
Device Builder add-on in Home Assistant.

1. Download the repository, or just the `firmware/` folder. With the Device Builder, copy
   `planch.yaml` and the `planch/` folder into the `esphome` folder of Home Assistant (next to
   `configuration.yaml`).
2. Connect the board with a USB-C **data** cable.
3. Run:

    ```bash
    esphome run firmware/planch.yaml
    ```

    The first time, choose the USB serial port. If the board is not detected, hold the **BOOT**
    button on the ESP32-S3-Zero while plugging in the cable.

4. **Wi-Fi.** The firmware has no Wi-Fi password in it: set it after flashing.
    - Open [web.esphome.io](https://web.esphome.io) in Chrome or Edge, press **Connect**, pick
      the board and choose **Configure Wi-Fi**; or
    - join the **PlancH** Wi-Fi network that the board opens while it has no network, and pick
      your network on the page that appears (or open `192.168.4.1`).
5. Later updates go over Wi-Fi (OTA): run the same command and pick the network address.

Then add the board to Home Assistant and enable its actions, as described in
[Home Assistant](../home-assistant/index.md).

## Keys and display

The display shows rooms → types in the room (skipped if there is only one) → devices →
the controls of one device.

| Key | In the lists | On a device |
| --- | --- | --- |
| Up, Down | Move | Light, thermostat: previous / next device. Cover: open / close |
| OK, Right | Enter | Light, thermostat: on / off. Cover: stop. Right on a light: brighter; on a thermostat: +0.5 °C |
| Back, Left | Go back | Back: go back. Left on a light: dimmer; on a thermostat: −0.5 °C |
| Scene 1–4 | Run the scene, script or automation assigned in Home Assistant | Same |
| Touch | Wake the display | Wake the display |

The display switches off after 30 seconds without keys; the first key only wakes it up. The
scene keys work with the display off too.

## LEDs

| LED | Meaning |
| --- | --- |
| D1 (ear, left) | Off when everything is fine. Red: no connection to Home Assistant. Amber: menu missing, empty or over the limits. Short red flash: a command failed |
| D2, D3 (ear) | Free: two RGB lights in Home Assistant, **Status LED 2** and **Status LED 3**, for your automations |
| D4–D7 (scene keys) | On with the display, and for 5 seconds after a scene key. Full on the **active scene**, dim on the other assigned keys. After a press: green if Home Assistant confirms, red on error |
| D8–D13 (backlight) | The RGB light **Backlight** in Home Assistant |

LED brightness is limited to 40% in the firmware to keep the USB current low.

## What the firmware exposes

| Entity | Type | Notes |
| --- | --- | --- |
| Up, Down, Left, Right, OK, Back, Scene 1 – Scene 4 | Binary sensor | `on` while pressed: you can use them in automations too |
| Touch | Binary sensor | Capacitive area on the ear, calibrate the threshold |
| Status LED 2, Status LED 3 | Light (RGB) | D2 and D3 on the ear |
| Backlight | Light (RGB) | D8–D13, towards the desk (or the wall) |

## Calibrating the touch area

The touch threshold depends on your board and on the stand. With `setup_mode: true` (the
default in `planch.yaml`) the ESPHome log prints the raw touch value:

1. Read the value at rest and with a finger on the ear.
2. Set `threshold` about halfway between the two.
3. Set `setup_mode: false` and flash again.

## Test bench without the hardware

[`firmware/planch-test.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch-test.yaml)
runs the same menu on any ESP32 board (it was developed on an ESP-WROOM-32) with nothing
connected. The keys are buttons in Home Assistant; what the display and the LEDs would show is
published as two text sensors, **Screen** and **LEDs**. The dashboard
[`homeassistant/planch-test-dashboard.yaml`](https://github.com/caccia78/PlancH/blob/main/homeassistant/planch-test-dashboard.yaml)
draws the panel: display, LEDs as coloured dots and keys laid out as on the board. Useful to try
the menu with your devices before building the board.

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
