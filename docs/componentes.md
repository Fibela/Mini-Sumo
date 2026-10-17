# Componentes: qué es cada cosa, para qué sirve y cómo se conecta

Guía de referencia del MiniSumo Rev. B. Sirve para dos rutas de microcontrolador:
**Raspberry Pi Pico (RP2040)** y **ESP32 DevKit V1**. Donde hay diferencia, se indica.

Para la asignación exacta de pines, ver [conexiones.md](conexiones.md).

---

## 1. Conceptos previos

Si estos ocho conceptos quedan claros, el resto del documento se lee solo.

| Concepto | Qué significa |
|---|---|
| **Microcontrolador (MCU)** | Un chip que corre un solo programa, sin sistema operativo. Arranca en milisegundos. No confundir con una computadora: el Pi **Pico** es un microcontrolador, la Raspberry **Pi** es una computadora con Linux. Son cosas distintas de la misma empresa |
| **GPIO** | *General Purpose Input/Output*. Cada patita del MCU que se puede programar como entrada o como salida |
| **Digital** | Una señal que solo vale 0 o 1 (0 V o 3.3 V). Un botón, un pin de dirección de motor |
| **Analógico** | Una señal que puede valer cualquier voltaje entre 0 y 3.3 V. La salida de un sensor de línea |
| **ADC** | *Analog-to-Digital Converter*. El circuito que convierte un voltaje analógico en un número. En el Pico da 0–4095; en el ESP32 también |
| **PWM** | *Pulse Width Modulation*. Encender y apagar muy rápido para regular potencia. Al 50 % el motor recibe la mitad de la energía. Es como se controla la velocidad |
| **I²C** | Un bus de solo dos cables (datos y reloj) donde se cuelgan varios dispositivos. Cada uno responde a una **dirección** distinta. Así conectamos cinco sensores de distancia con solo dos pines |
| **Masa común (GND)** | Todos los circuitos deben compartir el mismo punto de referencia de 0 V. Si el driver y el MCU tienen masas separadas, el driver no entiende las órdenes aunque los cables de señal estén bien. Es el error más común y el más difícil de ver |

---

## 2. El cerebro

### Raspberry Pi Pico (RP2040) — ruta A

**Qué es.** Un microcontrolador de dos núcleos a 133 MHz, lógica de 3.3 V, 26 pines GPIO
utilizables y 3 entradas analógicas.

**Qué hace en el robot.** Cuenta los 5 segundos obligatorios del arranque, lee todos los
sensores, decide qué hacer (buscar, atacar, empujar, escapar del borde) unas 200 veces por
segundo, y manda órdenes al driver de motores.

**A qué va conectado.** A todo: es el centro de la estrella.

**Cómo alimentarlo.**
- `VSYS` (pin 39) acepta de 1.8 a **5.5 V**. Aquí entran los 5 V del convertidor buck.
- `3V3 OUT` (pin 36) es una **salida** de 3.3 V que genera el propio Pico. De aquí sale la
  alimentación de todos los sensores y de la lógica del driver.
- **Nunca** conectar los 7.4 V de la batería a VSYS. Se quema al instante.

**Limitación importante.** Solo tiene **3 entradas analógicas** (GP26, GP27, GP28) y hay
4 sensores de línea. Hay dos salidas: comprar un multiplexor CD4051, o unir los dos sensores
traseros en paralelo sobre una sola entrada (atrás basta con saber que hay borde, no de qué lado).

**Cómo se programa.** Se conecta por USB; aparece como una memoria USB. Se puede usar
MicroPython o C/C++ con el SDK oficial.

### ESP32 DevKit V1 — ruta B

**Qué es.** Un microcontrolador de dos núcleos a 240 MHz, lógica de 3.3 V, con wifi y
bluetooth integrados.

**Ventaja decisiva para este robot.** Tiene **8 canales analógicos en ADC1** (GPIO 32 a 39).
Los cuatro sensores de línea entran directo y sobra para el monitor de batería: **no hace
falta el CD4051**.

