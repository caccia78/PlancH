"""PlancH: supporto da parete (build123d).

Staffa incollata dietro la scheda e base fissata alla parete con strisce adesive rimovibili;
si agganciano con magneti Ø 5 x 3 mm e due pioli conici di centraggio. Le sagome sono
concentriche al contorno del corpo della scheda: la staffa è un anello, la base la stessa
sagoma piena. Parametri e scelte in SPEC.md, sezione "Supporto da parete".

Coordinate come nello STEP di KiCad: X a destra, Y verso l'orecchia (Y di SPEC.md cambiata
di segno: appesa, verso l'alto), Z verso il fronte. Z = 0 è la faccia posteriore della
scheda, la parete sta verso -Z.

Uso (dalla radice del repository):
    uv run --no-project --python 3.12 --with build123d --with pyvista --with shapely \\
        python mechanical/parete.py [--render]

Con --viewer <GLB della scheda> scrive anche parete.glb e parete-assieme.glb per il viewer
3D del sito (servono trimesh e pillow); la scheda è output/planch-viewer.glb di
tools/viewer_glb.py.
"""

import argparse
from pathlib import Path

from build123d import (
    Align, Cone, Cylinder, Face, GeomType, Mesher, Pos, Wire, export_step, export_stl,
    extrude, import_step,
)
from shapely.geometry import Point, Polygon, box

RADICE = Path(__file__).resolve().parent.parent
STEP_SCHEDA = RADICE / "output" / "planch.step"
USCITA = RADICE / "output"

# Parametri (SPEC.md, "Supporto da parete")
DISTANZA = 9.5           # sagome a questa distanza dal bordo del corpo: i LED 5050 restano fuori
ANELLO = 8.0             # larghezza dell'anello della staffa = diametro di appoggi e dischi VHB
RACCORDO = 3.0           # raggio minimo degli angoli
ADESIVO = 1.0            # dischi VHB tra appoggi e scheda
APPOGGIO = 2.4           # appoggi tra anello e adesivo
SPESSORE = 3.0           # anello della staffa
BASE = 4.0               # spessore della base
MAGNETE_D, MAGNETE_H = 5.0, 3.0
SEDE_GIOCO = 0.2         # sedi dei magneti: Ø 5,2 x 3,1
PIOLO_D1, PIOLO_D2, PIOLO_H = 5.0, 3.0, 2.0   # cono di centraggio: base, punta, altezza
PIOLO_GIOCO = 0.2
MARGINE = 0.5            # appoggi lontani almeno tanto dai componenti del retro

# Coordinate di SPEC.md; appoggi sulla linea media dell'anello, dove il retro è libero
APPOGGI_SPEC = [(35.5, 20.33), (51.76, 5.5), (52.12, 30.5), (94.93, 13.5), (95.12, 30.5), (106.5, 24.09)]
PIOLI_SPEC = [(73.5, 13.5), (73.5, 30.5)]
PIAZZOLE_TEST_SPEC = [(68, 16.5), (68, 21), (68, 25.5), (68, 30)]   # TP1-TP4, Ø 1,5
LED_RETRO_SPEC = [(80, 40.5), (116.5, 27), (109.5, 9.5), (78, 3), (45, 40.5), (11, 9)]  # D8-D13

Z_ANELLO = -(ADESIVO + APPOGGIO + SPESSORE)   # faccia posteriore della staffa
Z_BASE = Z_ANELLO - BASE                      # parete


def p(x, y_spec):
    return (x, -y_spec)


def contorno(scheda, passo=0.3):
    """Contorno della scheda come poligono shapely, in coordinate STEP."""
    pcb = next(c for c in scheda.children if c.label.endswith("_PCB"))
    piane = [f for f in pcb.faces() if f.geom_type == GeomType.PLANE and f.normal_at().Z > 0.99]
    filo = max(piane, key=lambda f: f.center().Z).outer_wire()
    n = int(filo.length / passo)
    return Polygon([(q.X, q.Y) for q in (filo.position_at(i / n) for i in range(n))]).buffer(0)


