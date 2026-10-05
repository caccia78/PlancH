# Ordering the PCB

Download **`PlancH-v1.0-gerber.zip`** from the
[latest release](https://github.com/caccia78/PlancH/releases/latest) (or zip the files in
[`production/gerber/`](https://github.com/caccia78/PlancH/tree/main/production/gerber)) and
upload it to JLCPCB, PCBWay or any other PCB maker. You only need the **bare board**: every
part is soldered by hand.

## Recommended settings

| Option | Value |
| --- | --- |
| Layers | 2 |
| Size | 120 × 52 mm (detected from the Gerbers) |
| Thickness | **2.0 mm** (1.6 mm works too) |
| Copper | 1 oz |
| Solder mask | **Matte black**, both sides |
| Silkscreen | White |
| Surface finish | **ENIG** (lead-free HASL is fine for prototypes) |
| Vias | Tented |
| Internal cut-outs | Yes: 7 LED windows (3.4 × 3.0 mm) are in the outline file |
| Assembly | None |

## Notes

- **The front is meant to be seen.** Many fabs print their order number on the silkscreen:
  ask to remove it (JLCPCB: "Remove Mark") or to move it to the back.
- **The mask colour is an order option**, it is not in the Gerbers. Black matte looks best;
  any colour works electrically.
- **Board thickness** changes how deep the status LEDs sit in their windows: about 1.2 mm
  below the surface with 2.0 mm, about 0.8 mm with 1.6 mm. If you print the
  [desk stand](../accessories/desk-stand.md), generate it for the thickness you ordered.
- **Cost**: at the time of writing, five 1.6 mm boards in black with lead-free HASL cost a
  few euros at JLCPCB plus shipping. 2.0 mm and ENIG add a surcharge.
- The pick-and-place file [`production/planch-cpl.csv`](https://github.com/caccia78/PlancH/blob/main/production/planch-cpl.csv)
  is only useful if you choose an assembly service.