**Cuidado con ADC2.** El ESP32 tiene dos convertidores. ADC2 deja de funcionar cuando el wifi
está activo. Regla simple: **todo lo analógico va en ADC1 (GPIO 32–39)**.

**Cuidado con los pines de arranque.** GPIO 0, 2, 5, 12 y 15 se leen al encender para decidir
cómo arranca el chip. Si se conectan mal, el ESP32 no arranca. GPIO 12 en alto al encender
impide el arranque: no usarlo para nada que pueda quedar en alto.

**Nota de reglamento.** MiniSumo prohíbe cualquier control externo — wifi, bluetooth,
radiofrecuencia. El ESP32 los trae integrados aunque el firmware no los inicialice. Queda
pendiente confirmarlo con el responsable de categoría antes de competir.

**Cómo alimentarlo.** El pin `VIN` acepta 5 V (del buck). El pin `3V3` es salida para sensores.

---

## 3. Alimentación

### Batería 18650 2S

**Qué es.** Dos celdas de litio de 18 × 65 mm en serie. Da 8.4 V cargada, 7.4 V nominal y
cerca de 6.0 V descargada. Capacidad de 2550 mAh.

**Qué alimenta.** Dos cosas, por caminos separados:
1. **Directo a `VM` del driver** — los motores funcionan bien con 7.4 V.
2. **A través del convertidor buck** → 5 V → el microcontrolador.

**Cómo conectarla.** El positivo pasa primero por el **interruptor principal**, y de ahí se
reparte a las dos ramas. El negativo va a la masa común.

**Advertencia de seguridad.** Nunca descargarla por debajo de 3.0 V por celda (6.0 V en total)
ni cargarla sin balanceador. Se transporta y se carga en bolsa ignífuga — el reglamento del
foro lo exige.

**Nota de montaje.** Las dos celdas juntas miden 37 × 65 × 18.5 mm. Van **transversales** en
el chasis, con el eje largo cruzando el robot.

### Convertidor buck a 5 V *(pendiente de comprar)*

**Qué es.** Un circuito que baja un voltaje de forma eficiente, sin convertirlo en calor
como lo haría un regulador lineal.

**Para qué sirve aquí.** Es el único puente posible entre la batería de 7.4 V y el
microcontrolador, que aguanta 5.5 V como máximo. **Sin este componente no hay robot autónomo.**

**Cómo conectarlo.**
- Entrada: `IN+` al positivo después del switch, `IN-` a masa.
- Salida: `OUT+` a `VSYS` (Pico) o `VIN` (ESP32), `OUT-` a masa.
- **Antes de conectar el MCU:** alimentar el buck, medir la salida con multímetro y ajustar
  el potenciómetro hasta leer 5.0 V exactos. Confirmarlo dos veces. Este paso no se salta.

---

## 4. Motores y driver

### Micromotorreductor Pololu 50:1

**Qué es.** Un motor de corriente continua de 6 V con una caja reductora metálica de 50:1
integrada. Gira a 590 RPM en vacío y entrega unos 0.47 kg·cm de par de paro.

**Qué hace.** Mueve las ruedas traseras. Uno por lado, controlados de forma independiente:
girando en sentidos opuestos el robot pivota sobre su propio eje.

**Cómo se conecta.** Dos cables, a las salidas del driver. No tiene polaridad fija: si la
rueda gira al revés, se invierte en el firmware, no en el cableado.

**Cuidado.** Son de 6 V y el driver recibe 7.4 V. Hay que **limitar el PWM al 80 %** en el
firmware, o se acortará su vida.

### Driver TB6612FNG

**Qué es.** Un circuito de doble puente H con transistores MOSFET. Un "puente H" es el arreglo
que permite invertir el sentido de giro de un motor.

