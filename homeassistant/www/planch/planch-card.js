/*
 * PlancH card for Home Assistant: the panel drawn on its own picture, live.
 * Shows the display (from the "Screen" text sensor), the 13 LEDs (from the "LEDs" sensor), the
 * glow of the 6 backlight LEDs around the board, and presses the keys when you tap them.
 * Made for the test bench firmware (firmware/planch-test.yaml), which publishes both sensors.
 *
 * Install: copy the folder planch/ (this file and the two .webp pictures) to /config/www/,
 * add the resource /local/planch/planch-card.js?v=1.1 (JavaScript module) under
 * Settings > Dashboards > Resources (change ?v= after every update: browsers cache it hard),
 * then add a card:
 *
 *   type: custom:planch-card
 *   device: planch_test        # prefix of the entity IDs (default)
 *
 * Optional: screen, leds (sensor entity IDs), buttons (prefix of the button entity IDs),
 * images (folder of the pictures, default /local/planch), zoom (false hides the enlarged
 * display under the board; the real one is 22 x 11 mm and small on screen).
 *
 * Coordinates in mm as in the PCB design (SPEC.md): X to the right, Y down, origin at the top
 * left of the main body; the ear goes to Y = -8. The pictures are 10 px per mm, 120 x 52 mm.
 */

const VERSION = "1.1";   // also in the resource URL: /local/planch/planch-card.js?v=1.1
console.info(`%c PLANCH-CARD %c ${VERSION} `, "color:#fff;background:#ff7043;font-weight:600",
             "color:#ff7043;background:#26262a");

const BOARD = { x0: 0, y0: -8, w: 120, h: 52 };
const MARGIN = 16; // mm around the board, room for the backlight glow
const STAGE = { w: BOARD.w + 2 * MARGIN, h: BOARD.h + 2 * MARGIN };

// LEDs in chain order: D1-D3 status (ear), D4-D7 next to the scene keys, D8-D13 backlight (back)
const LEDS = [
  [35.5, -4], [43, -4], [50.5, -4],
  [76.25, 9], [76.25, 17.67], [76.25, 26.33], [76.25, 35],
  [80, 40.5], [116.5, 27], [109.5, 9.5], [78, 3], [45, 40.5], [11, 9],
];
const BACKLIGHT = [7, 8, 9, 10, 11, 12];

// Keys: entity suffix, centre (mm), size (mm)
const KEYS = [
  ["up", 98, 11.75], ["down", 98, 32.25], ["left", 87.75, 22], ["right", 108.25, 22],
  ["ok", 98, 22], ["back", 108.25, 32.25],
  ["scene_1", 69, 9], ["scene_2", 69, 17.67], ["scene_3", 69, 26.33], ["scene_4", 69, 35],
];
const TOUCH = [65, -4, 7];
const OLED = { x: 43, y: 21.05, w: 21.74, h: 10.86, px: 128, py: 64 };

// mm -> % of the stage; back view mirrored left to right
const sx = (x, back) => ((back ? BOARD.w - x : x) - BOARD.x0 + MARGIN) / STAGE.w * 100;
const sy = (y) => (y - BOARD.y0 + MARGIN) / STAGE.h * 100;
const pw = (w) => w / STAGE.w * 100;
const ph = (h) => h / STAGE.h * 100;

// "#rrggbb" -> {r, g, b, k}: hue at full brightness and brightness k (0-1)
function led(hex) {
  const m = /^#?([0-9a-f]{6})$/i.exec(hex || "");
  if (!m) return null;
  const n = parseInt(m[1], 16);
  const r = (n >> 16) & 255, g = (n >> 8) & 255, b = n & 255;
  const max = Math.max(r, g, b);
  if (max === 0) return null;
  const f = 255 / max;
  return { r: Math.round(r * f), g: Math.round(g * f), b: Math.round(b * f), k: Math.sqrt(max / 255) };
}

// Text of the "Screen" sensor -> {title, rows: [{left, right, sel}]} or null when off
function screen(text) {
  if (!text || text.startsWith("(screen off)") || text === "unknown" || text === "unavailable") return null;
  const lines = text.split("\n");
  const rows = lines.slice(1).map((l) => {
    const sel = l.startsWith("> ");
    const parts = l.slice(2).trim().split(/\s{2,}/);
    return { left: parts[0] || "", right: parts.length > 1 ? parts[parts.length - 1] : "", sel };
  });
  return { title: lines[0], rows };
}

class PlanchCard extends HTMLElement {
  setConfig(config) {
    const device = config.device || "planch_test";
    this._config = {
      screen: config.screen || `sensor.${device}_screen`,
      leds: config.leds || `sensor.${device}_leds`,
      buttons: config.buttons || `button.${device}_`,
      images: (config.images || "/local/planch").replace(/\/$/, ""),
      zoom: config.zoom !== false,
    };
    this._back = false;
    this._built = false;
  }

