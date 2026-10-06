# 3D Viewer

Explore PlancH in 3D — rotate, zoom, and pan to inspect every detail of the circuit board.

<iframe src="../../assets/viewer-3d.html" style="width: 100%; height: 500px; border: 1px solid var(--md-border-color); border-radius: 8px; margin: 2rem 0;"></iframe>

## About this model

The 3D model includes:

- **PCB** — 120 × 44 mm + 8 mm ear, 2.0 mm thickness with black solder mask and white silkscreen
- **Components** — ESP32-S3 module, OLED display, 10 tactile buttons, 13 addressable RGB LEDs
- **Silkscreen** — Front frame, scene button icons, touch area indicator, rear silkscreen with component labels

Click and drag to rotate, scroll to zoom, right-click to pan.

## Desk stand

The desk stand (leggio) is designed to hold PlancH at a 20° angle. You can [learn more](../accessories/desk-stand.md) about printing and assembling it.

## Model source

The 3D model is exported directly from the KiCad PCB design file using `kicad-cli`. All dimensions and component placements match the actual board. Find the source files on [GitHub](https://github.com/caccia78/PlancH/tree/main/hardware).
