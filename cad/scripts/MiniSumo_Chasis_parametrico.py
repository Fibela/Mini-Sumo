"""
MiniSumo UVM 2026 - chasis parametrico, traccion trasera (Rev. B)
Genera STEP (para SolidWorks) y STL (para laminador).
Origen: esquina frontal izquierda. Z=0 es la cara inferior de la pieza,
que queda a CLR mm del dojo.
"""
import cadquery as cq

# ---------------- parametros globales ----------------
ANCHO   = 100.0      # envolvente X
PROF    = 100.0      # envolvente Y
CLR     = 4.0        # separacion de la cara inferior al dojo
BASE_T  = 3.0        # espesor de la placa base
PARED   = 2.4        # espesor de pared (6 perimetros a 0.4 mm)
ALT_P   = 24.0       # altura de la pieza

EJE_Y   = 72.0       # eje motriz desde el frente
EJE_Z   = 17.5 - CLR # altura del eje en coords de la pieza
RUEDA_D = 35.0
RUEDA_W = 12.0
HOLG_R  = 1.0        # holgura lateral de rueda

MOT_W   = 10.0       # ancho del cuerpo N20
MOT_H   = 12.0       # alto del cuerpo N20
MOT_L   = 26.0       # largo del cuerpo N20
AJUSTE  = 0.4        # holgura de impresion en cavidades

BAT_X, BAT_Y, BAT_Z = 55.0, 30.0, 13.0   # bateria LiPo 2S
CUNA_Y  = 6.0        # espesor del bloque de montaje de la cuna
TOR_H   = 22.0       # altura de la torre de sensores

INS_M3  = 4.2        # agujero para inserto termico M3
INS_M2  = 3.2        # agujero para inserto termico M2

# ---------------- cuerpo base ----------------
ch = cq.Workplane("XY").box(ANCHO, PROF, ALT_P, centered=(False, False, False))

# vaciado interior general, dejando la placa base
ch = ch.cut(
    cq.Workplane("XY")
    .box(ANCHO - 2 * PARED, PROF - 2 * PARED, ALT_P - BASE_T + 1,
         centered=(False, False, False))
    .translate((PARED, PARED, BASE_T))
)

# ---------------- recortes de rueda ----------------
rec_w = RUEDA_W + 2 * HOLG_R          # 14 mm desde cada costado
rec_l = RUEDA_D + 3.0                 # holgura longitudinal
for x0 in (0.0, ANCHO - rec_w):
    ch = ch.cut(
        cq.Workplane("XY")
        .box(rec_w, rec_l, ALT_P + 2, centered=(False, False, False))
        .translate((x0, EJE_Y - rec_l / 2, -1))
    )

# ---------------- alojamientos de motor ----------------
# los motores apuntan hacia afuera desde el centro, eje en Y = EJE_Y
for lado in (0, 1):
    if lado == 0:
        x0 = rec_w                      # arranca donde termina el recorte
    else:
        x0 = ANCHO - rec_w - MOT_L
    ch = ch.cut(
        cq.Workplane("XY")
        .box(MOT_L + AJUSTE, MOT_W + AJUSTE, MOT_H + AJUSTE,
             centered=(False, False, False))
        .translate((x0, EJE_Y - (MOT_W + AJUSTE) / 2,
                    EJE_Z - (MOT_H + AJUSTE) / 2))
    )

# bridas de sujecion de motor: agujeros pasantes M2 arriba de cada motor
for x in (rec_w + MOT_L / 2, ANCHO - rec_w - MOT_L / 2):
    for dy in (-9.0, 9.0):
        ch = ch.cut(
            cq.Workplane("XY").circle(INS_M2 / 2)
            .extrude(ALT_P + 2).translate((x, EJE_Y + dy, -1))
        )

# ---------------- bolsillo de bateria ----------------
bat_y0 = 26.0
ch = ch.cut(
    cq.Workplane("XY")
    .box(BAT_X + 1.0, BAT_Y + 1.0, BAT_Z + 2,
         centered=(False, False, False))
    .translate(((ANCHO - BAT_X - 1.0) / 2, bat_y0, BASE_T))
)
# paso de cable de la bateria hacia el frente
ch = ch.cut(
    cq.Workplane("XY").box(10.0, 12.0, 8.0, centered=(False, False, False))
    .translate((ANCHO / 2 - 5, bat_y0 - 12.0, BASE_T + 2))
)

# ---------------- bloque frontal para la cuna ----------------
cuna_blk = (cq.Workplane("XY")
            .box(ANCHO - 2 * PARED, CUNA_Y, 10.0, centered=(False, False, False))
            .translate((PARED, PARED, BASE_T)))
