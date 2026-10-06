# Home Assistant

PlancH talks to Home Assistant through the native ESPHome API: no MQTT broker, no cloud.
You choose what it controls with **labels**: PlancH finds the devices by itself and groups them
by room (area) and type, so adding a lamp to the panel takes one click and no new firmware.

## 1 · Install the PlancH package

The package
[`homeassistant/planch.yaml`](https://github.com/caccia78/PlancH/blob/main/homeassistant/planch.yaml)
creates the sensor `sensor.planch_menu`, which collects the labelled devices with their state
and sends them to the panel.

1. In the Home Assistant configuration folder (the one with `configuration.yaml`) create a
   folder `packages` and copy `planch.yaml` into it.
2. If `configuration.yaml` does not load packages yet, add at the top level:

    ```yaml
    homeassistant:
      packages: !include_dir_named packages
    ```

3. **Developer tools → YAML → Check configuration**, then restart Home Assistant.
4. In **Developer tools → States** you find `sensor.planch_menu`: its state is the number of
   labelled devices (0 for now).

## 2 · Add the board

1. [Flash the firmware](../firmware/index.md) and let the board join your Wi-Fi.
2. Home Assistant discovers it: open **Settings → Devices & services**, find **PlancH** under
   *Discovered* and click **Configure**. If it is not discovered, click **Add integration →
   ESPHome** and enter the board's IP address (or `planch.local`).
3. On the ESPHome integration, open the **PlancH** device options (**Configure**) and enable
   **Allow the device to perform Home Assistant actions**. Without it the menu shows up but
   the keys do nothing.

## 3 · Choose the devices

Create the label **PlancH** (**Settings → Areas, labels & zones → Labels**) and put it on the
lights, covers and thermostats (climate) you want on the panel.

- Put it on the **entity**, to choose exactly what appears; or on the **device**, to take all
  its lights, covers and thermostats.
- The room is the area of the entity, or of its device. Without an area the device goes under
  **Other**.
- The name shown is the entity name without the room in front ("Living room Lamp" →
  "Lamp"), cut to 16 characters.

The menu updates by itself when a state changes. After adding labels or changing areas, toggle
one of the listed devices (or reload the template entities) to refresh it.

!!! warning "Limits"
    The panel shows at most **8 rooms**, **8 devices per room** and **32 devices** in total:
    more would not fit in the ESP32 memory nor on the 0.96" display. Beyond the limits the list
    is cut, the LED D1 turns amber and the room list reads "Rooms (limit!)".

## 4 · Assign the scene keys

Create the labels **PlancH S1**, **PlancH S2**, **PlancH S3** and **PlancH S4** and put each
one on the scene, script or automation for that key (one entity per label): a scene is turned
on, a script is run, an automation is triggered. A key without a label does nothing.

The LED next to each key shows the **active scene**: the one activated most recently among the
four, also when you activate it from Home Assistant or from an automation (a script also while
it runs). When you press a key, its LED turns green if Home Assistant confirms, red on error.

## Example: a status LED for the front door

D2 and D3 on the ear are free for your automations:

```yaml
automation:
  - alias: "PlancH: D2 red while the front door is open"
    triggers:
      - trigger: state
        entity_id: binary_sensor.front_door
    actions:
      - if:
          - condition: state
            entity_id: binary_sensor.front_door
            state: "on"
        then:
          - action: light.turn_on
            target:
              entity_id: light.planch_status_led_2
            data:
              rgb_color: [255, 0, 0]
              brightness_pct: 40
        else:
          - action: light.turn_off
            target:
              entity_id: light.planch_status_led_2
```

## Example: dim the backlight at night

```yaml
automation:
  - alias: "PlancH: warm, dim backlight after sunset"
    triggers:
      - trigger: sun
        event: sunset
    actions:
      - action: light.turn_on
        target:
          entity_id: light.planch_backlight
        data:
          rgb_color: [255, 180, 110]
          brightness_pct: 20
```

Entity IDs depend on the device name: check them on the device page and adjust the examples.
