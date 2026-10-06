# Desk stand

A 3D-printed stand that holds PlancH at 20° on the desk. The board keeps its rubber feet and
simply drops into a frame that follows its outline: no screws, no glue, and it lifts out with
a finger through the notch at the front. Slots in the frame let the backlight LEDs shine out
onto the desk.

<div class="grid" markdown>
<figure markdown="span">
  ![Desk stand with PlancH, front](../media/desk-stand-front.png)
  <figcaption>The board in the stand.</figcaption>
</figure>
<figure markdown="span">
  ![Desk stand, side view](../media/desk-stand-side.png)
  <figcaption>Side view: 20° tilt.</figcaption>
</figure>
</div>

## 3D View

Inspect the stand design in 3D, on its own or with the board in place. With the board shown
you can also switch on the LEDs: the backlight shining on the desk is suggested by the glowing
light slots.

<div style="margin: 2rem 0 0.5rem; border: 1px solid var(--md-border-color); border-radius: 8px; overflow: hidden;">
  <model-viewer 
    id="leggio-3d"
    src="/PlancH/assets/leggio-viewer.glb"
    alt="PlancH desk stand 3D model"
    auto-rotate
    camera-controls
    style="width: 100%; height: 500px; display: block;">
  </model-viewer>
</div>

<p>
  <button type="button" class="md-button" id="leggio-scheda" aria-pressed="false">Show the board</button>
  <span id="leggio-led-controlli" hidden>
    <button type="button" class="md-button" id="leggio-led" aria-pressed="false">Turn on the LEDs</button>
    <label>Colour <input type="color" id="leggio-colore" value="#ff7043"></label>
  </span>
</p>

<script>
  (() => {
    const viewer = document.getElementById("leggio-3d");
    const boardButton = document.getElementById("leggio-scheda");
    const ledControls = document.getElementById("leggio-led-controlli");
    const ledButton = document.getElementById("leggio-led");
    const ledColour = document.getElementById("leggio-colore");
    const models = {
      false: "/PlancH/assets/leggio-viewer.glb",
      true: "/PlancH/assets/leggio-assieme-viewer.glb",
    };
    let ledsOn = false;

    // glTF colour factors are linear, the colour picker is sRGB
    const linear = (hex) => [1, 3, 5].map((i) => {
      const c = parseInt(hex.slice(i, i + 2), 16) / 255;
      return c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4;
    });

    // Material names written by mechanical/leggio.py
    const applyLeds = () => {
      if (!viewer.model) return;
      const rgb = linear(ledColour.value);
      for (const name of ["led_stato", "led_scene", "led_retro"]) {
        const led = viewer.model.getMaterialByName(name);
        if (!led) continue;
        led.setEmissiveFactor(ledsOn ? rgb : [0, 0, 0]);
        led.pbrMetallicRoughness.setBaseColorFactor(ledsOn ? [...rgb, 1] : [0.85, 0.85, 0.85, 1]);
      }
      const slots = viewer.model.getMaterialByName("luce_retro");
      if (slots) {
        slots.setEmissiveFactor(ledsOn ? rgb : [0, 0, 0]);
        slots.pbrMetallicRoughness.setBaseColorFactor(ledsOn ? [...rgb, 0.85] : [1, 1, 1, 0]);
      }
      ledButton.setAttribute("aria-pressed", String(ledsOn));
      ledButton.textContent = ledsOn ? "Turn off the LEDs" : "Turn on the LEDs";
    };
    viewer.addEventListener("load", applyLeds);

    boardButton.addEventListener("click", () => {
      const show = boardButton.getAttribute("aria-pressed") !== "true";
      const orbit = viewer.getCameraOrbit();
      viewer.addEventListener("load", () => {
        viewer.cameraOrbit = `${orbit.theta}rad ${orbit.phi}rad ${orbit.radius}m`;
        viewer.jumpCameraToGoal();
      }, { once: true });
      viewer.src = models[show];
      boardButton.setAttribute("aria-pressed", String(show));
      boardButton.textContent = show ? "Hide the board" : "Show the board";
      ledControls.hidden = !show;
    });
    ledButton.addEventListener("click", () => { ledsOn = !ledsOn; applyLeds(); });
    ledColour.addEventListener("input", () => { ledsOn = true; applyLeds(); });
  })();
</script>

## Files

| File | Use |
| --- | --- |
| [`planch-desk-stand.3mf`](https://github.com/caccia78/PlancH/raw/main/mechanical/desk-stand/planch-desk-stand.3mf) | Ready for Bambu Studio / PrusaSlicer, already oriented |
| [`planch-desk-stand.stl`](https://github.com/caccia78/PlancH/blob/main/mechanical/desk-stand/planch-desk-stand.stl) | Any slicer (GitHub shows it in 3D) |
| [`planch-desk-stand.step`](https://github.com/caccia78/PlancH/raw/main/mechanical/desk-stand/planch-desk-stand.step) | CAD |
| [`leggio.py`](https://github.com/caccia78/PlancH/blob/main/mechanical/desk-stand/leggio.py) | Parametric source (build123d) |

## Printing

- **Material**: PLA, about 35 g with 15% infill.
- **Orientation**: on its base, as in the 3MF. No supports needed.
- **Colour**: white or light PLA works best: the inside of the frame reflects the backlight
  through the slots.
- **Tested on**: Bambu Lab A1, 0.4 mm nozzle, 0.2 mm layers.
- **Bumpers**: four Ø 10 mm rubber bumpers go in the recesses underneath, so the stand does not
  slide when you press the buttons.

## Design

| Parameter | Value |
| --- | --- |
| Tilt | 20° |
| Rubber feet height | 5 mm |
| Board thickness | 2.0 mm |
| Gap between board and frame | 0.3 mm per side |
| Walls / floor | 2.4 / 2.0 mm |
| Wall top | 0.5 mm below the top face of the board |
| Overall size | about 125 × 57 × 31 mm |

- The frame outline is read from the board STEP file, so it always follows the real PCB edge.
- Five light slots face the backlight LEDs D8–D12; D13 shines out of the USB-C opening.
- The USB-C opening leaves room for the plug and its overmould; the cable leaves from the top,
  behind the stand.

## Adapting it

If your board is 1.6 mm thick, your feet are taller or your printer runs tight, regenerate the
model with [uv](https://docs.astral.sh/uv/) from the repository root:

```bash
cd mechanical/desk-stand
uv run --no-project --python 3.12 --with build123d python leggio.py \
    --board ../../production/planch.step --out . --pcb 1.6 --feet 5 --tilt 20
```

The script writes `leggio.stl`, `leggio.3mf` and `leggio.step` ("leggio" is Italian for
"stand").
