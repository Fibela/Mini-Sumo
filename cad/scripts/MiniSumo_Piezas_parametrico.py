"""
MiniSumo UVM 2026 - cuna de acero y patines de deslizamiento.
La cuna se fabrica por corte laser + doblez: se exporta el solido 3D (STEP)
y el desarrollo plano (DXF) que es lo que pide el taller.
"""
import math
import cadquery as cq

OUT = "/mnt/user-data/outputs/"

# ---------------- parametros de la cuna ----------------
ANCHO_C = 98.0        # 1 mm de retiro por lado respecto a los 100 del galibo
ESP     = 2.0         # espesor de la lamina de acero
PROY    = 20.0        # proyeccion horizontal de la rampa
ANG     = 25.0        # angulo de la rampa
BRIDA_H = 14.0        # altura de la brida de montaje
BARR    = 3.4         # barreno pasante para M3
BARR_X  = (-36.0, -16.0, 16.0, 36.0)   # coinciden con las ranuras del chasis

a = math.radians(ANG)
L_RAMPA = PROY / math.cos(a)           # longitud real de la rampa
Z_ALTO  = PROY * math.tan(a)           # altura de la rampa en su extremo
nx, nz  = -math.sin(a), math.cos(a)    # normal a la rampa

# perfil lateral en el plano YZ
p0 = (0.0, 0.0)                                   # punta, cara inferior
p1 = (PROY, Z_ALTO)                               # fin de rampa, cara inferior
p2 = (PROY + ESP * abs(nx), Z_ALTO)               # arranque de la brida
p3 = (p2[0], Z_ALTO + BRIDA_H + ESP * nz)         # tope exterior de la brida
p4 = (p2[0] - ESP, p3[1])                         # tope interior
p5 = (PROY + ESP * nx, Z_ALTO + ESP * nz)         # fin de rampa, cara superior
p6 = (ESP * nx, ESP * nz)                         # punta, cara superior

cuna = (cq.Workplane("YZ")
        .polyline([p0, p1, p2, p3, p4, p5, p6]).close()
        .extrude(ANCHO_C)
        .translate((-ANCHO_C / 2, 0, 0)))

# barrenos pasantes en la brida, cilindros horizontales a lo largo de Y
for bx in BARR_X:
    taladro = (cq.Workplane("XZ").circle(BARR / 2).extrude(-(ESP + 6))
               .translate((bx, PROY + ESP + 3.0, Z_ALTO + BRIDA_H / 2 + 2.0)))
    cuna = cuna.cut(taladro)

cq.exporters.export(cuna, OUT + "MiniSumo_Cuna_acero_2mm.step")
cq.exporters.export(cuna, OUT + "MiniSumo_Cuna_acero_2mm.stl",
                    tolerance=0.01, angularTolerance=0.1)

vol_c = cuna.val().Volume() / 1000.0
print("cuna: volumen cm3", round(vol_c, 2),
      "| masa acero (7.85 g/cm3)", round(vol_c * 7.85, 1), "g")
print("cuna: longitud de rampa", round(L_RAMPA, 2),
      "| altura en el extremo", round(Z_ALTO, 2))

# ---------------- desarrollo plano para corte laser ----------------
# longitud desarrollada = rampa + brida + factor de doblez (K=0.44 para acero)
K = 0.44
R_DOBLEZ = 2.0
bend_allow = math.radians(90 + ANG) * (R_DOBLEZ + K * ESP)
L_DEV = L_RAMPA + BRIDA_H + bend_allow - 2 * (R_DOBLEZ + ESP)

plano = (cq.Workplane("XY")
         .rect(ANCHO_C, L_DEV)
         .extrude(ESP))
for bx in BARR_X:
    plano = plano.cut(
        cq.Workplane("XY").circle(BARR / 2).extrude(ESP + 2)
        .translate((bx, L_DEV / 2 - BRIDA_H / 2 - 1.0, -1))
    )
cq.exporters.export(plano.faces(">Z").wires().toPending(),
                    OUT + "MiniSumo_Cuna_desarrollo_plano.dxf")
cq.exporters.export(plano, OUT + "MiniSumo_Cuna_desarrollo_plano.step")
print("desarrollo plano:", round(ANCHO_C, 1), "x", round(L_DEV, 2), "mm",
      "| linea de doblez a", round(L_RAMPA - R_DOBLEZ - ESP, 2),
      "mm del borde de la punta")

# ---------------- patines de deslizamiento ----------------
PAT_D   = 10.0        # diametro del patin
PAT_H   = 6.0         # altura total
PAT_R   = 2.5         # radio de la corona esferica de contacto
AVELL   = 6.0         # diametro del avellanado para cabeza M3

patin = (cq.Workplane("XY").circle(PAT_D / 2).extrude(PAT_H))
# corona redondeada en la cara de contacto, para no clavarse en el dojo
patin = patin.edges(">Z").fillet(PAT_R)
# barreno M3 pasante con avellanado por la cara plana (lado del chasis)
patin = patin.cut(cq.Workplane("XY").circle(1.7).extrude(PAT_H + 2).translate((0, 0, -1)))
patin = patin.cut(
    cq.Workplane("XY").circle(AVELL / 2).extrude(2.5).translate((0, 0, -0.01))
)
cq.exporters.export(patin, OUT + "MiniSumo_Patin.step")
cq.exporters.export(patin, OUT + "MiniSumo_Patin.stl",
                    tolerance=0.005, angularTolerance=0.05)
vol_p = patin.val().Volume() / 1000.0
print("patin: volumen cm3", round(vol_p, 3),
      "| masa UHMW (0.95 g/cm3)", round(vol_p * 0.95, 2), "g c/u")

# ---------------- separadores de ajuste ----------------
for t in (0.4, 0.8, 1.6):
    sep = (cq.Workplane("XY").circle(PAT_D / 2).extrude(t)
           .cut(cq.Workplane("XY").circle(1.7).extrude(t + 2).translate((0, 0, -1))))
    cq.exporters.export(sep, OUT + f"MiniSumo_Separador_{str(t).replace('.','p')}mm.stl",
                        tolerance=0.005, angularTolerance=0.05)
print("separadores exportados: 0.4, 0.8 y 1.6 mm")
