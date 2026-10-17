# Conexiones: asignación de pines

Dos rutas de microcontrolador para el mismo hardware. El cableado de los módulos entre sí
no cambia; solo cambia a qué pin del MCU llega cada señal.

Para entender qué hace cada componente, ver [componentes.md](componentes.md).

---

## Resumen de las dos rutas

| | Raspberry Pi Pico | ESP32 DevKit V1 |
|---|---|---|
| Lógica | 3.3 V | 3.3 V |
| Entrada de alimentación | `VSYS`, 1.8–5.5 V | `VIN`, 5 V |
| Salida para sensores | `3V3 OUT` | `3V3` |
| Canales ADC utilizables | **3** (GP26–GP28) | **8** en ADC1 (GPIO 32–39) |
| ¿Necesita CD4051? | **Sí**, o unir los traseros en paralelo | **No** |
| Velocidad | 133 MHz, 2 núcleos | 240 MHz, 2 núcleos |
| Radio integrada | No | Sí — pendiente de confirmar con el responsable de categoría |
| Trampa principal | Pocos ADC | ADC2 inservible con wifi; pines de arranque |

---

## Ruta A — Raspberry Pi Pico

### Alimentación

| Pin | Nº físico | Conecta a | Nota |
|---|---|---|---|
| `VSYS` | 39 | Salida del buck, 5 V | **Nunca** 7.4 V aquí |
| `GND` | 38 y otros | Masa común | Todas las masas al mismo punto |
| `3V3 OUT` | 36 | VCC de todos los sensores y del driver | Es salida, no entrada |

### Bus I²C

| Pin | Nº físico | Conecta a |
|---|---|---|
| `GP4` | 6 | `SDA` de los 5 ToF, del INA219 y del OLED |
| `GP5` | 7 | `SCL` de los mismos |

Pull-ups de 4.7 kΩ a 3.3 V, **un solo par** en todo el bus.

| Dispositivo | Dirección |
|---|---|
| VL53L1X #1 a #5 | `0x30` a `0x34`, asignadas en el arranque |
| INA219 | `0x40` |
| OLED SSD1306 *(opcional)* | `0x3C` |

### XSHUT de los sensores de distancia

| Pin | Nº físico | Sensor |
|---|---|---|
| `GP6` | 9 | ToF 1 — frontal izquierdo |
| `GP7` | 10 | ToF 2 — frontal centro-izquierda |
| `GP8` | 11 | ToF 3 — frontal centro |
| `GP9` | 12 | ToF 4 — frontal centro-derecha |
| `GP10` | 14 | ToF 5 — frontal derecho |

### Sensores de línea

Solo hay tres entradas analógicas para cuatro sensores. Dos opciones:

**Opción 1 — con multiplexor CD4051** *(requiere comprarlo)*

| Pin | Nº físico | Conecta a |
|---|---|---|
| `GP11` | 15 | `S0` del CD4051 |
| `GP12` | 16 | `S1` del CD4051 |
| `GP13` | 17 | `S2` del CD4051 |
| `GP26` | 31 | Salida común del CD4051 |

Los cuatro QRE1113 van a los canales CH0–CH3 del multiplexor. El firmware los lee por barrido.

**Opción 2 — sin comprar nada**

| Pin | Nº físico | Conecta a |
|---|---|---|
| `GP26` (ADC0) | 31 | QRE1113 frontal izquierdo |
| `GP27` (ADC1) | 32 | QRE1113 frontal derecho |
| `GP28` (ADC2) | 34 | Los dos QRE1113 traseros, **unidos en paralelo** |

Atrás solo se necesita saber que hay borde, no de qué lado. Adelante sí hace falta distinguir,
y por eso los frontales nunca se unen.

Con esta opción, el monitor de batería se pierde o se mide con el INA219.

### Driver y control

| Pin | Nº físico | Conecta a |
|---|---|---|
| `GP16` | 21 | `PWMA` — velocidad motor izquierdo |
| `GP17` | 22 | `AIN1` — dirección motor izquierdo |
| `GP18` | 24 | `AIN2` — dirección motor izquierdo |
| `GP19` | 25 | `PWMB` — velocidad motor derecho |
| `GP20` | 26 | `BIN1` — dirección motor derecho |
| `GP21` | 27 | `BIN2` — dirección motor derecho |
| `GP22` | 29 | `STBY` — habilita el driver |
| `GP15` | 20 | Botón de arranque, a masa, con pull-up interno |
| `GP25` | — | LED interno de la placa, para estado |

---

## Ruta B — ESP32 DevKit V1

### Alimentación

| Pin | Conecta a | Nota |
|---|---|---|
| `VIN` | Salida del buck, 5 V | También llamado `5V` en algunas placas |
| `GND` | Masa común | |
| `3V3` | VCC de todos los sensores y del driver | Es salida |

### Bus I²C

