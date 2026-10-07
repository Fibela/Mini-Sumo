# Firmware Mini Sumo – ESP32

Firmware para el Mini Sumo de **tracción trasera (planos Rev B)**: ESP32 DevKit V1, puente H **L298N**, **2 motores N20** en el eje trasero, 4 sensores de línea, 1 sensor de oponente al frente y un display **OLED 0.96" SSD1306 I2C**.
Todo se prueba desde el display y el botón BOOT, sin necesidad de PC.

El código **no depende de un sensor en particular**. En `config.h` se elige:
- **Oponente:** Sharp GP2Y0A21 (analógico), sensor digital (JS40F, E18-D80NK…), **VL53L0X** o **VL53L1X** (ToF láser).
- **Línea:** analógicos (QRE1113 analógico) o digitales (TCRT5000, QRE1113 digital…).

📘 **[Manual de uso, conceptos, registro de pines y bitácora de pruebas](MANUAL.md)**

## Cargar el código

1. En Arduino IDE ve a *Preferencias → URLs adicionales* y agrega
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. En *Gestor de tarjetas* instala **esp32 de Espressif Systems** (probado con la 3.3.12; también sirve la 2.x).
3. En *Gestor de librerías* instala:
   - **Adafruit SSD1306** (acepta instalar también **Adafruit GFX** y **BusIO**)
   - Solo si usas ToF: **VL53L0X** o **VL53L1X** de **Pololu**
4. En `config.h` elige `SENSOR_OPONENTE` y `LINEA_DIGITAL` según el hardware montado.
5. Elige la placa **ESP32 Dev Module**, abre `MiniSumo_ESP32.ino` y súbelo.
6. El Monitor Serie a **115200** baudios es opcional; el display muestra lo mismo.

## Conexiones

| Componente | Pin del componente | GPIO del ESP32 |
|---|---|---|
| L298N, canal A (motor N20 izquierdo) | ENA (quitar el jumper) | 25 |
| | IN1 / IN2 | 26 / 27 |
| L298N, canal B (motor N20 derecho) | ENB (quitar el jumper) | 13 |
| | IN3 / IN4 | 18 / 19 |
| Sensor de línea delantero izq. | OUT | 36 (VP) |
| Sensor de línea delantero der. | OUT | 39 (VN) |
| Sensor de línea trasero izq. | OUT | 34 |
| Sensor de línea trasero der. | OUT | 35 |
| Oponente **Sharp o digital** | OUT | 32 |
| Oponente **ToF (VL53L0X / VL53L1X)** | SDA / SCL | 21 / 22 (bus compartido con el OLED) |
| Divisor de batería (10 kΩ + 4.7 kΩ) | punto medio | 33 |
| OLED SSD1306 | SDA / SCL | 21 / 22 |
| | VCC / GND | 3V3 / GND |
| Botón de inicio | botón **BOOT** de la placa | 0 |
| LED de estado | LED azul de la placa | 2 |

**Alimentación**
- LiPo 2S (7.4 V) → borne **12V** del L298N. Deja puesto el jumper del regulador 5V del L298N.
- LiPo 2S → regulador step-down a 5 V → pin **VIN/5V** del ESP32.
- Sensores de línea, OLED y ToF → **3V3** del ESP32. Sharp → **5V**; su salida máxima (~3.1 V) es segura para el ESP32.
- Sensor de oponente digital con salida de 5 V → **divisor de voltaje** antes del GPIO 32.
- El L298N pierde ~2 V internamente: con 7.4 V los motores reciben ~5.4 V.
- Divisor de batería: **+ LiPo → 10 kΩ → GPIO33 → 4.7 kΩ → GND**. Con la batería llena (8.4 V) llegan ~2.7 V al pin.
  Si el voltaje en pantalla no coincide con el del multímetro, ajusta `BAT_AJUSTE` en `config.h`.
- **Todas las tierras (GND) deben estar unidas.**

## Uso con el display y el botón BOOT

- **Pulsación corta** (menos de 0.6 s): pasa a la siguiente opción.
- **Pulsación larga** (la pantalla se invierte y el LED azul se enciende): entra a la opción.
- **Dentro de una prueba:** cualquier pulsación la detiene.
- **Esquina superior derecha:** voltaje e icono de la batería. Parpadea por debajo de 7.0 V (hay que cargar). Si dice "USB", no hay lectura de batería.

Al encender, el menú arranca en **COMBATE**: en la competencia basta con una pulsación larga.
No mantengas BOOT presionado mientras enciendes o reinicias, porque el ESP32 entraría en modo de programación.

## Menú

Haz las pruebas en este orden. En el Monitor Serie, el número de cada opción la ejecuta y `0` para los motores.

| # | Opción | Qué muestra el display | Qué verificar |
|---|---|---|---|
| 9 | Batería | Voltaje, porcentaje y voltaje por celda | Comparar con un multímetro y ajustar `BAT_AJUSTE` |
| 3, 4 | Motor izquierdo / derecho (ruedas al aire) | Velocidad y "ADELANTE / ATRAS" | Que coincida con lo que dice la pantalla. Si no, cambia `INVERTIR_IZQ` / `INVERTIR_DER`. |
| 5 | Movimientos | El nombre de cada movimiento | Que los giros vayan hacia el lado correcto |
| 6 | Sensores de línea | Dibujo del robot; un círculo relleno es un sensor que ve blanco | Que cada sensor cambie entre negro y blanco |
| 8 | Calibrar línea | Pasos negro → blanco y tabla con los resultados | Que ningún sensor aparezca marcado con `!`. El umbral queda guardado aunque apagues el robot. |
| 7 | Sensor oponente | Lectura (en mm si es ToF), barra y marca del umbral | A qué distancia aparece "OPONENTE!". Ajusta `UMBRAL_IR` o `DISTANCIA_ATAQUE_MM`. |
| 2 | **Prueba en conjunto** (50 % de velocidad) | Cuenta de 3 s; luego el estado, los sensores y el tiempo | Que el robot busque, ataque y no se salga del dohyo |
| 1 | **COMBATE** | Cuenta de 5 s y "A PELEAR!" | Pelea a velocidad completa. La pantalla no se actualiza durante la pelea para no frenar la reacción al borde. |

## Estrategia de combate

1. **Borde blanco al frente:** retrocede y gira hacia el lado contrario del sensor que lo detectó.
2. **Borde blanco atrás:** avanza.
3. **Oponente confirmado:** ataca de frente a velocidad máxima y sigue empujando 200 ms aunque lo pierda un instante.
4. **Nada detectado:** gira sobre su eje buscando y cambia de sentido cada 2.5 s.
5. **4 s sin ver a nadie:** patrulla avanzando en arco y vuelve a buscar desde otra posición.

Filtros para no depender del rival: el "blanco" debe repetirse 2 lecturas seguidas y el oponente debe verse 25 ms sin interrupción. Así se ignoran los destellos del IR o del láser del rival.