ch = ch.union(cuna_blk)
# ranuras verticales para ajustar la altura de la cuna (M3, ranura de 4 mm)
for x in (14.0, 34.0, 66.0, 86.0):
    ranura = (cq.Workplane("XZ")
              .slot2D(8.0, INS_M3, 90)
              .extrude(CUNA_Y + 4)
              .translate((x, PARED - 2, BASE_T + 6.0)))
    ch = ch.cut(ranura)

# ---------------- nervios longitudinales de rigidez ----------------
# unen el frente con la parte trasera; los recortes de rueda cortan las
# paredes laterales, asi que la rigidez a flexion viene de aqui
for nx in (17.0, ANCHO - 17.0 - PARED):
    ch = ch.union(
        cq.Workplane("XY")
        .box(PARED, PROF - 2 * PARED, 9.0, centered=(False, False, False))
        .translate((nx, PARED, BASE_T))
    )

# ---------------- torre de sensores ToF ----------------
TOR_T = 4.0
tor = (cq.Workplane("XY")
       .box(44.0, TOR_T, TOR_H, centered=(False, False, False))
       .translate((ANCHO / 2 - 22.0, PARED + CUNA_Y, BASE_T)))
ch = ch.union(tor)
# escuadras de refuerzo: la torre trabaja a flexion contra el rival
for sx in (ANCHO / 2 - 22.0, ANCHO / 2 + 18.0):
    esc = (cq.Workplane("XZ")
           .polyline([(0, 0), (14.0, 0), (0, TOR_H - 2.0)]).close()
           .extrude(PARED)
           .translate((sx, PARED + CUNA_Y + TOR_T, BASE_T)))
    ch = ch.union(esc)
# ventanas para los cinco VL53L1X (12 x 8 mm)
for i, x in enumerate((-18.0, -9.0, 0.0, 9.0, 18.0)):
    ch = ch.cut(
        cq.Workplane("XY")
        .box(6.0, PARED + 3.0, 8.0, centered=(False, False, False))
        .translate((ANCHO / 2 + x - 3.0, PARED + CUNA_Y - 1.0, BASE_T + 8.0))
    )

# ---------------- ventanas de sensores de linea ----------------
# dos adelantados y dos traseros, pasantes en la placa base
for (sx, sy) in ((26.0, 24.0), (74.0, 24.0), (26.0, 94.0), (74.0, 94.0)):
    ch = ch.cut(
        cq.Workplane("XY").circle(5.0).extrude(BASE_T + 2)
        .translate((sx, sy, -1))
    )

# ---------------- postes para la placa de control ----------------
POSTE_Z = 18.0
for (px, py) in ((18.0, 26.0), (82.0, 26.0), (18.0, 58.0), (82.0, 58.0)):
    poste = (cq.Workplane("XY").circle(4.0).extrude(POSTE_Z)
             .translate((px, py, BASE_T)))
    ch = ch.union(poste)
    ch = ch.cut(
        cq.Workplane("XY").circle(INS_M2 / 2).extrude(8.0)
        .translate((px, py, BASE_T + POSTE_Z - 7.0))
    )

# ---------------- anclajes de lastre sobre el eje ----------------
for (lx, ly) in ((40.0, 62.0), (60.0, 62.0), (40.0, 82.0), (60.0, 82.0)):
    ch = ch.cut(
        cq.Workplane("XY").circle(INS_M3 / 2).extrude(9.0)
        .translate((lx, ly, BASE_T - 1.0))
    )

# ---------------- patines de deslizamiento delanteros ----------------
for px in (12.0, ANCHO - 12.0):
    ch = ch.cut(
        cq.Workplane("XY").circle(INS_M3 / 2).extrude(9.0)
        .translate((px, 16.0, BASE_T - 1.0))
    )

# ---------------- redondeo de las esquinas exteriores ----------------
# solo las cuatro verticales del perimetro, para no tocar cavidades internas
try:
    ch = (ch.edges("|Z")
            .edges(cq.selectors.BoxSelector((-1, -1, -1), (6, 6, ALT_P + 1)))
            .fillet(1.5))
except Exception:
    pass

# ---------------- exportar ----------------
cq.exporters.export(ch, "/mnt/user-data/outputs/MiniSumo_Chasis.step")
cq.exporters.export(ch, "/mnt/user-data/outputs/MiniSumo_Chasis.stl",
                    tolerance=0.01, angularTolerance=0.1)

vol = ch.val().Volume() / 1000.0          # cm3
print("volumen cm3:", round(vol, 2))
print("masa PETG (1.27 g/cm3):", round(vol * 1.27, 1), "g")
bb = ch.val().BoundingBox()
print("envolvente:", round(bb.xlen, 2), round(bb.ylen, 2), round(bb.zlen, 2))