  set hass(hass) {
    this._hass = hass;
    if (!this._built) this._build();
    this._update();
  }

  getCardSize() {
    return 5;
  }

  static getStubConfig() {
    return { device: "planch_test" };
  }

  _build() {
    const root = this.attachShadow({ mode: "open" });
    root.innerHTML = `
      <style>
        ha-card { display: block; overflow: hidden; background: #121214; color: #e9e7e2; }
        .stage { position: relative; width: 100%; aspect-ratio: ${STAGE.w} / ${STAGE.h}; }
        .stage > * { position: absolute; }
        .board { left: ${sx(0)}%; top: ${sy(BOARD.y0)}%; width: ${pw(BOARD.w)}%; height: ${ph(BOARD.h)}%;
                 z-index: 2; pointer-events: none; }
        .glow { z-index: 1; border-radius: 50%; transform: translate(-50%, -50%);
                transition: opacity .25s, background .25s; pointer-events: none; }
        .led { z-index: 3; transform: translate(-50%, -50%); border-radius: 18%;
               transition: background .2s, box-shadow .2s; pointer-events: none; }
        canvas.oled { z-index: 3; image-rendering: pixelated; pointer-events: none; }
        .key { z-index: 4; transform: translate(-50%, -50%); border-radius: 22%; cursor: pointer;
               -webkit-tap-highlight-color: transparent; transition: background .12s, box-shadow .12s; }
        .key:hover { box-shadow: 0 0 0 2px rgba(255, 255, 255, .35); }
        .key.pressed { background: rgba(255, 255, 255, .28); box-shadow: 0 0 0 2px rgba(255, 112, 67, .9); }
        .touch { border-radius: 50%; }
        .bar { display: flex; gap: 14px; align-items: center; padding: 4px 14px 14px; font-size: 13px; }
        .bar .side { flex: 1; display: flex; flex-direction: column; align-items: flex-end; gap: 8px;
                     text-align: right; opacity: .85; }
        canvas.zoom { width: min(56%, 384px); aspect-ratio: 2 / 1; image-rendering: pixelated;
                      background: #000; border: 6px solid #050506; border-radius: 6px;
                      box-shadow: 0 0 0 1px #2c2c31; }
        .nozoom canvas.zoom { display: none; }
        button { font: inherit; color: inherit; background: #26262a; border: 1px solid #3a3a40;
                 border-radius: 6px; padding: 4px 10px; cursor: pointer; }
        .back .front-only { display: none; }
      </style>
      <ha-card>
        <div class="stage"></div>
        <div class="bar"><canvas class="zoom" width="128" height="64"></canvas>
          <div class="side"><span class="info"></span><button class="flip">Show back</button></div></div>
      </ha-card>`;
    const stage = root.querySelector(".stage");
    this._stage = stage;
    this._zoom = root.querySelector("canvas.zoom");
    stage.parentElement.classList.toggle("nozoom", !this._config.zoom);

    // Backlight glow behind the board, from the six LEDs on the back
    this._glows = BACKLIGHT.map((i) => {
      const g = document.createElement("div");
      g.className = "glow";
      Object.assign(g.style, { left: `${sx(LEDS[i][0])}%`, top: `${sy(LEDS[i][1])}%`,
                               width: `${pw(40)}%`, height: `${ph(40)}%`, opacity: 0 });
      stage.appendChild(g);
      return g;
    });

    this._img = document.createElement("img");
    this._img.className = "board";
    this._img.alt = "PlancH";
    stage.appendChild(this._img);

    // The 13 LEDs: windows on the front (D1-D7), all of them on the back
    this._leds = LEDS.map((p, i) => {
      const d = document.createElement("div");
      d.className = "led" + (i >= 7 ? " back-only" : "");
      stage.appendChild(d);
      return d;
    });

    // Display
    this._canvas = document.createElement("canvas");
    this._canvas.className = "oled front-only";
    this._canvas.width = OLED.px;
    this._canvas.height = OLED.py;
    Object.assign(this._canvas.style, { left: `${sx(OLED.x - OLED.w / 2)}%`, top: `${sy(OLED.y - OLED.h / 2)}%`,
                                        width: `${pw(OLED.w)}%`, height: `${ph(OLED.h)}%` });
    stage.appendChild(this._canvas);

    // Keys and touch area
    const key = (name, x, y, size, extra) => {
      const k = document.createElement("div");
      k.className = "key front-only" + (extra ? ` ${extra}` : "");
      k.title = name.replace("_", " ");
      Object.assign(k.style, { left: `${sx(x)}%`, top: `${sy(y)}%`, width: `${pw(size)}%`, height: `${ph(size)}%` });
      k.addEventListener("click", () => this._press(name, k));
      stage.appendChild(k);
    };
    KEYS.forEach(([n, x, y]) => key(n, x, y, 7.5));
    key("touch", TOUCH[0], TOUCH[1], TOUCH[2], "touch");

    root.querySelector(".flip").addEventListener("click", () => {
      this._back = !this._back;
      this._update();
    });
    this._built = true;
  }

