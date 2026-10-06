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
you can also switch on the LEDs, by group as in the firmware: the backlight shining on the desk
is suggested by the glowing light slots.

--8<-- "model-viewer.html"

<div style="margin: 2rem 0 0.5rem; border: 1px solid var(--md-border-color); border-radius: 8px; overflow: hidden;">
  <model-viewer 
    id="leggio-3d"
    src="/PlancH/assets/leggio-viewer.glb"
    alt="PlancH desk stand 3D model"
    auto-rotate
    camera-controls
    style="width: 100%; height: 500px; display: block;">
    <!-- HDR rendering: only the LEDs, with emissive strength above 1, pass the threshold -->
    <effect-composer render-mode="quality">
      <bloom-effect strength="0.8" threshold="3" radius="0.6"></bloom-effect>
    </effect-composer>
  </model-viewer>
</div>

<p>
  <button type="button" class="md-button" id="leggio-scheda" aria-pressed="false">Show the board</button>
  <button type="button" class="md-button" data-viewer-background="leggio-3d" aria-pressed="false">Dark background</button>
</p>
<p id="leggio-led-controlli" hidden>
  LEDs:
  <button type="button" class="md-button" data-led="led_stato" aria-pressed="false">Status</button>
  <button type="button" class="md-button" data-led="led_scene" aria-pressed="false">Scenes</button>
  <button type="button" class="md-button" data-led="led_retro" aria-pressed="false">Backlight</button>
  <label>Colour <input type="color" value="#ff7043"></label>
</p>

<script>
  planchLeds("leggio-3d", "leggio-led-controlli");
  (() => {
    const viewer = document.getElementById("leggio-3d");
    const boardButton = document.getElementById("leggio-scheda");
    const models = {
      false: "/PlancH/assets/leggio-viewer.glb",
      true: "/PlancH/assets/leggio-assieme-viewer.glb",
    };
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
      document.getElementById("leggio-led-controlli").hidden = !show;
    });
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