**Por qué hace falta.** Los pines del microcontrolador entregan miliamperios; los motores
piden más de un amperio. El driver toma las órdenes de la lógica y entrega la corriente desde
la batería.

**Por qué este y no el L298N.** El TB6612 es MOSFET y pierde ~0.3 V; el L298N es bipolar y
pierde ~2 V, que se van en calor. Además pesa 5 g contra 28, y mide 20 × 20 mm contra 43 × 43.

**Cómo se conecta.** Tiene tres grupos de pines:

| Grupo | Pines | A dónde va |
|---|---|---|
| **Potencia** | `VM` | Positivo de la batería (7.4 V), después del switch |
| | `GND` | Masa común |
| **Lógica** | `VCC` | 3.3 V del microcontrolador |
| | `STBY` | Un GPIO. En alto habilita el driver; en bajo lo deja dormido |
| | `PWMA`, `AIN1`, `AIN2` | Tres GPIO: velocidad y dirección del motor izquierdo |
| | `PWMB`, `BIN1`, `BIN2` | Tres GPIO: velocidad y dirección del motor derecho |
| **Salidas** | `AO1`, `AO2` | Los dos cables del motor izquierdo |
| | `BO1`, `BO2` | Los dos cables del motor derecho |

**Cómo se controla cada motor.**

| AIN1 | AIN2 | PWM | Resultado |
|---|---|---|---|
| ALTO | BAJO | 0–255 | Gira adelante a esa velocidad |
| BAJO | ALTO | 0–255 | Gira atrás |
| ALTO | ALTO | — | **Freno activo**: se detiene de golpe |
| BAJO | BAJO | — | Rueda libre |

El freno activo importa en sumo: al detectar el borde hay que parar de golpe, no rodar hasta
salirse.

**Dos cosas que no se pueden olvidar.**
1. Un **condensador de 470 µF** soldado directamente entre `VM` y `GND`, pegado al módulo.
   Sin él, los picos de corriente al arrancar los motores hunden el voltaje y reinician el MCU.
2. La **masa del driver y la del microcontrolador deben estar unidas**, aunque se alimenten
   por caminos distintos. Si no, el driver no responde y todo parece estar bien conectado.

**Dónde se monta.** De canto contra la pared trasera del chasis, cerca de los motores: acorta
los cables de motor, que son los que meten ruido en el bus I²C.

---

## 5. Sensores

### VL53L1X — sensor de distancia ToF (×5)

**Qué es.** Un sensor láser que mide distancia por *tiempo de vuelo*: emite un pulso de luz
infrarroja invisible y cronometra cuánto tarda en volver.

**Por qué es mejor que un ultrasónico.** El ultrasónico rebota el sonido, y contra una cuña
inclinada —que es lo que todos los rivales traen— el eco se va para otro lado. El láser no
tiene ese problema. Además responde en 20 ms contra 60, y tiene campo de visión configurable.

**Qué hace en el robot.** Cinco en abanico al frente, para saber si hay rival y hacia qué lado.
Con uno solo sabrías que hay algo, pero no hacia dónde girar.

**Cómo se conecta.** Por I²C, los cinco al mismo par de cables:

| Pin del sensor | A dónde va |
|---|---|
| `VIN` | 3.3 V |
| `GND` | Masa |
| `SDA` | Pin de datos I²C del MCU — compartido por los cinco |
| `SCL` | Pin de reloj I²C del MCU — compartido por los cinco |
| `XSHUT` | Un GPIO **distinto para cada sensor** |

**El problema de las direcciones, y por qué existe XSHUT.** Los cinco salen de fábrica con la
misma dirección: `0x29`. Si se conectan todos a la vez, responden todos al mismo tiempo y el bus
devuelve basura. `XSHUT` apaga el sensor cuando se pone en bajo. La secuencia de arranque es:

