"""PlancH: leggio da scrivania (build123d).

La scheda, con i suoi piedini adesivi, si appoggia nel vano di una cornice che ne segue
il contorno; la cornice è inclinata su una base piena. Parametri e scelte in SPEC.md,
sezione "Leggio da scrivania".

Coordinate come nello STEP di KiCad: X a destra, Y verso l'orecchia (Y di SPEC.md
cambiata di segno), Z verso il fronte della scheda. Nel sistema della cornice Z = 0 è
la faccia superiore del fondo, dove poggiano i piedini.

Uso (dalla radice del repository):
    uv run --no-project --python 3.12 --with build123d --with pyvista --with trimesh \
        python mechanical/leggio.py [--pcb 1.6] [--piedini 5] [--render]

Per il viewer 3D del sito scrive anche leggio.glb e leggio-assieme.glb; la scheda
dell'assieme è output/planch-viewer.glb (kicad-cli, con serigrafia e maschera).
"""

import argparse
import math
from pathlib import Path

from build123d import (
    Align, Box, Compound, Cylinder, Face, GeomType, Kind, Mesher, Pos, Rot,
    Wire, export_step, export_stl, extrude, import_step, offset,
)

RADICE = Path(__file__).resolve().parent.parent
STEP_SCHEDA = RADICE / "output" / "planch.step"
GLB_SCHEDA = RADICE / "output" / "planch-viewer.glb"
USCITA = RADICE / "output"

# Parametri (SPEC.md, "Leggio da scrivania")
INCLINAZIONE = 20.0      # gradi
PIEDINI = 5.0            # altezza dei piedini adesivi
SPESSORE_PCB = 2.0       # 1,6 per i campioni JLCPCB
GIOCO = 0.3              # tra scheda e cornice, per lato
PARETE = 2.4
FONDO = 2.0
RIBASSO_PARETE = 0.5     # sommità delle pareti sotto la faccia superiore della scheda
SOTTO_FONDO = 3.0        # spessore minimo della base sotto il bordo anteriore
FESSURA = 12.0           # larghezza delle fessure per la luce
GOMMINO_D, GOMMINO_H = 10.4, 1.0

PIEDI_SPEC = [(11, 17.75), (28, 6), (28, 38), (64, 7), (98, 22), (114, 38)]
GOMMINI_SPEC = [(8, 12), (30, 40), (95, 5), (113, 40)]

_D10 = (11.5 / math.hypot(11.5, 12.5), -12.5 / math.hypot(11.5, 12.5))  # D10 dal centro ghiera (98, 22)
FESSURE_SPEC = [  # (punto sul bordo in coord. SPEC, normale uscente in coord. SPEC)
    ((80, 44), (0, 1)),                                    # D8
    ((45, 44), (0, 1)),                                    # D12
    ((120, 27), (1, 0)),                                   # D9
    ((98 + 22 * _D10[0], 22 + 22 * _D10[1]), _D10),        # D10
    ((82.5, 0), (0, -1)),                                  # D11, spostata a destra per restare sul tratto dritto
]


def p(x, y_spec):
    """Punto in coordinate di SPEC.md (Y verso il basso) -> coordinate STEP."""
    return (x, -y_spec)


def contorno_scheda(scheda):
    """Faccia piana con il contorno esterno della scheda, a Z = 0."""
    pcb = next(c for c in scheda.children if c.label.endswith("_PCB"))
    piane = [f for f in pcb.faces() if f.geom_type == GeomType.PLANE and f.normal_at().Z > 0.99]
    sopra = max(piane, key=lambda f: f.center().Z)
    faccia = Face(sopra.outer_wire())
    return faccia.translate((0, 0, -faccia.center().Z))


def fessure_luce(piedini):
    """Fessure davanti ai LED di retroilluminazione D8-D12 (D13 esce dall'apertura USB)."""
    lunghezza = GIOCO + PARETE + 4
    tagli = []
    for (x, y), (nx, ny) in FESSURE_SPEC:
        ang = math.degrees(math.atan2(-ny, nx))
        cx, cy = p(x + nx * (lunghezza / 2 - 2), y + ny * (lunghezza / 2 - 2))
        tagli.append(Pos(cx, cy, 0) * Rot(Z=ang) * Box(lunghezza, FESSURA, piedini,
                                                       align=(Align.CENTER, Align.CENTER, Align.MIN)))
    return tagli


