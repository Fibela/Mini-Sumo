# Mini Sumo — UVM 2026

Robot autónomo de combate para la categoría **MiniSumo** del 6.º Foro Nacional de
Ingenierías UVM 2026, Campus Cuernavaca, 3 y 4 de diciembre.

Diseño de tracción trasera con cuña frontal de acero. Envolvente de 100 × 100 mm,
500 g máximo.

---

## Por dónde empezar

| Si quieres… | Ve a |
|---|---|
| Entender qué hace cada componente y cómo se conecta | [`docs/componentes.md`](docs/componentes.md) |
| Cablear el robot | [`docs/conexiones.md`](docs/conexiones.md) |
| Saber qué comprar todavía | [`docs/lista-materiales.md`](docs/lista-materiales.md) |
| Armar la electrónica paso a paso | [`docs/pdf/Guia_de_montaje_electronico.pdf`](docs/pdf/Guia_de_montaje_electronico.pdf) |
| Imprimir el chasis | [`cad/README.md`](cad/README.md) |
| Cargar el primer firmware de prueba | [`firmware/pruebas/`](firmware/pruebas/) |

---

## Estructura del repositorio

```
docs/              Documentación del proyecto
  componentes.md       Qué es cada componente, para qué sirve, cómo se conecta
  conexiones.md        Asignación de pines para Pico y para ESP32
  conectores-switches.md   Interruptor, botón y conectores: qué comprar y por qué
  lista-materiales.md  Comprado, pendiente y descartado
  pdf/                 Planos, memoria de diseño, diagramas y guías
  imagenes/            Diagramas de montaje

cad/               Diseño mecánico
  modelos/             SLDPRT, STEP, STL y DXF de las piezas
  impresion/           Archivos laminados listos para imprimir
  macros/              Macros VBA que reconstruyen el chasis en SolidWorks
  scripts/             Modelos paramétricos en CadQuery (Python)
  vistas/              Renders de verificación

firmware/          Código
  MiniSumo_ESP32/      Firmware de combate (en migración al hardware nuevo)
  pruebas/             Sketches de prueba, uno por subsistema

_revisar/          Versiones intermedias pendientes de depurar
```

---

## Estado del proyecto

### Hardware

| Subsistema | Estado |
|---|---|
| Chasis | Modelado y listo para imprimir. Falta el alojamiento de la batería 18650 |
| Cuña de acero | DXF listo para corte láser |
| Electrónica | Componentes comprados. **Falta el convertidor buck a 5 V** |
| Cableado | Pendiente. Ver `docs/conectores-switches.md` |

### Firmware

| Prueba | Estado |
|---|---|
| 01 — Sensores de distancia ToF | Escrita, sin probar en hardware |
| 02 — Sensores de línea | Pendiente |
| 03 — Motores y driver | Pendiente |
| 04 — Integración y máquina de estados | Pendiente |

---

## Hardware

Dos rutas de microcontrolador, mismo hardware alrededor:

| | Raspberry Pi Pico | ESP32 DevKit V1 |
|---|---|---|
| Canales ADC utilizables | 3 | 8 |
| ¿Necesita multiplexor CD4051? | Sí | No |
| Radio integrada | No | Sí — pendiente de confirmar con el responsable de categoría |

Común a las dos:

- 2 micromotorreductores Pololu 50:1, 590 RPM — tracción trasera
- Driver TB6612FNG
- 5 sensores de distancia VL53L1X en abanico frontal
- 4 sensores de línea QRE1113 analógicos
- Sensor de corriente INA219
- Batería 18650 2S, 7.4 V, 2550 mAh
- Display OLED SSD1306 — opcional

---

## Fechas

| Qué | Cuándo |
|---|---|
| Cierre de inscripciones | **13 de noviembre de 2026** |
| Publicación de listas y horarios | 21 de noviembre de 2026 |
| Verificación técnica y eliminatorias | 3 de diciembre de 2026 |
| Semifinales y final | 4 de diciembre de 2026 |

Inscripción: https://bit.ly/foroing2026

---

## Pendientes abiertos

1. **Comprar el convertidor buck a 5 V.** Es el único bloqueo para operar con batería.
2. **Consulta al responsable de categoría** (Miguel Ángel Alcántara,
   miguel.alcantara@uvmnet.edu): si se admite una cuña rígida integrada al chasis, si el
   canto romo de 2 mm cumple la restricción de partes que dañen al oponente, a partir de
   cuánto tiempo sin contacto se considera «movimiento sin contacto», y si se permite un
   microcontrolador con radio integrada.
3. **Rediseñar el alojamiento de batería:** las celdas 18650 no caben en el bolsillo actual.
4. **Inscripción**, antes del 13 de noviembre.