1. Poner los cinco XSHUT en **bajo**: todos apagados.
2. Levantar el XSHUT del primero. Solo él despierta, en `0x29`.
3. Por software, cambiarle la dirección a `0x30`.
4. Levantar el segundo. Despierta en `0x29`, que ahora está libre. Cambiarla a `0x31`.
5. Repetir hasta el quinto.

Sin esta secuencia nada funciona, y el síntoma parece un problema de código.

**El segundo problema: los pull-ups.** Cada módulo trae sus propias resistencias de pull-up en
el bus. Con cinco módulos en paralelo la resistencia equivalente baja a un quinto y las señales
dejan de cumplir sus tiempos. **Hay que desoldar los pull-ups de cuatro de los cinco módulos**,
o quitarlos de todos y poner un solo par de 4.7 kΩ en el cable troncal.

**Limitación honesta.** Las carcasas negras mate absorben el infrarrojo y reducen el alcance
efectivo. A menos de 30 cm no es problema, pero por eso conviene el respaldo del INA219.

### QRE1113 — sensor de línea (×4)

**Qué es.** Un LED infrarrojo y un fototransistor juntos, apuntando al suelo. Mide cuánta luz
rebota.

**Qué hace.** Detecta el borde blanco del dohyo, que es la única forma de no salirse solo.
Dos adelante y dos atrás.

**Cómo leerlo.** Es **analógico**: la salida es un voltaje. Negro refleja poco (voltaje alto),
blanco refleja mucho (voltaje bajo). La relación es al revés de lo que uno espera.

**Cómo se conecta.**

| Pin | A dónde va |
|---|---|
| `VCC` | 3.3 V |
| `GND` | Masa |
| `OUT` | Una entrada ADC del microcontrolador |

- **Con ESP32:** los cuatro directo a GPIO 32, 33, 34 y 35.
- **Con Pico:** solo hay 3 entradas. O se usa un CD4051, o los dos traseros se unen en paralelo
  sobre la misma entrada, que indica "hay borde atrás" sin decir de qué lado.

**Por qué hay que calibrarlos.** El valor de "blanco" y "negro" depende de la luz del lugar, de
la altura a la que quedó el sensor y del acabado del dohyo. El dohyo del foro es de madera
pintada, no formaica, y eso cambia las lecturas. Se calibra **sobre el dohyo real el Día 1**,
midiendo el negro y el blanco y poniendo el umbral a la mitad.

**Dónde se montan.** Lo más adelantados posible, justo detrás de la cuña. El borde blanco de
MiniSumo mide apenas 3 cm: cada milímetro de anticipación cuenta.

### INA219 — sensor de corriente

**Qué es.** Un medidor de corriente y voltaje que se comunica por I²C. Lleva una resistencia
muy pequeña en serie con la carga y mide la caída de voltaje en ella.

**Qué hace en el robot.** Dos cosas:
1. **Detectar contacto.** Cuando el robot empuja contra el rival, los motores se frenan y la
   corriente se dispara. Eso es una señal de "estoy empujando" que no depende de ningún sensor
   óptico y que funciona aunque el rival sea negro mate.
2. **Vigilar la batería**, para no descargarla por debajo del límite seguro.

**Cómo se conecta.** Es el único sensor que va en **dos** circuitos a la vez:

| Pin | A dónde va |
|---|---|
| `VCC` | 3.3 V |
| `GND` | Masa |
| `SDA` / `SCL` | Al bus I²C, junto con los ToF. Su dirección es `0x40`, no choca con nadie |
| `VIN+` | Al positivo de la batería, **antes** del driver |
| `VIN-` | A `VM` del driver |

O sea: la corriente de los motores **pasa a través** del INA219. Esa es la única forma de medirla.

### Display OLED SSD1306 *(opcional)*

**Qué es.** Una pantalla monocroma de 0.96" que se conecta por I²C, dirección `0x3C`.

**Para qué sirve.** Para ver los valores crudos de los sensores durante la calibración del
Día 1 sin cargar una laptop, y para navegar un menú de pruebas.