| Pin | Conecta a |
|---|---|
| `GPIO 21` | `SDA` de los 5 ToF, del INA219 y del OLED |
| `GPIO 22` | `SCL` de los mismos |

Mismas direcciones que en la ruta A.

### XSHUT de los sensores de distancia

| Pin | Sensor |
|---|---|
| `GPIO 4` | ToF 1 — frontal izquierdo |
| `GPIO 16` | ToF 2 — frontal centro-izquierda |
| `GPIO 17` | ToF 3 — frontal centro |
| `GPIO 23` | ToF 4 — frontal centro-derecha |
| `GPIO 13` | ToF 5 — frontal derecho |

### Sensores de línea — los cuatro directo

Esta es la ventaja de la ruta B: no hace falta multiplexor.

| Pin | ADC | Conecta a |
|---|---|---|
| `GPIO 32` | ADC1_CH4 | QRE1113 frontal izquierdo |
| `GPIO 33` | ADC1_CH5 | QRE1113 frontal derecho |
| `GPIO 34` | ADC1_CH6 | QRE1113 trasero izquierdo |
| `GPIO 35` | ADC1_CH7 | QRE1113 trasero derecho |
| `GPIO 36` | ADC1_CH0 | Divisor de batería, 100 kΩ / 47 kΩ |

**GPIO 34 a 39 son solo de entrada** — no tienen resistencias internas ni pueden ser salidas.
Para leer sensores es exactamente lo que se necesita.

**No usar ADC2 (GPIO 0, 2, 4, 12–15, 25–27) para nada analógico.** Deja de funcionar cuando
el wifi está activo, y el fallo es intermitente y difícil de diagnosticar.

### Driver y control

| Pin | Conecta a | Nota |
|---|---|---|
| `GPIO 25` | `PWMA` — velocidad motor izquierdo | Canal LEDC |
| `GPIO 26` | `AIN1` — dirección motor izquierdo | |
| `GPIO 27` | `AIN2` — dirección motor izquierdo | |
| `GPIO 14` | `PWMB` — velocidad motor derecho | Canal LEDC |
| `GPIO 18` | `BIN1` — dirección motor derecho | |
| `GPIO 19` | `BIN2` — dirección motor derecho | |
| `GPIO 5` | `STBY` — habilita el driver | Pin de arranque: queda en bajo al encender, lo cual deja el driver dormido. Es el comportamiento deseado |
| `GPIO 0` | Botón **BOOT** de la placa | Sirve como botón de arranque. **No presionarlo al encender** o entra en modo programación |
| `GPIO 2` | LED azul de la placa | Pin de arranque, pero como salida de LED no estorba |

### Pines del ESP32 que conviene no tocar

| Pin | Por qué |
|---|---|
| `GPIO 6` a `GPIO 11` | Conectados a la memoria flash interna. Usarlos cuelga el chip |
| `GPIO 12` | Si queda en alto al encender, el ESP32 no arranca |
| `GPIO 1` y `GPIO 3` | TX y RX del puerto serie. Se pierde la consola USB |

---

## Conexiones que no dependen del microcontrolador

Estas son iguales en ambas rutas.

### Cadena de potencia

```
Batería (+) ──→ Switch ──→ INA219 VIN+
                             INA219 VIN- ──→ VM del TB6612
                                         └──→ Buck IN+ ──→ 5 V ──→ MCU

Batería (−) ──→ Masa común ←── todas las masas
```

El INA219 va **en serie** en la línea de potencia: la corriente de los motores pasa a través
de él. Es la única forma de medirla.

### Driver a motores

| Salida del TB6612 | Conecta a |
|---|---|
| `AO1` y `AO2` | Los dos cables del motor izquierdo |
| `BO1` y `BO2` | Los dos cables del motor derecho |

Si una rueda gira al revés, se invierten `AIN1`/`AIN2` **en el firmware**, no el cableado.

### Alimentación de los sensores

Todos los sensores —los cinco ToF, los cuatro QRE1113, el INA219 y el OLED— se alimentan de
los **3.3 V que genera el microcontrolador**, nunca de los 5 V del buck ni de la batería.

---

## Verificación antes de energizar

Con el multímetro, **sin batería conectada**:

- Continuidad entre todas las masas: batería, driver, MCU, sensores. Deben ser el mismo punto.
- Ausencia de corto entre 3.3 V y masa, y entre 5 V y masa.
- Cada pin del header contra su destino. Un puente de soldadura entre `SDA` y `SCL` deja el
  bus muerto y el síntoma parece de software.

Con batería, **antes de conectar el microcontrolador**:

- Salida del buck ajustada a 5.0 V, medida en vacío y confirmada dos veces.
- Positivo de la batería llegando a `VM` del driver.

La primera vez que giran los motores:

- Ruedas en el aire, siempre. PWM al 30 %.
- Si el MCU se reinicia al arrancar los motores: falta el condensador de 470 µF, o la masa
  está mal hecha.
