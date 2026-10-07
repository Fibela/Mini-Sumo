// =====================================================================
//  config.h  -  Configuración del Mini Sumo (ESP32 DevKit V1)
//  Todo lo que depende del hardware se ajusta AQUÍ, no en el .ino
// =====================================================================
#pragma once

// ---------------------------------------------------------------------
//  Puente H
//  DRIVER_TB6612 : usa pin STBY (se pone en HIGH al iniciar)
//  DRIVER_L298N  : quitar los jumpers de ENA/ENB, ahí va el PWM
//  Ambos se controlan con IN1/IN2 (dirección) + PWM (velocidad).
// ---------------------------------------------------------------------
#define DRIVER_TB6612 1
#define DRIVER_L298N  2
#define PUENTE_H      DRIVER_L298N   // confirmado: L298N

// Motores (Rev B, tracción trasera): 1 motor N20 por lado en el eje trasero.
// Si se usan 2 motores por lado, van EN PARALELO al mismo canal.
// Lado izquierdo  -> canal A del puente H
#define PIN_IZQ_PWM   25
#define PIN_IZQ_IN1   26
#define PIN_IZQ_IN2   27
// Lado derecho    -> canal B del puente H
#define PIN_DER_PWM   13
#define PIN_DER_IN1   18
#define PIN_DER_IN2   19
// Solo TB6612
#define PIN_STBY      23

// Si un lado gira al revés en la prueba de motores, cambia su valor a true
#define INVERTIR_IZQ  false
#define INVERTIR_DER  false

// L298N es lento conmutando: a 20 kHz pierde mucha fuerza. 1 kHz va bien.
#define PWM_FREQ      1000    // Hz (TB6612 admite 20000)
#define PWM_RES       8       // bits -> velocidades de 0 a 255

// ---------------------------------------------------------------------
//  Sensores de línea QRE1113 analógicos (x4)
//  Solo pines ADC1 (32-39): ADC2 no funciona bien en ESP32.
//  36 = VP, 39 = VN en la serigrafía de la placa.
// ---------------------------------------------------------------------
#define PIN_LINEA_DEL_IZQ  36   // delantero izquierdo
#define PIN_LINEA_DEL_DER  39   // delantero derecho
#define PIN_LINEA_TRA_IZQ  34   // trasero izquierdo
#define PIN_LINEA_TRA_DER  35   // trasero derecho

// LINEA_DIGITAL false : sensores analógicos (QRE1113 analógico)
// LINEA_DIGITAL true  : módulos digitales (TCRT5000, QRE1113 digital...)
#define LINEA_DIGITAL      false

// QRE1113 y la mayoría de módulos: más reflejo (blanco) => voltaje MÁS BAJO.
#define BLANCO_ES_BAJO     true

// Lecturas seguidas de "blanco" necesarias para creerlas (filtra destellos
// del IR/láser del rival). 2 lecturas ≈ 3–4 ms de retraso.
#define CONFIRMA_LINEA     2

// Umbral por defecto (0-4095). Se reemplaza al calibrar (opción 8 "Calibrar linea")
// y queda guardado en la memoria del ESP32.
#define UMBRAL_LINEA_DEFECTO 1500

// ---------------------------------------------------------------------
//  Sensor de oponente (x1, al frente) - elegir UNO
//  SENSOR_SHARP   : Sharp GP2Y0A21 analógico en GPIO32 (más cerca = más voltaje)
//  SENSOR_DIGITAL : JS40F, E18-D80NK, etc. en GPIO32 (salida 5 V -> divisor)
//  SENSOR_VL53L0X : ToF láser I2C (0x29), mismo bus que el OLED (21/22)
//  SENSOR_VL53L1X : ToF láser I2C (0x29), más alcance y más inmune a la luz
//  Librerías ToF: "VL53L0X" y "VL53L1X" de Pololu (solo la que se use)
// ---------------------------------------------------------------------
#define SENSOR_SHARP       1
#define SENSOR_DIGITAL     2
#define SENSOR_VL53L0X     3
#define SENSOR_VL53L1X     4
#define SENSOR_OPONENTE    SENSOR_SHARP

#define PIN_IR_OPONENTE    32     // Sharp o digital
#define UMBRAL_IR          1400   // Sharp: lectura mayor => oponente (≈25-30 cm)
#define IR_ACTIVO_BAJO     true   // digital: LOW => oponente
#define DISTANCIA_ATAQUE_MM 400   // ToF: más cerca que esto => oponente
                                  // (el dohyo mide 77 cm de diámetro)

// Filtro del oponente (no depender de cómo sea el rival):
// - debe verse sin interrupción este tiempo para atacar (filtra interferencia)
// - al perderlo se sigue empujando este tiempo (rivales negros/mate que
//   el sensor ve a ratos)
#define T_CONFIRMA_OPONENTE_MS 25
#define T_MEMORIA_OPONENTE_MS  200

// ---------------------------------------------------------------------
//  Batería LiPo 2S (divisor de voltaje -> GPIO33)
//    + batería --[ 10k ]--+--[ 4.7k ]-- GND
//                         +--> GPIO33
//  Con 8.4 V en la batería llegan ~2.7 V al pin (máx. seguro 3.3 V).
// ---------------------------------------------------------------------
#define USAR_BATERIA       true
#define PIN_BATERIA        33
#define BAT_R_ARRIBA       10.0f  // kOhm, del + de la batería al pin
#define BAT_R_ABAJO        4.7f   // kOhm, del pin a GND
#define BAT_AJUSTE         1.00f  // = voltaje del multímetro / voltaje en pantalla
#define BAT_LLENA          8.4f   // 2S cargada (4.2 V por celda)
#define BAT_VACIA          6.8f   // 2S descargada (3.4 V por celda)
#define BAT_BAJA           7.0f   // debajo de esto parpadea el aviso: cargar

// ---------------------------------------------------------------------
//  Interfaz
// ---------------------------------------------------------------------
#define PIN_BOTON          0      // botón BOOT de la placa
#define PIN_LED            2      // LED azul de la placa
#define T_PULSO_LARGO_MS   600    // pulsación corta = siguiente, larga = OK

// Display OLED 0.96" SSD1306 128x64 I2C
// Librerías: "Adafruit SSD1306" y "Adafruit GFX" (Gestor de librerías)
#define PIN_SDA            21
#define PIN_SCL            22
#define OLED_DIRECCION     0x3C   // algunos módulos usan 0x3D
#define REFRESCO_PANTALLA_MS 150  // en prueba en conjunto
// En COMBATE la pantalla no se actualiza mientras pelea: cada refresco
// bloquea ~25 ms y retrasaría la detección del borde.

// ---------------------------------------------------------------------
//  Estrategia (velocidades 0-255, tiempos en ms)
// ---------------------------------------------------------------------
#define ESPERA_INICIO_MS   5000   // regla: 5 s antes de moverse
#define VEL_ATAQUE         255
#define VEL_BUSQUEDA       150
#define VEL_ESCAPE         200
#define T_RETROCESO_MS     250
#define T_GIRO_ESCAPE_MS   300
#define T_CAMBIO_BUSQUEDA  2500   // cambia el sentido de giro al buscar
// Si no ve a nadie en este tiempo, patrulla avanzando en arco: el rival puede
// ser invisible para el sensor (negro mate, muy bajo) o estar quieto lejos.
#define T_SIN_OPONENTE_MS  4000
#define T_PATRULLA_MS      600

// Prueba en conjunto: misma lógica que combate, pero más lento
#define FACTOR_VEL_PRUEBA  0.5f
#define ESPERA_PRUEBA_MS   3000
