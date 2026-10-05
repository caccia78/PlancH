# Home Assistant

PlancH talks to Home Assistant through the native ESPHome API: no MQTT broker, no cloud.

## Adding the board

1. [Flash the firmware](../firmware/index.md) and let the board join your Wi-Fi.
2. Home Assistant discovers it automatically: open **Settings → Devices & services**, find
   **PlancH** under *Discovered* and click **Configure**.
3. If it is not discovered, click **Add integration → ESPHome** and enter the board's IP
   address (or `planch.local`).

The device page then shows the [entities](../firmware/index.md#what-the-firmware-exposes):
10 buttons, the touch area and 3 RGB lights.

## Example: a scene button

Turn on a scene when **Scene 1** is pressed, and use the scene LED as feedback:

```yaml
automation:
  - alias: "PlancH: Scene 1 → evening"
    triggers:
      - trigger: state
        entity_id: binary_sensor.planch_scene_1
        to: "on"
    actions:
      - action: scene.turn_on
        target:
          entity_id: scene.evening
      - action: light.turn_on
        target:
          entity_id: light.planch_scene_leds
        data:
          rgb_color: [255, 160, 60]
          brightness_pct: 60
```

Entity IDs depend on the device name: check them on the device page and adjust the example.

## Example: dim the backlight at night

```yaml
automation:
  - alias: "PlancH: backlight follows the sun"
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

!!! info "More to come"
    Menus on the display, rooms, shutters and heating control are planned for the next
    firmware versions. This page will grow with them.
