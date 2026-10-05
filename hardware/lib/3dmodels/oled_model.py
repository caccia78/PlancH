"""Modello STEP del modulo OLED 0,96" SSD1306 I2C dell'utente (24,8 x 26,9 mm), per il render di PlancH.
# Rigenerare con: uv run --python 3.12 --with cadquery python oled_model.py OLED_SSD1306_0.96in_I2C_24.8x26.9mm.step
# (eseguito dalla cartella hardware/lib/3dmodels). Per le misure reali del vetro modificare GW, GH, G_TOP.

Sistema di riferimento del modello KiCad: origine = centro del modulo (origine del footprint),
X a destra, Y verso l'alto (= -Y della scheda), Z = 0 sulla faccia superiore del PCB PlancH.
Misure del modulo e dei pin da SPEC.md; vetro e area attiva con valori tipici di un 0,96".
"""
import sys
import cadquery as cq

out = sys.argv[1]

W, H = 24.8, 26.9                 # modulo
T_PCB = 1.0                        # spessore PCB del modulo
SPACER = 2.5                       # plastica del connettore tra PlancH e il modulo
PIN_Y = H / 2 - 1.3                # fila dei pin a 1,3 mm dal bordo superiore (Y modello verso l'alto)
PINS_X = [-3.81, -1.27, 1.27, 3.81]
GW, GH, G_TOP = 24.6, 17.0, 5.0    # vetro: larghezza, altezza, distanza dal bordo superiore
G_T = 1.4
AW, AH, A_OFF = 21.74, 10.86, -1.0 # area attiva; spostamento verso l'alto rispetto al centro del vetro

def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))

z_pcb0, z_pcb1 = SPACER, SPACER + T_PCB
pcb = box(-W/2, W/2, -H/2, H/2, z_pcb0, z_pcb1)
header = box(-5.08, 5.08, PIN_Y - 1.27, PIN_Y + 1.27, 0, SPACER)
pins = None
for x in PINS_X:
    p = box(x - 0.32, x + 0.32, PIN_Y - 0.32, PIN_Y + 0.32, -2.0, z_pcb1 + 0.6)
    pins = p if pins is None else pins.union(p)
pads = None
for x in PINS_X:
    c = cq.Workplane("XY").circle(0.8).circle(0.32).extrude(0.05).translate((x, PIN_Y, z_pcb1))
    pads = c if pads is None else pads.union(c)

g_y1 = H/2 - G_TOP; g_y0 = g_y1 - GH
glass = box(-GW/2, GW/2, g_y0, g_y1, z_pcb1, z_pcb1 + G_T)
gc = (g_y0 + g_y1) / 2 - A_OFF
active = box(-AW/2, AW/2, gc - AH/2, gc + AH/2, z_pcb1 + G_T, z_pcb1 + G_T + 0.02)
fpc = box(-6.0, 6.0, -H/2 + 0.6, g_y0, z_pcb1, z_pcb1 + 0.12)

assy = cq.Assembly(name="OLED_SSD1306_0.96in_I2C")
assy.add(pcb, name="pcb", color=cq.Color(0.08, 0.20, 0.55))
assy.add(header, name="header", color=cq.Color(0.05, 0.05, 0.05))
assy.add(pins, name="pins", color=cq.Color(0.85, 0.72, 0.35))
assy.add(pads, name="pads", color=cq.Color(0.80, 0.80, 0.80))
assy.add(glass, name="glass", color=cq.Color(0.16, 0.17, 0.19))
assy.add(active, name="active", color=cq.Color(0.01, 0.01, 0.01))
assy.add(fpc, name="fpc", color=cq.Color(0.80, 0.50, 0.12))
assy.save(out, exportType="STEP")
print("ok", out)
