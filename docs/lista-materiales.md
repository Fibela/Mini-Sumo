# Lista de materiales — MiniSumo Rev. B

Componentes **realmente comprados**, según cotización PIGRA C920 del 12 de octubre de 2026,
más el ESP32 que ya se tenía. Esta lista sustituye a todas las anteriores.

## Comprados

| # | Componente | Cant. | Para qué sirve |
|---|---|---|---|
| 1 | Micromotorreductor Pololu 6 V 50:1, 590 RPM | 2 | Tracción trasera, uno por rueda |
| 2 | Sensor ToF VL53L1X | 5 | Detectar al oponente, en abanico frontal |
| 3 | Sensor de línea QRE1113 analógico (QTR-1A) | 4 | Detectar el borde blanco del dohyo |
| 4 | Driver dual TB6612FNG, 15 V 1.2 A | 1 | Entregar corriente a los dos motores |
| 5 | Microcontrolador RP2040 (Raspberry Pi Pico) | 1 | Cerebro del robot — ruta A |
| 6 | Sensor de corriente INA219, I²C | 1 | Medir consumo y detectar contacto con el rival |
| 7 | Batería 18650 2S 7.4 V 2550 mAh, armada con conector | 1 | Alimentación |

Subtotal de la cotización: **$1 746.96 MXN** con IVA.

## Ya se tenía

| Componente | Cant. | Nota |
|---|---|---|
| ESP32 DevKit V1 | 1 | Cerebro del robot — ruta B. Sus 8 canales ADC evitan el multiplexor |

## Fabricación propia

| Componente | Cant. | Nota |
|---|---|---|
| Chasis impreso en PETG | 1 | `cad/modelos/MiniSumo_Chasis_RevB.SLDPRT` |
| Cuña de acero de 2 mm | 1 | Corte láser desde `cad/modelos/MiniSumo_Cuna_desarrollo_plano.dxf` |
| Patines de deslizamiento UHMW | 2 | Torneados o impresos |
| Separadores de 0.4 / 0.8 / 1.6 mm | 1 juego | Calibración de la altura de la cuña el Día 1 |
| Ruedas de silicón 35 × 12 mm | 2 | Moldeadas. Durómetro Shore A 20–30 |

## Pendiente de comprar

| Componente | Prioridad | Por qué |
|---|---|---|
| **Convertidor buck a 5 V** (MP1584 o LM2596) | **Crítica** | La batería da 7.4 V y ningún MCU la tolera directo. Sin esto no hay operación autónoma |
| CD4051 multiplexor analógico | Solo con Pico | El Pico tiene 3 canales ADC y hay 4 sensores de línea. Con ESP32 no hace falta |
| Perfboard, headers y conectores JST-XH | Alta | Para el montaje definitivo. Ver `docs/componentes.md` |
| Condensador electrolítico 470 µF | Alta | Entre VM y GND del driver |
| Resistencias 4.7 kΩ | Alta | Pull-ups del bus I²C |
| Lastre (latón o plomo) | Media | Faltan ~98 g para el objetivo de 490 g |

## Opcional

| Componente | Nota |
|---|---|
| Display OLED SSD1306 0.96" I²C | Complemento final, no obligatorio. El diseño reserva su espacio en el bus I²C y en el chasis. Mientras no esté, el menú y la calibración van por consola serial |

## Descartado

| Componente | Por qué |
|---|---|
| Driver L298N | Caída de 2 V, 28 g y 43 × 43 mm. El TB6612 hace lo mismo con 0.3 V de caída, 5 g y 20 × 20 mm |
| Batería LiPo 2S 850 mAh | Sustituida por la 18650 2S de 2550 mAh: más capacidad y más peso útil |
| Arduino Nano | Insuficiente para cinco sensores I²C más el lazo de control a 200 Hz |
| Cuatro motores | La tracción trasera con dos motores y patines evita el problema de un chasis rígido con cuatro ruedas, que en un dojo no perfectamente plano reparte la carga de forma impredecible |
