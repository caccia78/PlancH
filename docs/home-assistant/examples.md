# Usage examples

Ideas for PlancH around the house, with the Home Assistant automations behind them. They use
only what the panel already offers: the menu built from labels, the four scene keys, the free
status LEDs **D2** and **D3** on the ear, the **Backlight** and the touch area.

!!! note "Entity IDs"
    The examples use `light.planch_status_led_2`, `light.planch_backlight` and so on. The real
    IDs depend on the name of your panel (for example `light.planch_3a4b5c_status_led_2`):
    check them on the device page and adjust the examples. Paste an automation in
    **Settings → Automations → Create automation → ⋮ → Edit in YAML**, without the
    `automation:` line, or keep them in a [package](index.md#1-install-the-planch-package).

## Office

**Scene keys.** A *Focus* scene (desk lamp on, cool white, ceiling off) on **S1**, a
*Video call* scene (face light on, blinds half closed) on **S2**, *Break* on **S3** and
*Leave* (everything off) on **S4**: put the labels **PlancH S1** … **PlancH S4** on the scenes.
The LED next to the active scene stays lit, so a glance tells which mode the office is in.

**D2 red during meetings.** With a calendar in Home Assistant (Google Calendar, CalDAV, Local
calendar…) the calendar entity is `on` while an event is running:

```yaml
automation:
  - alias: "PlancH: D2 red during meetings"
    triggers:
      - trigger: state
        entity_id: calendar.work
    actions:
      - if:
          - condition: state
            entity_id: calendar.work
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

## Bedside

A second PlancH on the night table, with its own keys: name the panel *Bedroom* (field
**Panel** on its device page, see [More than one PlancH](index.md#more-than-one-planch)) and
use the labels **PlancH Bedroom S1** … **S4**. They win over the shared scene keys on that
panel only.

- **S1** *Reading*: bedside lamp at 40 %, warm white.
- **S2** *Night*: everything off except a dim night light.
- **S3** *Wake up*: a script that brings the lamp up slowly (below).
- **S4** *All off*.

```yaml
script:
  planch_wake_up:
    alias: "Wake up"
    sequence:
      - action: light.turn_on
        target:
          entity_id: light.bedside_lamp
        data:
          brightness_pct: 1
          color_temp_kelvin: 2200
      - action: light.turn_on
        target:
          entity_id: light.bedside_lamp
        data:
          brightness_pct: 80
          color_temp_kelvin: 4000
          transition: 600   # 10 minutes
```

In the dark the panel stays quiet by itself: the display and the scene LEDs switch off 30 s
after the last key, and D1 is off while everything works. Keep the backlight low at night:

```yaml
automation:
  - alias: "PlancH: dim backlight at night"
    triggers:
      - trigger: time
        at: "22:30:00"
    actions:
      - action: light.turn_on
        target:
          entity_id: light.planch_bedroom_backlight
        data:
          rgb_color: [255, 120, 40]
          brightness_pct: 3
  - alias: "PlancH: backlight off in the morning"
    triggers:
      - trigger: time
        at: "07:30:00"
    actions:
      - action: light.turn_off
        target:
          entity_id: light.planch_bedroom_backlight
```

## Kitchen timer

A scene key starts a timer and the backlight counts down, from green to red.

1. Create a timer helper: **Settings → Devices & services → Helpers → Create helper → Timer**,
   name *Kitchen*, duration 10 minutes (`timer.kitchen`).
2. Create a script that starts it and put the label **PlancH S3** on the script:

    ```yaml
    script:
      planch_kitchen_timer:
        alias: "Kitchen timer"
        sequence:
          - action: timer.start
            target:
              entity_id: timer.kitchen
    ```

3. Add the automation that colours the backlight and flashes D3 at the end:

{% raw %}
```yaml
automation:
  - alias: "PlancH: kitchen timer on the backlight"
    mode: restart
    triggers:
      - trigger: state
        entity_id: timer.kitchen
        to: "active"
      - trigger: time_pattern
        seconds: "/10"
    conditions:
      - condition: state
        entity_id: timer.kitchen
        state: "active"
    actions:
      - variables:
          left: >-
            {{ (as_datetime(state_attr('timer.kitchen', 'finishes_at')) - now()).total_seconds() }}
          total: >-
            {{ as_timedelta(state_attr('timer.kitchen', 'duration')).total_seconds() }}
      - action: light.turn_on
        target:
          entity_id: light.planch_backlight
        data:
          hs_color: ["{{ [0, [120 * left / total, 120] | min] | max }}", 100]
          brightness_pct: 60
  - alias: "PlancH: kitchen timer finished"
    triggers:
      - trigger: event
        event_type: timer.finished
        event_data:
          entity_id: timer.kitchen
    actions:
      - action: light.turn_off
        target:
          entity_id: light.planch_backlight
      - action: light.turn_on
        target:
          entity_id: light.planch_status_led_3
        data:
          rgb_color: [255, 0, 0]
          flash: long
```
{% endraw %}

Pressing S3 again restarts the timer. If you cancel the timer from Home Assistant, the
backlight keeps its last colour until the next change.

## Indicators around the house

D2 and D3 are two RGB lights that any automation can use. Pick the two indicators that matter
most to you, one per LED: two automations writing the same LED overwrite each other.

| Indicator | LED in the example | Colour |
| --- | --- | --- |
| Front door open | D2 | Red |
| Washing machine finished | D3 | Cyan, until you touch the ear |
| Rain in the next hours | D3, instead of the washing machine | Blue |

### Front door open

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

### Washing machine finished

With a smart plug that measures power (`sensor.washing_machine_power`): when the power stays
low for a few minutes the cycle is over. Touching the ear of the panel clears the LED.

```yaml
automation:
  - alias: "PlancH: washing machine finished"
    triggers:
      - trigger: numeric_state
        entity_id: sensor.washing_machine_power
        below: 3
        for: "00:03:00"
    actions:
      - action: light.turn_on
        target:
          entity_id: light.planch_status_led_3
        data:
          rgb_color: [0, 255, 220]
          brightness_pct: 40
  - alias: "PlancH: touch clears the washing machine LED"
    triggers:
      - trigger: state
        entity_id: binary_sensor.planch_touch
        to: "on"
    actions:
      - action: light.turn_off
        target:
          entity_id: light.planch_status_led_3
```

The first automation may also fire when the plug comes back after a power cut; add a condition
on the power being above a few watts some minutes before if that bothers you.

### Rain in the next hours

Checks the hourly forecast every 30 minutes (`weather.home` is the default weather integration):

{% raw %}
```yaml
automation:
  - alias: "PlancH: D3 blue if rain is coming"
    triggers:
      - trigger: time_pattern
        minutes: "/30"
    actions:
      - action: weather.get_forecasts
        target:
          entity_id: weather.home
        data:
          type: hourly
        response_variable: forecast
      - if:
          - condition: template
            value_template: >-
              {{ forecast['weather.home'].forecast[:3]
                 | selectattr('condition', 'in', ['rainy', 'pouring', 'lightning-rainy', 'snowy-rainy'])
                 | list | count > 0 }}
        then:
          - action: light.turn_on
            target:
              entity_id: light.planch_status_led_3
            data:
              rgb_color: [0, 60, 255]
              brightness_pct: 40
        else:
          - action: light.turn_off
            target:
              entity_id: light.planch_status_led_3
```
{% endraw %}
