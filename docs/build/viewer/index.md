# 3D Viewer

Explore the complete PlancH circuit board in 3D. Rotate, zoom, and inspect every component, connection, and detail.

## View the 3D model

[**Open 3D Viewer** →](../../assets/viewer-3d.html){: .md-button .md-button--primary }

The viewer opens in a full window. Use your mouse to:

- **Rotate** — Click and drag
- **Zoom** — Scroll wheel
- **Pan** — Right-click and drag

## About this model

The 3D model includes:

- **PCB** — 120 × 44 mm + 8 mm ear, 2.0 mm thickness with black solder mask and white silkscreen
- **Components** — ESP32-S3 module, OLED display, 10 tactile buttons, 13 addressable RGB LEDs
- **Silkscreen** — Front frame, scene button icons, touch area indicator, rear component labels

## Desk stand

The desk stand (leggio) is designed to hold PlancH at a 20° angle. You can [learn more](../accessories/desk-stand.md) about printing and assembling it.

## Source files

The 3D model is exported directly from the KiCad PCB design using `kicad-cli`. All dimensions and placements match the actual board. Source files are on [GitHub](https://github.com/caccia78/PlancH/tree/main/hardware).
