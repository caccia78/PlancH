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
