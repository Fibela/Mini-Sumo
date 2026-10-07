# CAD — MiniSumo Rev. B (tracción trasera)

Modelo del chasis, la cuña y los patines para la categoría MiniSumo del
6.º Foro Nacional de Ingenierías UVM 2026.

## Estructura

| Carpeta | Contenido |
|---|---|
| `modelos/` | Geometría lista para usar: `.SLDPRT` nativo, `.STEP` para importar, `.STL` para el laminador, `.DXF` para corte láser |
| `macros/` | Macros VBA que reconstruyen el chasis dentro de SolidWorks con árbol paramétrico |
| `scripts/` | Modelos en CadQuery (Python), fuente de los STEP y STL |
| `vistas/` | Renders de verificación |

## Piezas

**Chasis** — `MiniSumo_Chasis_RevB.SLDPRT`
Envolvente 100 × 100 × 25 mm. Impreso en PETG con 4 perímetros y 25 % de
relleno pesa unos 58 g. Orientación plana, sin soportes salvo bajo el
bloque frontal de la cuña.

**Cuña** — `MiniSumo_Cuna_acero_2mm.step`
Acero de 2 mm, 55.6 g. Para fabricarla se entrega al taller
`MiniSumo_Cuna_desarrollo_plano.dxf`: 98 × 33.85 mm, línea de doblez a
18.07 mm del borde de la punta, radio interior 2 mm. El canto frontal va
desbarbado y pulido — es requisito de reglamento y además desliza mejor.

**Patines** — `MiniSumo_Patin.step`
Dos piezas en UHMW, nylon o POM. Nunca en acero: el dojo es madera
pintada y se raya. Los separadores de 0.4, 0.8 y 1.6 mm calibran la
altura de la cuña sobre el dojo real el Día 1.

## Reconstruir el chasis en SolidWorks

1. Herramientas → Macro → Nueva, guardar un `.swp` vacío.
2. En el editor VBA: Archivo → Importar archivo → `macros/MiniSumo_Chasis_v2.bas`.
3. Seleccionar el módulo `MiniSumoChasisV2` y pulsar F5.

La macro construye el árbol completo y guarda la pieza. Las cotas quedan
en Herramientas → Ecuaciones.

## Cotas críticas

| Parámetro | Valor |
|---|---|
| Envolvente | 100 × 100 mm |
| Altura de la pieza | 25 mm |
| Eje motriz desde el frente | 72 mm |
| Altura del eje | 17.5 mm |
| Rueda | ø35 × 12 mm |
| Proyección de la cuña | 20 mm a 25° |
| Holgura de la punta | 1.2 mm, ajustable |
| Peso objetivo del robot | 490 g |

## Pendiente

- Ranuras verticales del bloque de la cuña: 4 cortes de 4.2 × 8 mm en el
  plano Alzado, a x = 14, 34, 66 y 86 mm.
- Consulta al responsable de categoría sobre la cuña rígida, el canto
  romo y el microcontrolador con radio integrada.
