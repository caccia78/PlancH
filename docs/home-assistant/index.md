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
lights, switches (smart plugs), covers and thermostats (climate) you want on the panel.

- Put it on the **entity**, to choose exactly what appears; or on the **device**, to take all
  its lights, switches, covers and thermostats.
- A lamp on a smart plug is often a `switch`: it appears under **Switches**. To have it under
  **Lights**, turn it into a light with the Home Assistant helper *Change device type of a
  switch* and label the new light instead.
- The room is the area of the entity, or of its device. Without an area the device goes under
  **Other**.
- The name shown is the entity name without the room in front ("Living room Lamp" →
  "Lamp"), cut to 16 characters.

The menu updates by itself: states live, and labels or areas within a couple of seconds (the
package includes a small automation, **PlancH: refresh the menu when labels or areas change**,
for that).

!!! warning "Limits"
    A panel shows at most **8 rooms**, **8 devices per room** and **32 devices** in total:
    more would not fit in the ESP32 memory nor on the 0.96" display. Beyond the limits the list
    is cut, the LED D1 turns amber and the room list reads "Rooms (limit!)".

### More than one PlancH

Each panel can show its own devices besides the shared ones:

1. Give the panel a name: on its device page in Home Assistant, under **Configuration**, type
   it in the **Panel** field (for example *Studio*). No new firmware needed.
2. Create the label **PlancH Studio** and put it on the devices for that panel only.

The panel named Studio shows the devices with **PlancH** and with **PlancH Studio**; a device
can have several labels to appear on more than one panel. A panel without a name, or with a
name that has no label, shows the **PlancH** devices. Names are not case sensitive, and up to
6 named panels are supported.

## 4 · Assign the scene keys

Create the labels **PlancH S1**, **PlancH S2**, **PlancH S3** and **PlancH S4** and put each
one on the scene, script or automation for that key (one entity per label): a scene is turned
on, a script is run, an automation is triggered. A key without a label does nothing.

With more panels, **PlancH Studio S1** … **PlancH Studio S4** set the keys of the panel Studio
only, and win over the shared ones.

The LED next to each key shows the **active scene**: the one activated most recently among the
four, also when you activate it from Home Assistant or from an automation (a script also while
it runs). When you press a key, its LED turns green if Home Assistant confirms, red on error.

## Colours

On a colour light the panel steps through a **palette**, defined at the top of the `palette`
attribute in `planch.yaml`. Edit it as you like: name (up to 10 characters), hue (0–360) and
saturation (0–100).

{% raw %}
```yaml
{%- set palette = [
  ['White', 0, 0], ['Warm white', 35, 45], ['Orange', 30, 100], ['Red', 0, 100],
  ['Pink', 330, 65], ['Violet', 275, 80], ['Blue', 225, 100], ['Cyan', 185, 90],
  ['Green', 120, 90], ['Yellow', 55, 90]] %}
```
{% endraw %}

The favourite colours that Home Assistant shows in the light dialog are stored where templates
cannot read them, so the panel uses this list instead. Lights with a white temperature get a
**White** row, lights with effects (WLED and many others) an **Effect** row: see the
[Firmware](../firmware/index.md#keys-and-display) page.

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
