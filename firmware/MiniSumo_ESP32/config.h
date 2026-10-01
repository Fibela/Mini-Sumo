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

// Motores: los 2 motores D20 de cada lado van EN PARALELO a un canal.
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

// QRE1113: más reflejo (blanco) => voltaje MÁS BAJO.
#define BLANCO_ES_BAJO     true

// Umbral por defecto (0-4095). Se reemplaza al calibrar (opción 8 "Calibrar linea")
// y queda guardado en la memoria del ESP32.
#define UMBRAL_LINEA_DEFECTO 1500

// ---------------------------------------------------------------------
//  Sensor IR de oponente (x1, al frente)
//  IR_ANALOGICO true  : tipo Sharp GP2Y0A21 (más cerca = más voltaje)
//  IR_ANALOGICO false : sensor digital (JS40F, E18-D80NK, etc.)
//  Si el sensor da 5 V en su salida, usar divisor de voltaje a 3.3 V.
// ---------------------------------------------------------------------
#define PIN_IR_OPONENTE    32
#define IR_ANALOGICO       true   // confirmado: Sharp GP2Y0A21
#define UMBRAL_IR          1400   // analógico: lectura mayor => oponente
#define IR_ACTIVO_BAJO     true   // digital: LOW => oponente

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

// Prueba en conjunto: misma lógica que combate, pero más lento
#define FACTOR_VEL_PRUEBA  0.5f
#define ESPERA_PRUEBA_MS   3000