**Estado en este proyecto.** Es un **complemento final, no obligatorio**. El diseño reserva su
dirección en el bus y su espacio en el chasis. Mientras no esté, el menú y la calibración van
por consola serial a través del USB.

**Regla si se instala:** **no actualizarlo durante el combate.** Comparte bus con los cinco ToF
y refrescar la pantalla cuesta decenas de milisegundos que el lazo de control no puede ceder.
Se dibuja en el estado de armado y se deja congelado mientras pelea.

---

## 6. Elementos pasivos y de montaje

| Componente | Para qué sirve | Cómo va |
|---|---|---|
| **Interruptor principal** | Cortar toda la energía. Es el único corte posible: MiniSumo prohíbe el paro remoto | En serie con el positivo de la batería, antes de todo lo demás |
| **Botón de arranque** | Iniciar la cuenta de 5 segundos que exige el reglamento | Entre un GPIO y masa, con el pull-up interno activado |
| **Condensador 470 µF** | Absorber los picos de corriente de los motores | Entre `VM` y `GND` del driver, soldado al módulo |
| **Resistencias 4.7 kΩ (×2)** | Pull-ups del bus I²C | De `SDA` a 3.3 V y de `SCL` a 3.3 V. **Un solo par en todo el bus** |
| **Divisor de voltaje** | Dejar que el ADC lea el voltaje de la batería sin quemarse | 100 kΩ y 47 kΩ: con 8.4 V entrega 2.68 V, dentro del rango seguro |
| **Headers hembra** | Permitir sacar los módulos sin desoldar | Soldados a la placa madre. Los módulos se enchufan |
| **Conectores JST-XH** | Unir la placa con los sensores y el driver sin que se suelten | Crimpados. Tienen retención mecánica, los dupont no |

**Sobre los jumpers dupont:** sirven para la protoboard y nada más. En combate, un robot recibe
golpes a un metro por segundo y los dupont se aflojan solos. Un cable de I²C que se sale a mitad
del segundo round es un combate perdido sin saber por qué.

---

## 7. Mapa de conexiones, de un vistazo

```
          BATERÍA 18650 2S (7.4 V)
                    │
              [ SWITCH ]
                    │
        ┌───────────┴───────────┐
        │                       │
   [ BUCK 5 V ]            [ INA219 ]  ← mide la corriente de motores
        │                       │
    VSYS / VIN              VM del driver
        │                       │
   ┌────┴────┐            [ TB6612 ]
   │   MCU   │────────────────┘  señales: PWM, dirección, STBY
   │ Pico ó  │                       │
   │  ESP32  │                  ┌────┴────┐
   └────┬────┘               MOTOR IZQ  MOTOR DER
        │
        ├── 3.3 V ──→ alimenta todos los sensores
        │
        ├── I²C (2 cables) ──→ 5 × VL53L1X + INA219 + OLED opcional
        │
        ├── 5 GPIO ──→ XSHUT de cada ToF
        │
        ├── ADC ──→ 4 × QRE1113 (directo en ESP32; con mux o en paralelo en Pico)
        │
        └── 1 GPIO ──→ botón de arranque

        TODAS LAS MASAS AL MISMO PUNTO
```

---

## 8. Los tres errores que más cuestan

1. **Conectar 7.4 V al microcontrolador.** VSYS del Pico y VIN del ESP32 aguantan hasta 5.5 V.
   No hay fusible ni protección: se quema en el instante y no hay vuelta atrás. El buck se
   ajusta y se mide **antes** de que su salida toque el MCU.

2. **Dejar los pull-ups en los cinco módulos ToF.** El bus deja de funcionar y el síntoma
   parece de firmware, así que se pierden horas buscando en el lugar equivocado.

3. **Olvidar la masa común.** Cuando el driver se alimenta de la batería y el MCU del USB,
   es fácil pensar que son circuitos independientes. No lo son: sin masa común el driver
   simplemente no responde.