def luci_fessure(piedini):
    """Pannelli sottili a metà parete di ogni fessura: nel viewer 3D si illuminano con la
    retroilluminazione, che il modello non può proiettare davvero."""
    pannelli = []
    for (x, y), (nx, ny) in FESSURE_SPEC:
        ang = math.degrees(math.atan2(-ny, nx))
        d = GIOCO + PARETE / 2
        pannelli.append(Pos(*p(x + nx * d, y + ny * d), 0) * Rot(Z=ang)
                        * Box(0.2, FESSURA, piedini, align=(Align.CENTER, Align.CENTER, Align.MIN)))
    return pannelli


def poligono(sagoma, passo):
    """Contorno di una sagoma piana campionato a passo costante, con normale verso +Z.

    Le curve esatte del contorno (Bézier dell'orecchia e raccordi spostati) danno a
    OpenCascade booleane fallite con l'estrusione obliqua della base e mesh STL aperte
    negli angoli: un poligono fitto evita entrambi i problemi."""
    filo = sagoma.faces()[0].outer_wire()
    n = int(filo.length / passo)
    punti = [filo.position_at(i / n) for i in range(n)]
    faccia = Face(Wire.make_polygon(punti, close=True))
    if faccia.normal_at().Z < 0:
        faccia = Face(Wire.make_polygon(punti[::-1], close=True))
    return faccia


def costruisci(pcb=SPESSORE_PCB, piedini=PIEDINI, inclinazione=INCLINAZIONE):
    scheda = import_step(str(STEP_SCHEDA))
    bordo = contorno_scheda(scheda)
    vano = poligono(offset(bordo, GIOCO, kind=Kind.ARC), 0.2)                 # scarto < 0,01 mm
    esterno = poligono(offset(bordo, GIOCO + PARETE, kind=Kind.ARC), 0.3)     # scarto < 0,002 mm
    z_sommita = piedini + pcb - RIBASSO_PARETE

    # Cornice nel suo sistema di riferimento
    cornice = Pos(0, 0, -FONDO) * extrude(esterno, FONDO + z_sommita)
    cornice -= extrude(vano, z_sommita + 1)

    tagli = fessure_luce(piedini)
    # Apertura per l'USB-C sul bordo superiore della linguetta (Y 6)
    tagli.append(Box(16.5, 5, z_sommita + 1, align=(Align.MIN, Align.MIN, Align.MIN))
                 .translate((2.5, -7, 0)))
    # Tacca per le dita al centro del bordo inferiore
    tagli.append(Pos(62.5, -44, z_sommita + 2) * Rot(X=90) * Cylinder(9, 12))

    # Inclinazione e base piena fino alla scrivania
    rot = Rot(X=inclinazione)
    fondo_esterno = rot * Pos(0, 0, -FONDO) * esterno
    dz = SOTTO_FONDO - fondo_esterno.bounding_box().min.Z
    porta = Pos(0, 0, dz) * rot
    base = extrude(Pos(0, 0, dz) * fondo_esterno, 300, dir=(0, 0, -1))
    leggio = porta * cornice + base
    leggio &= Box(400, 400, 200, align=(Align.CENTER, Align.CENTER, Align.MIN))
    for t in tagli:
        leggio -= porta * t

    # Sedi dei gommini sotto la base
    for x, y in GOMMINI_SPEC:
        c = (porta * Pos(*p(x, y), -FONDO)).position
        leggio -= Pos(c.X, c.Y, 0) * Cylinder(GOMMINO_D / 2, GOMMINO_H, align=(Align.CENTER, Align.CENTER, Align.MIN))

    # Assieme: scheda appoggiata sui piedini
    piedi = [Pos(*p(x, y), 0) * Cylinder(5, piedini, align=(Align.CENTER, Align.CENTER, Align.MIN))
             for x, y in PIEDI_SPEC]
    # I sottoassiemi dello STEP non seguono lo spostamento del gruppo: si spostano uno per uno
    sposta = porta * Pos(0, 0, piedini)
    scheda_posata = Compound(children=[sposta * c for c in scheda.children], label="planch")
    piedi_posati = [porta * f for f in piedi]
    return leggio, scheda_posata, piedi_posati, sposta


