# 3D Viewer

Explore PlancH in 3D — rotate, zoom, and pan to inspect every detail of the circuit board.

<div style="margin: 2rem 0; border: 1px solid var(--md-border-color); border-radius: 8px; overflow: hidden; background: var(--md-code-bg-color);">
  <model-viewer 
    id="planch-3d"
    src="../../../assets/planch-viewer.glb"
    alt="PlancH 3D model"
    auto-rotate
    camera-controls
    style="width: 100%; height: 500px; display: block;">
  </model-viewer>
  <script type="module" src="https://ajax.googleapis.com/ajax/libs/model-viewer/3.5.0/model-viewer.min.js"></script>
</div>

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