  _press(name, el) {
    el.classList.add("pressed");
    setTimeout(() => el.classList.remove("pressed"), 180);
    this._hass.callService("button", "press", { entity_id: `${this._config.buttons}${name}` });
  }

  _update() {
    const c = this._config, h = this._hass;
    const back = this._back;
    this._stage.parentElement.classList.toggle("back", back);
    this._img.src = `${c.images}/planch-${back ? "back" : "front"}.webp`;
    this.shadowRoot.querySelector(".flip").textContent = back ? "Show front" : "Show back";

    const ledState = h.states[c.leds];
    const colours = (ledState ? ledState.state : "").split(" ");
    const shown = [];
    this._leds.forEach((d, i) => {
      const on = led(colours[i]);
      const visible = back || i < 7;
      const size = i < 7 ? (back ? 3.4 : 3.2) : 5.2;   // MINI-E window / body, 5050 body
      Object.assign(d.style, { display: visible ? "block" : "none", left: `${sx(LEDS[i][0], back)}%`,
                               top: `${sy(LEDS[i][1])}%`, width: `${pw(size)}%`, height: `${ph(size)}%` });
      if (on) {
        const rgb = `${on.r}, ${on.g}, ${on.b}`;
        d.style.background = `rgba(${rgb}, ${0.35 + 0.65 * on.k})`;
        d.style.boxShadow = `0 0 ${4 + 14 * on.k}px ${2 + 6 * on.k}px rgba(${rgb}, ${0.25 + 0.6 * on.k})`;
        if (i >= 7 && !shown.includes("backlight")) shown.push("backlight");
      } else {
        d.style.background = "transparent";
        d.style.boxShadow = "none";
      }
    });

    // Glow of the backlight around the board (front and back views)
    BACKLIGHT.forEach((i, j) => {
      const on = led(colours[i]);
      const g = this._glows[j];
      g.style.left = `${sx(LEDS[i][0], back)}%`;
      if (on) {
        g.style.background = `radial-gradient(closest-side, rgba(${on.r}, ${on.g}, ${on.b}, .75), rgba(${on.r}, ${on.g}, ${on.b}, 0))`;
        g.style.opacity = 0.25 + 0.75 * on.k;
      } else {
        g.style.opacity = 0;
      }
    });

    this._draw(h.states[c.screen] ? h.states[c.screen].state : null);
    this._copyZoom();
    const info = this.shadowRoot.querySelector(".info");
    info.textContent = !ledState ? `Entity ${c.leds} not found` : back ? "Back: backlight D8-D13" : "Tap the keys and the touch area";
  }

  _draw(text) {
    const ctx = this._canvas.getContext("2d");
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, OLED.px, OLED.py);
    const s = screen(text);
    if (!s) return;
    ctx.textBaseline = "top";
    ctx.fillStyle = "#fff";
    ctx.font = "500 10px Roboto, Arial, sans-serif";
    ctx.fillText(s.title, 0, 1, 127);
    ctx.fillRect(0, 12, 128, 1);
    ctx.font = "10px Roboto, Arial, sans-serif";
    s.rows.slice(0, 4).forEach((r, i) => {
      const y = 14 + i * 12;
      if (r.sel) {
        ctx.fillStyle = "#fff";
        ctx.fillRect(0, y, 128, 12);
      }
      ctx.fillStyle = r.sel ? "#000" : "#fff";
      ctx.textAlign = "left";
      ctx.fillText(r.left, 2, y + 1, 90);
      ctx.textAlign = "right";
      ctx.fillText(r.right, 126, y + 1);
    });
    ctx.textAlign = "left";
  }

  // Enlarged copy of the display under the board, sharp pixels
  _copyZoom() {
    const z = this._zoom.getContext("2d");
    z.imageSmoothingEnabled = false;
    z.drawImage(this._canvas, 0, 0);
  }
}

customElements.define("planch-card", PlanchCard);
window.customCards = window.customCards || [];
window.customCards.push({ type: "planch-card", name: "PlancH", description: "The PlancH panel, live: display, LEDs and keys" });
