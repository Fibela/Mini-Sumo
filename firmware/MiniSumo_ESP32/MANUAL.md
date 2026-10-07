# Manual del Mini Sumo – ESP32

Manual de uso, conceptos y registros del firmware `MiniSumo_ESP32`.
Para cargar el código, ve al [README](README.md).

**Hardware (planos Rev B, tracción trasera):** ESP32 DevKit V1 · puente H L298N · 2 motores N20 en el eje trasero (uno por lado) · patines de deslizamiento al frente · 4 sensores de línea (2 delanteros, 2 traseros) · 1 sensor de oponente al frente · OLED 0.96" SSD1306 I2C · LiPo 2S 850 mAh · divisor de batería 10 kΩ / 4.7 kΩ.

**Sensores intercambiables:** el código no depende de un modelo concreto. En `config.h` se elige el sensor de oponente (Sharp GP2Y0A21, digital, VL53L0X o VL53L1X) y el tipo de sensor de línea (analógico o digital). Así el robot se adapta a lo que haya disponible el día de la competencia.

---

## Contenido

1. [Conceptos](#1-conceptos)
2. [Registro de pines](#2-registro-de-pines)
3. [Registro de recursos del ESP32](#3-registro-de-recursos-del-esp32)
4. [Uso del robot](#4-uso-del-robot)
5. [Opciones del menú](#5-opciones-del-menú)
6. [Secuencia de pruebas recomendada](#6-secuencia-de-pruebas-recomendada)
7. [Lógica de combate](#7-lógica-de-combate)
8. [Parámetros ajustables (config.h)](#8-parámetros-ajustables-configh)
9. [Solución de problemas](#9-solución-de-problemas)
10. [Bitácora de pruebas](#10-bitácora-de-pruebas)

---

## 1. Conceptos

| Concepto | Qué es | Dónde aparece en el robot |
|---|---|---|
| **Dohyo** | La pista circular negra con un borde blanco. Sale perdiendo el robot que la abandona. | Los sensores de línea buscan ese borde blanco. |
| **Puente H** | Circuito que deja invertir el sentido de giro de un motor. El ESP32 no puede alimentar motores directamente. | L298N: un canal por lado del robot. |
| **IN1 / IN2** | Pines de dirección del puente H. `HIGH/LOW` = adelante, `LOW/HIGH` = atrás, `HIGH/HIGH` = freno. | Motores izquierdos: GPIO 26/27; derechos: 18/19. |
| **PWM** | Encender y apagar muy rápido para regular la potencia. El *duty* (0–255) es el porcentaje del tiempo encendido. | Pines ENA/ENB del L298N, a 1 kHz y 8 bits. |
| **Freno activo** | Las dos terminales del motor en cortocircuito: el motor se detiene de golpe en vez de girar libre. | `motor(lado, 0)` frena; no deja rodar. |
| **ADC** | Convertidor analógico-digital. Convierte un voltaje de 0–3.3 V en un número de 0 a 4095. | Sensores de línea, sensor de oponente (Sharp) y batería. |
| **ADC1 / ADC2** | El ESP32 tiene dos ADC. ADC2 deja de funcionar cuando se usa WiFi y da problemas, así que **todo lo analógico va en ADC1** (GPIO 32–39). | Ver [registro de pines](#2-registro-de-pines). |
| **Umbral** | Valor que separa "negro" de "blanco" (o "oponente" de "nada"). | Línea: uno por sensor, calibrado. Oponente: `UMBRAL_IR` (Sharp) o `DISTANCIA_ATAQUE_MM` (ToF). |
| **Calibración** | Medir el negro y el blanco del dohyo real y poner el umbral a la mitad. La luz y la altura cambian las lecturas. | Menú → *Calibrar linea*. |
| **QRE1113** | Sensor infrarrojo de reflexión. **Más reflejo (blanco) = voltaje más bajo.** | `BLANCO_ES_BAJO true`. |
| **Sharp GP2Y0A21** | Sensor de distancia por infrarrojo (10–80 cm). **Más cerca = voltaje más alto.** | Lectura mayor que `UMBRAL_IR` = oponente. |
| **ToF (Time of Flight)** | Sensor láser infrarrojo que mide la distancia por el tiempo que tarda la luz en ir y volver. Responde en milímetros y le afecta poco el color del objeto. | VL53L0X (hasta ~1.2 m) o VL53L1X (más alcance y más inmune a la luz). Bus I2C, dirección 0x29. |
| **Interferencia del rival** | El rival también emite infrarrojo (su ToF, su Sharp, sus sensores de línea). Esos destellos pueden hacer que nuestros sensores vean un "oponente" o un "borde" que no existe. | Se filtra con la confirmación de lecturas. |
| **Confirmación (filtro)** | Una lectura solo se cree si se repite: la línea debe verse blanca 2 veces seguidas y el oponente durante 25 ms sin interrupción. | `CONFIRMA_LINEA`, `T_CONFIRMA_OPONENTE_MS`. |
| **Memoria del oponente** | Al perder al rival, el robot sigue empujando un momento. Sirve contra rivales negros o mate que el sensor ve a ratos. | `T_MEMORIA_OPONENTE_MS` = 200 ms. |
| **Tracción trasera** | Las ruedas motrices están atrás (eje a 72 mm del frente) y el frente se apoya en patines. Al girar sobre su eje, el robot pivota sobre las ruedas traseras y la cuña barre un arco amplio. | Los tiempos de giro y de escape se ajustan con pruebas. |
| **Divisor de voltaje** | Dos resistencias que reducen un voltaje. La LiPo da hasta 8.4 V y el ESP32 solo aguanta 3.3 V en sus pines. | 10 kΩ + 4.7 kΩ → GPIO 33. |
| **LiPo 2S** | Batería de litio de 2 celdas en serie: 8.4 V cargada, 7.4 V nominal, ~6.8 V vacía. Por debajo de 3.0 V por celda se daña. | Indicador en la esquina de la pantalla. |
| **I2C** | Bus de 2 cables (SDA datos, SCL reloj) para conectar varios dispositivos. Cada uno tiene una dirección. | GPIO 21/22. OLED en 0x3C y ToF en 0x29, en el mismo bus. |
| **Máquina de estados** | El robot siempre está en un único "estado" (buscar, atacar, escapar…) y cambia de estado según los sensores. | Función `pelea()`, ver [sección 7](#7-lógica-de-combate). |
| **Pines de arranque (strapping)** | Pines que el ESP32 lee al encender para decidir cómo arrancar. Si se conectan mal, el ESP32 no arranca o entra en modo de programación. | GPIO 0, 2, 5, 12, 15. Solo se usan el 0 (BOOT) y el 2 (LED). |
| **NVS (Preferences)** | Memoria interna que no se borra al apagar. | Guarda los umbrales de línea calibrados. |
| **Rebote (debounce)** | Al presionar un botón, el contacto vibra unos milisegundos. Se ignoran las lecturas de los primeros 20 ms. | `leerBoton()`. |

---

## 2. Registro de pines

### Pines en uso

| GPIO | Función en el código | Dirección | Tipo | Conecta a | Notas |
|---|---|---|---|---|---|
| 0 | `PIN_BOTON` | Entrada (pull-up) | Digital | Botón **BOOT** de la placa | Pin de arranque: **no presionarlo al encender**. |
| 2 | `PIN_LED` | Salida | Digital | LED azul de la placa | Pin de arranque; como LED no causa problemas. |
| 13 | `PIN_DER_PWM` | Salida | PWM (LEDC) | L298N **ENB** | Quitar el jumper de ENB. |
| 18 | `PIN_DER_IN1` | Salida | Digital | L298N **IN3** | |
| 19 | `PIN_DER_IN2` | Salida | Digital | L298N **IN4** | |
| 21 | `PIN_SDA` | Bidireccional | I2C | OLED **SDA** y ToF **SDA** | Bus compartido. |
| 22 | `PIN_SCL` | Salida | I2C | OLED **SCL** y ToF **SCL** | Bus compartido. |
| 25 | `PIN_IZQ_PWM` | Salida | PWM (LEDC) | L298N **ENA** | Quitar el jumper de ENA. |
| 26 | `PIN_IZQ_IN1` | Salida | Digital | L298N **IN1** | |
| 27 | `PIN_IZQ_IN2` | Salida | Digital | L298N **IN2** | |
| 32 | `PIN_IR_OPONENTE` | Entrada | Analógica ADC1 o digital | Sharp **Vo** (cable amarillo) o salida del sensor digital | Sharp: salida máx. ~3.1 V, sin divisor. Digital de 5 V: con divisor. **Libre si se usa ToF.** |
| 33 | `PIN_BATERIA` | Entrada | Analógica ADC1 | Punto medio del divisor 10k/4.7k | Máx. 2.7 V con la batería llena. |
| 34 | `PIN_LINEA_TRA_IZQ` | Entrada | Analógica ADC1 o digital | Sensor de línea trasero izquierdo **OUT** | Solo entrada, sin pull-up. |
| 35 | `PIN_LINEA_TRA_DER` | Entrada | Analógica ADC1 o digital | Sensor de línea trasero derecho **OUT** | Solo entrada, sin pull-up. |
| 36 (VP) | `PIN_LINEA_DEL_IZQ` | Entrada | Analógica ADC1 o digital | Sensor de línea delantero izquierdo **OUT** | Solo entrada. En la placa dice **VP**. |
| 39 (VN) | `PIN_LINEA_DEL_DER` | Entrada | Analógica ADC1 o digital | Sensor de línea delantero derecho **OUT** | Solo entrada. En la placa dice **VN**. |
| 23 | `PIN_STBY` | — | — | Sin uso con L298N | Reservado por si cambian a TB6612. |

### Alimentación

| De | A | Notas |
|---|---|---|
| LiPo 2S **+** | L298N **12V** | Con el jumper "5V-EN" del L298N puesto. |
| LiPo 2S **+** | Regulador step-down 5 V → ESP32 **VIN** | No alimentar el ESP32 desde el pin 5V del L298N con los motores frenando: el voltaje cae y el ESP32 se reinicia. |
| LiPo 2S **+** | Resistencia 10 kΩ → GPIO 33 → 4.7 kΩ → GND | Divisor de batería. |
| ESP32 **3V3** | Sensores de línea (x4) VCC, OLED VCC, ToF VIN | |
| ESP32 **VIN (5 V)** | Sharp VCC (cable rojo), si se usa | El Sharp necesita 5 V. |
| **GND** | Todos los GND (LiPo, L298N, ESP32, sensores, OLED) | **Una sola tierra común.** Sin ella nada funciona bien. |
| L298N **OUT1/OUT2** | Motor N20 izquierdo | Si gira al revés, usa `INVERTIR_IZQ` o invierte sus cables. |
| L298N **OUT3/OUT4** | Motor N20 derecho | Si se usan 2 motores por lado, van en paralelo en el mismo canal. |

### Pines libres para ampliaciones

| GPIO | Uso posible | Restricción |
|---|---|---|
| 4, 16, 17 | Botones, LEDs, sensores digitales | Ninguna. |
| 14 | Salida digital | Emite pulsos al arrancar: no conectar nada que se mueva. |
| 5, 15 | Salida digital | Pines de arranque: sin cargas que los fuercen al encender. |
| 12 | Evitar | Pin de arranque: si está en HIGH al encender, el ESP32 no arranca. |
| 6–11 | **No usar** | Conectados a la memoria flash interna. |
| 1, 3 | **No usar** | TX/RX del puerto USB (Monitor Serie). |

**ADC1 está completo** con Sharp: 32, 33, 34, 35, 36 y 39 están ocupados (con ToF queda libre el 32). Un sensor analógico más tendría que ir en ADC2 (GPIO 4, 13, 14, 15, 25, 26, 27). Eso es posible porque no usamos WiFi, pero implicaría mover alguna salida.

---

## 3. Registro de recursos del ESP32

| Recurso | Uso | Configuración |
|---|---|---|
| **LEDC canal 0** | PWM de los motores izquierdos (GPIO 25) | 1 kHz, 8 bits (0–255) |
| **LEDC canal 1** | PWM de los motores derechos (GPIO 13) | 1 kHz, 8 bits (0–255) |
| **ADC1** | 4 sensores de línea (analógicos), Sharp y batería | 12 bits (0–4095), atenuación por defecto (0–3.3 V) |
| **I2C (Wire)** | OLED SSD1306 y ToF (si se usa) | 400 kHz · OLED 0x3C · ToF 0x29 |
| **UART0 (Serial)** | Monitor Serie, opcional | 115200 baudios |
| **NVS** espacio `"sumo"` | Umbrales calibrados | Claves `u0` (DI), `u1` (DD), `u2` (TI), `u3` (TD) como `int` |

### Tiempos internos

| Qué | Valor | Dónde |
|---|---|---|
| Promedio de cada lectura de línea | 4 muestras | `leerLineaCruda()` |
| Promedio en calibración | 20 lecturas × 5 ms ≈ 100 ms por sensor | `promedioLinea()` |
| Lectura de batería | 16 muestras, como máximo cada 500 ms, con filtro 70/30 | `voltajeBateria()` |
| Rebote del botón | 20 ms | `leerBoton()` |
| Pulsación larga | > 600 ms | `T_PULSO_LARGO_MS` |
| Ciclo del bucle de pelea | ~2 ms + lecturas | `pelea()` |
| Medición del ToF | Cada 20 ms, en modo continuo; el bucle lee sin esperar | `actualizarToF()` |
| Confirmación de línea | 2 lecturas seguidas (≈ 3–4 ms) | `CONFIRMA_LINEA` |
| Confirmación / memoria del oponente | 25 ms / 200 ms | `T_CONFIRMA_OPONENTE_MS` / `T_MEMORIA_OPONENTE_MS` |
| Refresco de pantalla en la prueba en conjunto | 150 ms (cada refresco bloquea ~25 ms) | `REFRESCO_PANTALLA_MS` |
| Refresco del menú (voltaje y parpadeo) | 400 ms | `loop()` |

---

## 4. Uso del robot

### El botón BOOT es el único control

| Acción | Resultado |
|---|---|
| **Pulsación corta** (< 0.6 s) | Pasa a la siguiente opción del menú (al llegar al final vuelve al inicio). |
| **Pulsación larga** (> 0.6 s) | Entra a la opción. La pantalla se invierte y el LED azul se enciende para indicar que ya cuenta como larga: **suelta el botón**. |
| **Cualquier pulsación dentro de una prueba** | Detiene la prueba y frena los motores. |

### Partes de la pantalla

```
┌────────────────────────────────┐
│ MINI SUMO          7.8V [███ ] │  ← título · voltaje · icono de batería
├────────────────────────────────┤
│ ╭────────────────────────────╮ █│  ← opción seleccionada (barra blanca)
│ │ COMBATE                    │ ┊│
│ ╰────────────────────────────╯ ┊│  ← barra de desplazamiento
│   Prueba en conjunto           ┊│
│   Motor izquierdo              ┊│
│· · · · · · · · · · · · · · · · │
│     corto: sig  largo: OK      │  ← ayuda de controles
└────────────────────────────────┘
```

- **Voltaje parpadeando:** batería por debajo de 7.0 V. Carga la LiPo.
- **"USB" en la esquina:** no hay lectura de batería (alimentado solo por USB o divisor desconectado).

### Encendido

1. Coloca el robot y enciéndelo **sin presionar BOOT**.
2. Aparece "MINI SUMO / ESP32" durante 1 s y luego el menú, con **COMBATE** ya seleccionado.

### En una competencia

1. Enciende el robot: el menú queda en **COMBATE**.
2. Cuando el juez lo indique, haz una **pulsación larga** y suelta.
3. La pantalla cuenta **5, 4, 3, 2, 1** (el LED parpadea). Aléjate del robot.
4. Aparece **"A PELEAR!"** y el robot empieza.
5. Para detenerlo, presiona BOOT. La pantalla muestra cuánto duró la pelea; otra pulsación regresa al menú.

### Con computadora (opcional)

Monitor Serie a 115200 baudios. Al enviar un número se ejecuta esa opción: `1` = COMBATE … `9` = Batería, y `0` frena los motores. Cada prueba imprime ahí sus lecturas con más detalle.

---

## 5. Opciones del menú

| # | Opción | Qué hace | Pantalla | Cómo termina |
|---|---|---|---|---|
| 1 | **COMBATE** | Espera 5 s y pelea a velocidad completa. | Cuenta regresiva y luego "A PELEAR!" (no se actualiza durante la pelea). | Una pulsación. |
| 2 | **Prueba en conjunto** | La misma lógica que COMBATE, al **50 %** de velocidad y con 3 s de espera. | El robot con sus sensores, el estado actual y el tiempo, en vivo. | Una pulsación. |
| 3 | **Motor izquierdo** | 2 s de aviso y luego: +120, +255, freno, −120, −255, freno (1.5 s cada uno). | Velocidad grande y ADELANTE / ATRAS / FRENO. | Termina sola o con una pulsación. |
| 4 | **Motor derecho** | Igual que la anterior, para el lado derecho. | Igual. | Igual. |
| 5 | **Movimientos** | Adelante, atrás, giro izq., giro der., arco izq., arco der. (1.2 s cada uno). | Nombre del movimiento. | Termina sola o con una pulsación. |
| 6 | **Sensores de línea** | Lectura en vivo de los 4 sensores (digitales: 0 o 4095). | El robot visto desde arriba: círculo **relleno = ve blanco**. Valores en las esquinas. | Una pulsación. |
| 7 | **Sensor oponente** | Lectura en vivo del sensor elegido. El LED se enciende cuando detecta (lectura sin filtrar). | Sharp: valor 0–4095 · ToF: distancia en mm ("---" fuera de rango, "SIN ToF" si no responde) · digital: SI/NO. Barra (llena = cerca), marca del umbral y "OPONENTE!". | Una pulsación. |
| 8 | **Calibrar linea** | Paso 1: los 4 sensores sobre negro → pulsación. Paso 2: sobre blanco → pulsación. | Tabla `NEGR BLAN UMBR` por sensor. Un `!` marca poca diferencia. | Una pulsación para volver. |
| 9 | **Bateria** | Voltaje en vivo. | Voltaje grande, barra de %, voltaje por celda. "CARGAR" parpadea si está baja. | Una pulsación. |

**Siglas de los sensores de línea:** DI = delantero izquierdo · DD = delantero derecho · TI = trasero izquierdo · TD = trasero derecho.

---

## 6. Secuencia de pruebas recomendada

Haz las pruebas **en este orden** la primera vez y cada vez que cambies el cableado. Anota los resultados en la [bitácora](#10-bitácora-de-pruebas).

### Pruebas individuales (robot sobre un soporte, ruedas al aire)

- [ ] **9 · Batería:** compara el voltaje con un multímetro. Si difiere, ajusta `BAT_AJUSTE = multímetro / pantalla`.
- [ ] **3 · Motor izquierdo:** que diga ADELANTE cuando las ruedas empujan hacia adelante. Si no, `INVERTIR_IZQ = true`.
- [ ] **4 · Motor derecho:** lo mismo; si no, `INVERTIR_DER = true`.
- [ ] **5 · Movimientos:** que GIRO IZQ gire realmente a la izquierda.
- [ ] **6 · Sensores de línea:** pasa una hoja blanca bajo cada sensor; su círculo debe rellenarse.
- [ ] **8 · Calibrar línea** sobre el dohyo real. Ningún sensor debe quedar con `!`.
- [ ] **7 · Sensor oponente:** anota a qué distancia aparece "OPONENTE!". Ajusta `UMBRAL_IR` (Sharp) o `DISTANCIA_ATAQUE_MM` (ToF).
- [ ] **7 · Sensor oponente con objetos distintos:** blanco, **negro mate**, metálico y bajo (~3 cm). Anota cuáles detecta y a qué distancia: no hay que suponer cómo será el rival.

### Pruebas en conjunto (robot en el dohyo)

- [ ] **2 · Prueba en conjunto, sin oponente:** el robot gira buscando y **nunca se sale**; al tocar el borde retrocede y gira.
- [ ] **2 · Prueba en conjunto, con un objeto** (una caja de ~500 g): lo detecta, lo ataca y lo saca, y no se sale detrás de él.
- [ ] **2 · Giro con tracción trasera:** con el robot cerca del borde, verifica que al girar la cuña no saque los sensores delanteros del dohyo.
- [ ] **2 · Sin oponente durante más de 4 s:** el robot debe pasar a RONDA (avanzar en arco) y luego seguir buscando.
- [ ] **2 · Interferencia:** con otro sensor IR o ToF encendido apuntando al robot, que no ataque al aire ni escape de un borde inexistente.
- [ ] **1 · COMBATE:** espera exactamente 5 s antes de moverse.
- [ ] **1 · COMBATE:** 10 asaltos seguidos sin salirse solo. Anota la batería antes y después.

---

## 7. Lógica de combate

El robot repite este ciclo cada ~2 ms. **El borde siempre tiene prioridad sobre el ataque.**

Antes de decidir, cada lectura pasa por filtros para no depender de cómo sea el rival:
- **Línea:** "blanco" solo si se repite 2 lecturas seguidas (ignora destellos del IR del rival).
- **Oponente:** debe verse 25 ms sin interrupción para atacar (ignora interferencia) y se recuerda 200 ms al perderlo (sigue empujando a rivales negros o mate).

```mermaid
stateDiagram-v2
    [*] --> BUSCAR: fin de la cuenta regresiva
    BUSCAR --> ATACAR: ve oponente
    ATACAR --> BUSCAR: pierde oponente
    BUSCAR --> ESCAPE_ATRAS: borde delantero
    ATACAR --> ESCAPE_ATRAS: borde delantero
    ESCAPE_ATRAS --> ESCAPE_GIRO: 250 ms
    ESCAPE_GIRO --> BUSCAR: 300 ms o ve oponente
    BUSCAR --> ESCAPE_ADELANTE: borde trasero
    ATACAR --> ESCAPE_ADELANTE: borde trasero
    ESCAPE_ADELANTE --> BUSCAR: 250 ms
    BUSCAR --> PATRULLAR: 4 s sin ver a nadie
    PATRULLAR --> BUSCAR: 600 ms
    PATRULLAR --> ATACAR: ve oponente
    PATRULLAR --> ESCAPE_ATRAS: borde delantero
```

| Estado | Pantalla | Motores | Sale cuando |
|---|---|---|---|
| BUSCAR | BUSCAR | Giro sobre su eje a `VEL_BUSQUEDA`; cambia de sentido cada 2.5 s. | Ve al oponente → ATACAR; ve un borde → escape. |
| ATACAR | ATACAR | Ambos lados adelante a `VEL_ATAQUE`. | Pierde al oponente más de 200 ms → BUSCAR; ve un borde → escape. |
| PATRULLAR | RONDA | Avanza en arco (lado derecho a 2/3 de velocidad). | Ve al oponente → ATACAR; pasan 600 ms → BUSCAR; ve un borde → escape. |
| ESCAPE_ATRAS | ATRAS | Ambos atrás a `VEL_ESCAPE`. | Pasan 250 ms → ESCAPE_GIRO. |
| ESCAPE_GIRO | GIRO | Gira **hacia el lado contrario** del sensor que vio blanco (si fueron los dos, en el sentido de búsqueda). | Pasan 300 ms o ve al oponente → BUSCAR. |
| ESCAPE_ADELANTE | AVANZA | Ambos adelante a `VEL_ESCAPE`. | Pasan 250 ms → BUSCAR. |

---

## 8. Parámetros ajustables (config.h)

Después de cambiar un valor hay que **volver a compilar y subir** el código.

| Parámetro | Valor actual | Unidad | Qué cambia | Cuándo ajustarlo |
|---|---|---|---|---|
| `INVERTIR_IZQ` / `INVERTIR_DER` | false | — | Invierte el sentido de un lado. | Si la prueba de motor dice ADELANTE y el robot va hacia atrás. |
| `PWM_FREQ` | 1000 | Hz | Frecuencia del PWM. | Solo si cambian el puente H (TB6612 → 20000). |
| `UMBRAL_LINEA_DEFECTO` | 1500 | 0–4095 | Umbral de línea antes de calibrar. | Normalmente nunca: se usa la calibración. |
| `LINEA_DIGITAL` | false | — | Sensores de línea analógicos (false) o digitales (true). | Según los sensores montados. |
| `BLANCO_ES_BAJO` | true | — | Si el blanco da una lectura baja (QRE1113 y la mayoría de módulos). | Si la opción 6 muestra el blanco y el negro al revés. |
| `CONFIRMA_LINEA` | 2 | lecturas | Lecturas seguidas para creer un "blanco". | Súbelo a 3 si hay falsos escapes; bájalo a 1 si el robot se sale. |
| `SENSOR_OPONENTE` | SENSOR_SHARP | — | Qué sensor de oponente está montado. | `SENSOR_SHARP`, `SENSOR_DIGITAL`, `SENSOR_VL53L0X` o `SENSOR_VL53L1X`. |
| `UMBRAL_IR` | 1400 | 0–4095 | Sharp: lectura a partir de la cual hay oponente (≈ 25–30 cm, por confirmar). | Más alto = detecta más cerca; más bajo = más lejos, pero con más falsas alarmas. |
| `DISTANCIA_ATAQUE_MM` | 400 | mm | ToF: más cerca que esto es oponente. | El dohyo mide 77 cm: más de ~600 mm puede "ver" fuera de la pista. |
| `T_CONFIRMA_OPONENTE_MS` | 25 | ms | Tiempo que debe verse al oponente antes de atacar. | Súbelo si ataca al aire; bájalo si reacciona tarde. |
| `T_MEMORIA_OPONENTE_MS` | 200 | ms | Cuánto sigue empujando tras perderlo. | Súbelo contra rivales que el sensor ve a ratos. |
| `BAT_AJUSTE` | 1.00 | factor | Corrige el voltaje mostrado. | Voltaje del multímetro / voltaje en pantalla. |
| `BAT_BAJA` | 7.0 | V | Cuándo parpadea el aviso. | — |
| `T_PULSO_LARGO_MS` | 600 | ms | Duración mínima de una pulsación larga. | Si el menú confunde las pulsaciones. |
| `ESPERA_INICIO_MS` | 5000 | ms | Espera antes de pelear. | **No bajar de 5000** (regla oficial). |
| `VEL_ATAQUE` | 255 | 0–255 | Fuerza de empuje. | — |
| `VEL_BUSQUEDA` | 150 | 0–255 | Velocidad de giro al buscar. | Si el sensor IR "no alcanza a ver" al oponente, bájala. |
| `VEL_ESCAPE` | 200 | 0–255 | Velocidad de las maniobras de escape. | — |
| `T_RETROCESO_MS` | 250 | ms | Cuánto retrocede (o avanza) al ver el borde. | Si aun así se sale, súbelo. |
| `T_GIRO_ESCAPE_MS` | 300 | ms | Cuánto gira tras retroceder. | Mide en la opción 2 cuánto gira; depende del robot y de la batería. |
| `T_CAMBIO_BUSQUEDA` | 2500 | ms | Cada cuánto cambia el sentido de búsqueda. | — |
| `T_SIN_OPONENTE_MS` | 4000 | ms | Tiempo sin ver a nadie antes de patrullar. | — |
| `T_PATRULLA_MS` | 600 | ms | Duración del avance en arco. | Si llega al borde patrullando, bájalo. |
| `FACTOR_VEL_PRUEBA` | 0.5 | factor | Velocidad de la prueba en conjunto. | — |

Los **umbrales de línea calibrados** no están en `config.h`: viven en la memoria NVS. Se conservan al apagar y al volver a subir el código, y se reemplazan cada vez que calibras.

---

## 9. Solución de problemas

| Síntoma | Causa probable | Solución |
|---|---|---|
| La pantalla no enciende | Cableado SDA/SCL invertido, o dirección 0x3D. | Revisa 21 = SDA y 22 = SCL. Prueba `OLED_DIRECCION 0x3D`. El Monitor Serie avisa "no se encontro el OLED". |
| El ESP32 no arranca o entra en modo de programación | BOOT presionado al encender. | Suelta BOOT y presiona EN (reset). |
| El robot se reinicia al arrancar los motores | Caída de voltaje. | Alimenta el ESP32 con su propio step-down y revisa la tierra común. Carga la batería. |
| Un motor no gira | Jumper de ENA/ENB puesto, o falta tierra común. | Quita el jumper; une los GND. |
| Los motores giran débiles | El L298N pierde ~2 V; batería baja. | Revisa la opción 9. Es normal que sean más lentos que con un TB6612. |
| Un sensor de línea nunca cambia | Desconectado, demasiado alto o pin equivocado. | Debe estar a 1–3 mm del piso. Revisa en la opción 6. |
| La calibración marca `!` | Diferencia negro-blanco menor a 300. | Acerca el sensor al piso y limpia el lente. |
| "OPONENTE!" sin nada al frente | Umbral bajo, luz solar directa o interferencia del rival. | Sube `UMBRAL_IR` o baja `DISTANCIA_ATAQUE_MM`; sube `T_CONFIRMA_OPONENTE_MS`. |
| La opción 7 dice "SIN ToF" | El ToF no responde por I2C. | Revisa SDA 21 / SCL 22 y 3V3. Confirma que `SENSOR_OPONENTE` coincide con el modelo (L0X o L1X). |
| La opción 7 marca "---" con un objeto enfrente | Objeto fuera de rango, o muy oscuro o brillante y en ángulo. | Acércalo; prueba con el objeto de frente. Con VL53L0X el alcance real baja con objetos negros. |
| Escapa del borde sin estar en él | Destellos del IR del rival sobre los sensores de línea. | Sube `CONFIRMA_LINEA` a 3. |
| El robot se sale del dohyo | El retroceso no alcanza, falta calibrar o los sensores delanteros quedan detrás de la cuña. | Calibra en el dohyo real; sube `T_RETROCESO_MS`; baja `CONFIRMA_LINEA` a 1. |
| El voltaje dice "USB" con la batería conectada | Divisor desconectado o resistencias invertidas. | 10 kΩ va del + al pin y 4.7 kΩ del pin a GND. |
| El voltaje no coincide con el multímetro | Tolerancia de las resistencias. | `BAT_AJUSTE = multímetro / pantalla`. |

---

## 10. Bitácora de pruebas

Copia esta sección para cada sesión de pruebas o imprímela.

### Sesión

| Fecha | Responsable(s) | Versión del código (commit) | Sensor de oponente | Sensores de línea | Batería al inicio (V) | Batería al final (V) |
|---|---|---|---|---|---|---|
| | | | | | | |

### Pruebas individuales

| Prueba | Resultado (✔ / ✘) | Lecturas / observaciones | Ajuste aplicado |
|---|---|---|---|
| 9 · Batería (pantalla vs. multímetro) | | Pantalla: ___ V · Multímetro: ___ V | `BAT_AJUSTE` = |
| 3 · Motor izquierdo | | | `INVERTIR_IZQ` = |
| 4 · Motor derecho | | | `INVERTIR_DER` = |
| 5 · Movimientos | | | |
| 6 · Sensores de línea | | | |
| 7 · Sensor oponente (objeto blanco) | | Detecta a ___ cm · lectura ___ | `UMBRAL_IR` / `DISTANCIA_ATAQUE_MM` = |
| 7 · Sensor oponente (objeto negro mate) | | Detecta a ___ cm · lectura ___ | |

### Calibración de línea

| Sensor | Negro | Blanco | Umbral | ¿`!`? |
|---|---|---|---|---|
| DI | | | | |
| DD | | | | |
| TI | | | | |
| TD | | | | |

### Pruebas en conjunto y combate

| # | Modo (2 = conjunto / 1 = combate) | Oponente | Duración (s) | ¿Se salió? | ¿Sacó al oponente? | Observaciones |
|---|---|---|---|---|---|---|
| 1 | | | | | | |
| 2 | | | | | | |
| 3 | | | | | | |
| 4 | | | | | | |
| 5 | | | | | | |

### Cambios en el hardware o en config.h durante la sesión

| Hora | Qué se cambió | Por qué | Resultado |
|---|---|---|---|
| | | | |

Comentarios de Alex:El firmware compila sin errores ni avisos, pero todavía no lo he subido: no hay ningún ESP32 conectado a la computadora.

Resultado de la compilación

Placa: ESP32 Dev Module (core 3.3.12).
Ocupa 343 KB de los 1.3 MB de memoria de programa (26 %) y 24 KB de los 320 KB de RAM (7 %).
Corregí 8 avisos de conversión de tipos en el dibujo del robot.
También quedaron instaladas las librerías Adafruit SSD1306 2.5.17, GFX 1.12.6 y BusIO 1.17.4.
Para subir el código:

Si aun así no aparece, a la placa le falta el driver USB. Mira el chip junto al conector:
Si dice CP2102, instala el driver CP210x de Silicon Labs.
Si dice CH340, instala el driver CH340 de WCH.
Mientras subo, no presiones BOOT. Si la carga se queda en "Connecting…", mantén BOOT presionado unos segundos hasta que empiece.