def esporta(leggio, scheda, piedi):
    export_step(leggio, str(USCITA / "leggio.step"))
    export_stl(leggio, str(USCITA / "leggio.stl"), tolerance=0.01, angular_tolerance=0.1)
    m = Mesher()
    m.add_shape(leggio, linear_deflection=0.01, angular_deflection=0.1)
    m.write(str(USCITA / "leggio.3mf"))
    leggio.label = "leggio"
    for f in piedi:
        f.label = "piedino"
    export_step(Compound(children=[leggio, scheda, *piedi]), str(USCITA / "leggio-assieme.step"))


# Gruppi dei LED come nel firmware; il viewer 3D li accende cambiando questi materiali
GRUPPI_LED = {"led_stato": range(1, 4), "led_scene": range(4, 8), "led_retro": range(8, 14)}


def esporta_glb(leggio, piedi, sposta, piedini):
    """GLB per il viewer 3D del sito: leggio da solo e assieme con la scheda. glTF vuole metri
    e Y in alto: (x, y, z) dello STEP -> (x, z, -y) / 1000, come fa kicad-cli."""
    import numpy as np
    import trimesh
    from trimesh.visual.material import PBRMaterial

    gltf = np.diag([0.001, 0.001, 0.001, 1.0]) @ np.array(
        [[1, 0, 0, 0], [0, 0, 1, 0], [0, -1, 0, 0], [0, 0, 0, 1]], dtype=float)

    def mesh(forma, nome, colore, alpha="OPAQUE"):
        v, t = forma.tessellate(0.05, 0.2)
        m = trimesh.Trimesh(np.array([(q.X, q.Y, q.Z) for q in v]), np.array(t), process=False)
        m.apply_transform(gltf)
        m.vertex_normals   # calcolate qui, così l'export le include
        m.visual = trimesh.visual.TextureVisuals(material=PBRMaterial(
            name=nome, baseColorFactor=colore, metallicFactor=0.0, roughnessFactor=0.7,
            alphaMode=alpha))
        return m

    corpo = mesh(leggio, "leggio", [0.91, 0.90, 0.88, 1.0])
    trimesh.Scene({"leggio": corpo}).export(str(USCITA / "leggio.glb"), include_normals=True)

    # La scheda di kicad-cli è già nel sistema glTF: si porta sui piedini con lo stesso
    # spostamento dello STEP, espresso in quel sistema. Le parti si uniscono per materiale:
    # trimesh separa ogni primitiva (oltre 5000 mesh, troppe chiamate di disegno sui telefoni)
    t = sposta.wrapped.Transformation()
    m = np.vstack([[[t.Value(i, j) for j in range(1, 5)] for i in range(1, 4)], [0, 0, 0, 1]])
    porta = gltf @ m @ np.linalg.inv(gltf)
    scheda = trimesh.load(str(GLB_SCHEDA), force="scene")
    grigio = PBRMaterial(name="senza_materiale", baseColorFactor=[0.4, 0.4, 0.4, 1.0])
    led = {f"D{n}": g for g, numeri in GRUPPI_LED.items() for n in numeri}
    genitori = scheda.graph.transforms.parents

    def gruppo_led(nodo):
        while nodo in genitori:
            nodo = genitori[nodo]
            if nodo in led:
                return led[nodo]
        return None

    gruppi = {}
    for nodo in scheda.graph.nodes_geometry:
        tr, nome = scheda.graph[nodo]
        parte = scheda.geometry[nome]
        materiale = getattr(parte.visual, "material", grigio)   # alcune primitive non ne hanno
        g = gruppo_led(nodo)
        if g:   # LED interi (corpo e piedini) in un materiale proprio per gruppo
            materiale = PBRMaterial(name=g, baseColorFactor=[0.85, 0.85, 0.85, 1.0],
                                    metallicFactor=0.0, roughnessFactor=0.4)
        gruppi.setdefault(materiale.name, (materiale, []))[1].append((parte, porta @ tr))
    assieme = trimesh.Scene()
    for nome, (materiale, parti) in gruppi.items():
        v, vn, f, n = [], [], [], 0
        for parte, tr in parti:
            v.append(trimesh.transform_points(parte.vertices, tr))
            vn.append(parte.vertex_normals @ tr[:3, :3].T)
            f.append(parte.faces + n)
            n += len(parte.vertices)
        vn = np.vstack(vn)
        unita = trimesh.Trimesh(np.vstack(v), np.vstack(f), process=False,
                                vertex_normals=vn / np.linalg.norm(vn, axis=1, keepdims=True))
        unita.visual = trimesh.visual.TextureVisuals(material=materiale)
        assieme.add_geometry(unita, node_name=f"planch_{nome}", geom_name=f"planch_{nome}")
    assieme.add_geometry(corpo, node_name="leggio", geom_name="leggio")
    for i, f in enumerate(piedi, 1):
        assieme.add_geometry(mesh(f, "piedino", [0.85, 0.92, 0.97, 0.6], "BLEND"),
                             node_name=f"piedino_{i}", geom_name=f"piedino_{i}")
    cornice = sposta * Pos(0, 0, -piedini)   # sistema della cornice, come i tagli delle fessure
    luci = Compound(children=[cornice * f for f in luci_fessure(piedini)])
    assieme.add_geometry(mesh(luci, "luce_retro", [1.0, 1.0, 1.0, 0.0], "BLEND"),   # spente
                         node_name="luce_retro", geom_name="luce_retro")
    assieme.export(str(USCITA / "leggio-assieme.glb"), include_normals=True)


