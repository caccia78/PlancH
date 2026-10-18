# Firmware

PlancH runs [ESPHome](https://esphome.io). The devices to control are not written in the
firmware: you choose them in Home Assistant with a label, and PlancH builds its menu by itself,
by room and by type. See [Home Assistant](../home-assistant/index.md) for the setup.

| File | Use |
| --- | --- |
| [`firmware/planch.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch.yaml) | **The firmware**: menu on the display, keys, LEDs |
| [`firmware/planch/`](https://github.com/caccia78/PlancH/tree/main/firmware/planch) | The board, the test bench and the shared logic (`core.yaml`), plus the packages used by [adopted panels](#adopt-it-in-the-esphome-device-builder) |
| [`firmware/components/planch/`](https://github.com/caccia78/PlancH/tree/main/firmware/components/planch) | The menu model (`menu.h`), as an ESPHome external component |
| [`firmware/planch-bringup.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch-bringup.yaml) | Assembly test: buttons and LED groups on a web page, see the [assembly guide](../build/assembly.md) |
| [`firmware/planch-test.yaml`](https://github.com/caccia78/PlancH/blob/main/firmware/planch-test.yaml) | Test bench on any ESP32 board, without the PlancH hardware: see [Try it without the hardware](test-bench.md) |

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
6. Optional: [adopt it in the ESPHome Device Builder](#adopt-it-in-the-esphome-device-builder)
   to update it over Wi-Fi and change its settings.

This button installs PlancH on its ESP32-S3-Zero. To try the menu on a spare ESP32 board before
building PlancH, see [Try it without the hardware](test-bench.md).

## Adopt it in the ESPHome Device Builder

With the **ESPHome Device Builder** add-on in Home Assistant, a PlancH installed from the
browser shows up by itself, ready to be adopted: no files to copy.

1. Open **ESPHome Device Builder**: the panel appears as *Discovered*. Click **Adopt**, keep
   the name or change it, and confirm.
2. The Device Builder writes a short configuration with the name, a new API encryption key, the
   Wi-Fi of its `secrets.yaml` and one line that loads PlancH from this repository:

    ```yaml
    packages:
      caccia78.planch: github://caccia78/PlancH/firmware/planch/import.yaml@main
    ```

3. Click **Install → Wirelessly**. The first build takes a few minutes. The install worked if
   the log ends with the panel restarting and showing `Noise encryption: YES`.
4. The panel now accepts only encrypted connections. Home Assistant usually takes the new key
   from the Device Builder add-on by itself, within a minute or so. If it asks for the key
   instead (a **Reauthenticate** notice at the top of **Settings → Devices & services**), copy it
   from the `api:` section of the panel's configuration.

!!! tip "Install → Wirelessly stops halfway"
    With a weak Wi-Fi link the upload can stop with an error such as `receiving chunk result`.
    Nothing is lost: the panel keeps running the firmware it had. Choose **Install → Manual
    download → Factory format** instead, connect the panel by USB and install the file from
    [web.esphome.io](https://web.esphome.io) (**Connect**, then **Install**). The Wi-Fi settings
    stay on the panel.

!!! tip "The panel does not show up as Discovered"
    The Device Builder hides a device when it already has a configuration with the same name, for
    example one copied by hand before. Delete that configuration (**⋮ → Delete**), restart the
    ESPHome Device Builder add-on and reload the page.

From then on **Install** builds the latest PlancH from the `main` branch of this repository
(the Device Builder fetches it again at most once a day). Your own changes go in the panel's
configuration, below the package, and win over it. For example, to change the touch threshold
after the [calibration](#calibrating-the-touch-area):

```yaml
binary_sensor:
  - id: !extend touch_key
    threshold: 3000
```

The test bench works the same way: it adopts `firmware/planch/import-test.yaml`.

## Flash with ESPHome

You need [ESPHome](https://esphome.io/guides/installing_esphome) on your computer
(`pip install esphome`, or `uvx esphome` with [uv](https://docs.astral.sh/uv/)), or the ESPHome
Device Builder add-on in Home Assistant.

1. Download the repository, or just the `firmware/` folder: `planch.yaml` needs the folders
   `planch/` and `components/` next to it. (With the Device Builder, adopting the panel is
   simpler: see above.)
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
| Up, Down | Move | Light: move through its rows, and past the last row on to the next light. Switch, thermostat: previous / next. Cover: open / close |
| OK | Enter | Light, switch, thermostat: on / off. Cover: stop |
| Left, Right | Back / enter | Light: change the selected row. Switch: off / on. Thermostat: −/+ 0.5 °C |
| Back | Go back | Go back to the list |
| Scene 1–4 | Run the scene, script or automation assigned in Home Assistant | Same |
| Touch | Wake the display | Wake the display |

The screen of a light shows only the rows that the light supports:

```
Strip
> Brightness      70%
  Colour          Red
  White         2700K
  Effect       Candle
```

- **Brightness**: ±10% (a light that can only switch on and off has a **State** row instead).
- **Colour**: previous / next colour of the palette defined in the
  [Home Assistant package](../home-assistant/index.md#colours); the row shows the palette colour
  closest to the current one.
- **White**: warmer / cooler in steps of 500 K, within the range of the light.
- **Effect**: previous / next effect of the light (WLED and many others), picked by Home
  Assistant from the light's own list.

The display switches off after 30 seconds without keys; the first key only wakes it up. The
scene keys work with the display off too.

### Clock

**Scene 4** opens a small dashboard: day and date, the time, the temperature of the panel's room,
the outdoor temperature with an icon of today's weather and today's min and max. It stays on until you leave it with Scene 4 again, the touch area or any of the
arrows, OK and Back (that key only closes the clock); Scene 1–3 still run their scenes. To avoid
marks on the OLED it moves by a couple of pixels every minute and dims from 23:00 to 7:00.

The room is the area of the panel's device in Home Assistant, with its labelled temperature and
humidity sensors; outdoors is a weather entity with the label PlancH (see
[Home Assistant](../home-assistant/index.md#clock)). On the panel's device page, **Scene 4 key**
turns Scene 4 back into a scene key and **Language** shows days and months in Italian.

Hold **OK** and **Back** together for 3 seconds to open the hidden games: see
[PlancH Arcade](arcade.md).

## LEDs

| LED | Meaning |
| --- | --- |
| D1 (ear, left) | Off when everything is fine. Red: no connection to Home Assistant. Amber: menu missing, empty or over the limits. Short red flash: a command failed |
| D2, D3 (ear) | Free: two RGB lights in Home Assistant, **Status LED 2** and **Status LED 3**, for your automations |
| D4–D7 (scene keys) | On with the display, and for 5 seconds after a scene key. Full on the **active scene**, dim on the other assigned keys. After a press: green if Home Assistant confirms, red on error. Brightness: **Scene LED active** and **Scene LED idle** below. D7 is light blue when Scene 4 opens the clock, full while the clock is on |
| D8–D13 (backlight) | The RGB light **Backlight** in Home Assistant; it can turn off by itself, see **Backlight off** below |

LED brightness is limited to 40% in the firmware to keep the USB current low.

## What the firmware exposes

| Entity | Type | Notes |
| --- | --- | --- |
| Up, Down, Left, Right, OK, Back, Scene 1 – Scene 4 | Binary sensor | `on` while pressed: you can use them in automations too |
| Panel | Text | Name of the panel, picks its devices: see [More than one PlancH](../home-assistant/index.md#more-than-one-planch) |
| Touch | Binary sensor | Capacitive area on the ear, wakes the display |
| Status LED 2, Status LED 3 | Light (RGB) | D2 and D3 on the ear |
| Backlight | Light (RGB) | D8–D13, towards the desk (or the wall) |
| Scene LED active | Number (%) | On the panel's device page, 5–100 % (default 100): the active scene, a key waiting for Home Assistant and the green or red flash |
| Scene 4 key | Select | **Clock** (default) or **Scene** |
| Language | Select | **English** or **Italiano**: days and months of the clock |
| Scene LED idle | Number (%) | 0–50 % of the full colour (default 10): the other assigned scene keys; 0 turns them off |
| Backlight off | Select | On the panel's device page: **Never** (default), **With the screen** (after 30 s without keys), **After 5 min**, **After 15 min**, **After 1 h** without keys. Only the LEDs turn off: the Backlight light stays on, and the next key or touch brings them back |

## Calibrating the touch area

The firmware comes with a threshold that works on the first assembled board: about 110 000 at
rest (jumping by up to 3 800 between two levels), 210 000 to 280 000 with a finger, threshold
40 000. Calibrate only if the touch triggers by itself (the display then never turns off) or
misses light touches:

1. Set `setup_mode: true` under `esp32_touch`: the ESPHome log prints the touch value and the
   resting value it is compared with (`benchmark`).
2. Read the value at rest and with a light finger on the ear.
3. On the ESP32-S3 the threshold is the **rise** over the resting value, not an absolute value:
   set `threshold` to about half of (touched − resting), and well above how much the resting value
   moves. With the numbers above: (210 000 − 110 000) / 2 = 50 000; the firmware uses 40 000.
   Read the numbers in the log, digit by digit: they are large.
4. Set `setup_mode: false` again (it slows the panel down) and flash. On an adopted panel, put
   the changes in its own configuration as shown in
   [Adopt it in the ESPHome Device Builder](#adopt-it-in-the-esphome-device-builder).

!!! note "Touch stuck at 4194303"
    That is the top of the counter: the measurement takes too long. The PlancH firmware sets
    `measurement_duration: 0.25ms`, as ESPHome suggests for the ESP32-S3; with the 8 ms default
    the touch never responds.

## Test bench without the hardware

The same menu runs on any ESP32 board with nothing connected, driven from a Home Assistant card
that draws PlancH: see [Try it without the hardware](test-bench.md).

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