def sagome(scheda):
    """Sagoma piena (base) e foro dell'anello (staffa), concentrici al bordo del corpo."""
    corpo = contorno(scheda).intersection(box(22, -60, 200, 20))   # senza la linguetta del modulo

    def liscio(g):   # raccorda angoli convessi e concavi
        return g.buffer(-RACCORDO, quad_segs=32).buffer(2 * RACCORDO, quad_segs=32).buffer(-RACCORDO, quad_segs=32)

    piena = liscio(corpo.buffer(-DISTANZA, quad_segs=32))
    foro = liscio(corpo.buffer(-(DISTANZA + ANELLO), quad_segs=32))
    return piena, foro


def faccia(poligono):
    f = Face(Wire.make_polygon(list(poligono.exterior.coords)[:-1], close=True))
    for buco in poligono.interiors:
        f -= Face(Wire.make_polygon(list(buco.coords)[:-1], close=True))
    return f


def verifica(scheda, piena, foro):
    """Appoggi lontani dai componenti del retro e sulla linea media dell'anello; sagome
    fuori dalle impronte dei LED di retroilluminazione (la luce va sulla parete)."""
    retro = []
    for c in scheda.children:
        bb = c.bounding_box()
        if bb.min.Z < -0.05 and not c.label.endswith("_PCB"):
            retro.append(box(bb.min.X, bb.min.Y, bb.max.X, bb.max.Y))
    retro += [Point(p(*t)).buffer(0.75) for t in PIAZZOLE_TEST_SPEC]
    anello = piena.difference(foro)
    for t in APPOGGI_SPEC:
        cerchio = Point(p(*t)).buffer(ANELLO / 2)
        assert not any(cerchio.buffer(MARGINE).intersects(r) for r in retro), f"appoggio {t} su un componente"
        assert cerchio.difference(anello.buffer(0.3)).area < 0.5, f"appoggio {t} fuori dall'anello"
    for n, (x, y) in enumerate(LED_RETRO_SPEC, 8):
        qx, qy = p(x, y)
        d = box(qx - 2.5, qy - 2.5, qx + 2.5, qy + 2.5).distance(piena)
        assert d > 0.5, f"la sagoma copre D{n} ({d:.1f} mm)"


