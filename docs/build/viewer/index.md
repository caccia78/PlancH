# 3D Viewer

Explore the complete PlancH circuit board in 3D. Rotate, zoom, and inspect every component, connection, and detail.
Switch on the LEDs by group, as in the firmware, and turn the board over to see the backlight.

--8<-- "model-viewer.html"

<div style="margin: 2rem 0 0.5rem; border: 1px solid var(--md-border-color); border-radius: 8px; overflow: hidden;">
  <model-viewer 
    id="planch-3d"
    src="/PlancH/assets/planch-viewer.glb"
    alt="PlancH 3D model"
    auto-rotate
    camera-controls
    style="width: 100%; height: 600px; display: block;">
    <!-- HDR rendering: only the LEDs, with emissive strength above 1, pass the threshold -->
    <effect-composer render-mode="quality">
      <bloom-effect strength="0.8" threshold="3" radius="0.6"></bloom-effect>
    </effect-composer>
  </model-viewer>
</div>

<p>
  <button type="button" class="md-button" data-viewer-background="planch-3d" aria-pressed="false">Dark background</button>
</p>
<p id="planch-led-controlli">
  LEDs:
  <button type="button" class="md-button" data-led="led_stato" aria-pressed="false">Status</button>
  <button type="button" class="md-button" data-led="led_scene" aria-pressed="false">Scenes</button>
  <button type="button" class="md-button" data-led="led_retro" aria-pressed="false">Backlight</button>
  <label>Colour <input type="color" value="#ff7043"></label>
</p>

<script>planchLeds("planch-3d", "planch-led-controlli");</script>

Use your mouse to:

- **Rotate** — Click and drag
- **Zoom** — Scroll wheel  
- **Pan** — Right-click and drag

## About this model

The 3D model includes:

- **PCB** — 120 × 44 mm + 8 mm ear, 2.0 mm thickness with black solder mask and white silkscreen
- **Components** — ESP32-S3 module, OLED display, 10 tactile buttons, 13 addressable RGB LEDs
- **Silkscreen** — Front frame, scene button icons, touch area indicator, rear component labels

## Desk stand

The desk stand (leggio) is designed to hold PlancH at a 20° angle. You can [learn more](../../accessories/desk-stand.md) about printing and assembling it.

## Source files

The 3D model is exported directly from the KiCad PCB design using `kicad-cli`. All dimensions and placements match the actual board. Source files are on [GitHub](https://github.com/caccia78/PlancH/tree/main/hardware).