def render(parti, nome, direzione):
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
    pl.camera_position = [tuple(np.array(direzione) * 500), (60, -20, 10), (0, 0, 1)]
    pl.reset_camera()
    pl.camera.zoom(1.2)
    pl.screenshot(str(USCITA / nome))
    pl.close()


def parti_colorate(leggio, scheda, piedi):
    parti = [(leggio, "#e8e6e1")]
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
    parti += [(f, "#cfe3ef") for f in piedi]
    return parti


if __name__ == "__main__":
    a = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    a.add_argument("--pcb", type=float, default=SPESSORE_PCB,
                   help="spessore della scheda / board thickness (mm)")
    a.add_argument("--piedini", "--feet", dest="piedini", type=float, default=PIEDINI,
                   help="altezza dei piedini / rubber feet height (mm)")
    a.add_argument("--inclinazione", "--tilt", dest="inclinazione", type=float,
                   default=INCLINAZIONE, help="gradi / degrees")
    a.add_argument("--scheda", "--board", dest="scheda", type=Path, default=STEP_SCHEDA,
                   help="STEP della scheda / board STEP file")
    a.add_argument("--uscita", "--out", dest="uscita", type=Path, default=USCITA,
                   help="cartella dei file generati / output folder")
    a.add_argument("--render", action="store_true", help="genera anche i PNG di controllo")
    args = a.parse_args()
    STEP_SCHEDA, USCITA = args.scheda.resolve(), args.uscita.resolve()

    leggio, scheda, piedi, sposta = costruisci(args.pcb, args.piedini, args.inclinazione)
    bb = leggio.bounding_box()
    print(f"Leggio: {bb.size.X:.1f} x {bb.size.Y:.1f} x {bb.size.Z:.1f} mm, "
          f"volume {leggio.volume / 1000:.1f} cm³, solidi {len(leggio.solids())}")
    esporta(leggio, scheda, piedi)
    esporta_glb(leggio, piedi, sposta, args.piedini)
    if args.render:
        render([(leggio, "#e8e6e1")], "leggio-render.png", (0.45, -0.75, 0.75))
        render(parti_colorate(leggio, scheda, piedi), "leggio-assieme-fronte.png", (0.35, -0.8, 0.5))
        render(parti_colorate(leggio, scheda, piedi), "leggio-assieme-retro.png", (-0.5, 0.8, 0.35))
        render(parti_colorate(leggio, scheda, piedi), "leggio-assieme-lato.png", (1, 0, 0))