def costruisci():
    scheda = import_step(str(STEP_SCHEDA))
    piena, foro = sagome(scheda)
    verifica(scheda, piena, foro)

    def colonna(t, z, altezza, d):
        return Pos(*p(*t), z) * Cylinder(d / 2, altezza, align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Staffa: anello verso la parete, appoggi fino all'adesivo; sedi dei magneti e dei pioli
    # aperte sulla faccia posteriore
    staffa = Pos(0, 0, Z_ANELLO) * extrude(faccia(piena.difference(foro)), SPESSORE, dir=(0, 0, 1))
    for t in APPOGGI_SPEC:
        staffa += colonna(t, Z_ANELLO + SPESSORE - 0.1, APPOGGIO + 0.1, ANELLO)   # compenetrati: un solido
        staffa -= colonna(t, Z_ANELLO, MAGNETE_H + 0.1, MAGNETE_D + SEDE_GIOCO)
    for t in PIOLI_SPEC:
        staffa -= Pos(*p(*t), Z_ANELLO) * Cone((PIOLO_D1 + 2 * PIOLO_GIOCO) / 2, (PIOLO_D2 + 2 * PIOLO_GIOCO) / 2,
                                               PIOLO_H + 0.3, align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Base: sagoma piena, faccia posteriore tutta piana per le strisce; magneti e pioli davanti
    base = Pos(0, 0, Z_BASE) * extrude(faccia(piena), BASE, dir=(0, 0, 1))
    for t in APPOGGI_SPEC:
        base -= colonna(t, Z_ANELLO - MAGNETE_H - 0.1, MAGNETE_H + 0.1, MAGNETE_D + SEDE_GIOCO)
    for t in PIOLI_SPEC:
        base += Pos(*p(*t), Z_ANELLO - 0.1) * Cone(PIOLO_D1 / 2 + 0.04, PIOLO_D2 / 2, PIOLO_H + 0.1,
                                                   align=(Align.CENTER, Align.CENTER, Align.MIN))
    return scheda, staffa, base


def esporta(staffa, base):
    for forma, nome in [(staffa, "parete-staffa"), (base, "parete-base")]:
        piano = forma.translate((0, 0, -forma.bounding_box().min.Z))   # faccia verso la parete sul piatto
        export_step(piano, str(USCITA / f"{nome}.step"))
        export_stl(piano, str(USCITA / f"{nome}.stl"), tolerance=0.01, angular_tolerance=0.1)
        m = Mesher()
        m.add_shape(piano, linear_deflection=0.01, angular_deflection=0.1)
        m.write(str(USCITA / f"{nome}.3mf"))


def esporta_glb(scheda_glb, staffa, base):
    """GLB per il viewer 3D del sito, in metri con Y in alto (glTF). Appesa, l'alto della
    parete è già Y e Z punta verso chi guarda: basta la scala. La scheda viene da
    tools/viewer_glb.py, nel sistema di kicad-cli (scheda orizzontale): si raddrizza.
    - parete.glb: muro, base e staffa staccata di 15 mm, per vedere i due pezzi;
    - parete-assieme.glb: scheda appesa con l'alone della retroilluminazione sul muro
      (materiale luce_retro, che il viewer accende con i LED del retro)."""
    import numpy as np
    import trimesh
    from PIL import Image
    from trimesh.visual.material import PBRMaterial

    metri = np.diag([0.001, 0.001, 0.001, 1.0])
    kicad = metri @ np.array([[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1]], dtype=float)

    def mesh(forma, nome, colore, sposta=0.0):
        v, t = forma.translate((0, 0, sposta)).tessellate(0.05, 0.2)
        m = trimesh.Trimesh(np.array([(q.X, q.Y, q.Z) for q in v]), np.array(t), process=False)
        m.apply_transform(metri)
        m.vertex_normals   # calcolate qui, così l'export le include
        m.visual = trimesh.visual.TextureVisuals(material=PBRMaterial(
            name=nome, baseColorFactor=colore, metallicFactor=0.0, roughnessFactor=0.7))
        return m

    def rettangolo(x0, y0, x1, y1, z):
        v = np.array([(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)], dtype=float) * 0.001
        return trimesh.Trimesh(v, [[0, 1, 2], [0, 2, 3]], process=False)

    # Muro un po' più grande della scheda, appena dietro la base
    x0, x1, y0, y1 = -20.0, 140.0, -66.0, 22.0   # coordinate STEP (Y = -Y di SPEC)
    muro = rettangolo(x0, y0, x1, y1, Z_BASE - 0.05)
    muro.visual = trimesh.visual.TextureVisuals(material=PBRMaterial(
        name="muro", baseColorFactor=[0.45, 0.45, 0.44, 1.0], metallicFactor=0.0, roughnessFactor=0.95))   # grigio chiaro: stacca dal PLA bianco
    colore_base, colore_staffa = [0.807, 0.791, 0.753, 1.0], [0.807, 0.791, 0.753, 1.0]   # PLA #e8e6e1

    pezzi = trimesh.Scene()
    pezzi.add_geometry(muro, node_name="muro", geom_name="muro")
    pezzi.add_geometry(mesh(base, "base", colore_base), node_name="base", geom_name="base")
    pezzi.add_geometry(mesh(staffa, "staffa", colore_staffa, 15), node_name="staffa", geom_name="staffa")
    pezzi.export(str(USCITA / "parete.glb"), include_normals=True)

    # Alone: una macchia gaussiana per LED del retro, su un piano davanti al muro; la
    # texture dà la forma (alfa) e l'intensità (emissione), il viewer ne cambia il colore
    px = 4   # pixel per mm
    xs = np.arange(x0, x1, 1 / px) + 0.5 / px
    ys = np.arange(y1, y0, -1 / px) - 0.5 / px
    gx, gy = np.meshgrid(xs, ys)
    luce = np.zeros_like(gx)
    for lx, ly in LED_RETRO_SPEC:
        qx, qy = p(lx, ly)
        luce += np.exp(-((gx - qx) ** 2 + (gy - qy) ** 2) / (2 * 14.0 ** 2))
    luce = np.clip(luce / 1.2, 0, 1) ** 1.5
    a8 = (luce * 255).astype(np.uint8)
    alfa = Image.fromarray(np.dstack([np.full_like(a8, 255)] * 3 + [a8]), "RGBA")
    emissione = Image.fromarray(np.dstack([a8] * 3), "RGB")
    alone = rettangolo(x0, y0, x1, y1, Z_BASE + 0.3)
    alone.visual = trimesh.visual.TextureVisuals(
        uv=np.array([(0, 0), (1, 0), (1, 1), (0, 1)], dtype=float),
        material=PBRMaterial(name="luce_retro", baseColorFactor=[1.0, 1.0, 1.0, 0.0],
                             baseColorTexture=alfa, emissiveTexture=emissione,
                             metallicFactor=0.0, roughnessFactor=1.0, alphaMode="BLEND"))

    assieme = trimesh.load(str(scheda_glb), force="scene")
    assieme.apply_transform(metri @ np.linalg.inv(kicad))
    assieme.add_geometry(muro, node_name="muro", geom_name="muro")
    assieme.add_geometry(alone, node_name="luce_retro", geom_name="luce_retro")
    assieme.add_geometry(mesh(base, "base", colore_base), node_name="base", geom_name="base")
    assieme.add_geometry(mesh(staffa, "staffa", colore_staffa), node_name="staffa", geom_name="staffa")
    assieme.export(str(USCITA / "parete-assieme.glb"), include_normals=True)


def render(parti, nome, direzione, su=(0, 1, 0)):
    """Render di controllo con PyVista (VTK), fuori schermo; direzione = da dove si guarda."""
    import numpy as np
    import pyvista as pv

    pl = pv.Plotter(off_screen=True, window_size=(1600, 1000))
    pl.set_background("white")
    for forma, colore in parti:
        v, t = forma.tessellate(0.02, 0.2)
        v = np.array([(q.X, q.Y, q.Z) for q in v])
        facce = np.hstack([np.full((len(t), 1), 3), np.array(t)]).ravel()
        pl.add_mesh(pv.PolyData(v, facce), color=colore, specular=0.15, ambient=0.25)
    pl.enable_parallel_projection()
    pl.camera_position = [tuple(np.array(direzione) * 500), (60, -20, -4), su]
    pl.reset_camera()
    pl.camera.zoom(1.2)
    pl.screenshot(str(USCITA / nome))
    pl.close()


def parti_scheda(scheda):
    parti = []
    for c in scheda.children:
        if c.label.endswith("_PCB"):
            colore = "#202124"
        elif c.label.startswith(("SW_", "OLED")):
            colore = "#3a3a3a"
        elif c.label.startswith("Waveshare"):
            colore = "#2f6f4f"
        else:
            colore = "#b0b0b0"
        parti.append((c, colore))
    return parti


if __name__ == "__main__":
    a = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    a.add_argument("--scheda", "--board", dest="scheda", type=Path, default=STEP_SCHEDA,
                   help="STEP della scheda / board STEP file")
    a.add_argument("--uscita", "--out", dest="uscita", type=Path, default=USCITA,
                   help="cartella dei file generati / output folder")
    a.add_argument("--render", action="store_true", help="genera anche i PNG di controllo")
    a.add_argument("--viewer", type=Path, metavar="GLB_SCHEDA",
                   help="GLB della scheda (tools/viewer_glb.py): scrive anche i GLB per il viewer 3D del sito")
    args = a.parse_args()
    STEP_SCHEDA, USCITA = args.scheda.resolve(), args.uscita.resolve()

    scheda, staffa, base = costruisci()
    for forma, nome in [(staffa, "Staffa"), (base, "Base")]:
        bb = forma.bounding_box()
        print(f"{nome}: {bb.size.X:.1f} x {bb.size.Y:.1f} x {bb.size.Z:.1f} mm, "
              f"volume {forma.volume / 1000:.1f} cm³, solidi {len(forma.solids())}")
    print(f"Distanza scheda-parete: {-Z_BASE:.1f} mm")
    esporta(staffa, base)
    if args.viewer:
        esporta_glb(args.viewer.resolve(), staffa, base)
    if args.render:
        render([(staffa, "#e8e6e1"), (base.translate((0, 0, -25)), "#cfd8dc")],
               "parete-render.png", (0.45, -0.55, -0.7))
        render(parti_scheda(scheda) + [(staffa, "#e8e6e1"), (base, "#cfd8dc")],
               "parete-assieme-retro.png", (-0.35, 0.3, -0.9))
        render(parti_scheda(scheda) + [(staffa, "#e8e6e1"), (base, "#cfd8dc")],
               "parete-assieme-lato.png", (1, 0, 0))
