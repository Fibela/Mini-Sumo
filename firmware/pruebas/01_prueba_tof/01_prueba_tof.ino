/* ============================================================================
 * MiniSumo UVM 2026 — Prueba 01: sensores de distancia VL53L1X
 * ============================================================================
 *
 * QUE HACE ESTE PROGRAMA
 * ----------------------
 * Despierta los cinco sensores de distancia uno por uno, le da a cada uno una
 * direccion I2C distinta, y despues los lee en bucle mostrando las distancias
 * por el monitor serie.
 *
 * POR QUE HACE FALTA UN PROGRAMA SOLO PARA ESTO
 * ---------------------------------------------
 * Los cinco VL53L1X salen de fabrica con la MISMA direccion: 0x29. En un bus
 * I2C cada dispositivo necesita una direccion unica, porque la direccion es
 * como el nombre con el que el microcontrolador lo llama. Si los cinco se
 * llaman igual, todos contestan a la vez y lo que llega al bus es basura.
 *
 * La solucion es el pin XSHUT de cada sensor: cuando se pone en BAJO, el
 * sensor se apaga por completo y suelta el bus. Entonces se pueden encender
 * de uno en uno y renombrarlos antes de encender el siguiente.
 *
 * Este es el problema que mas tiempo hace perder a los equipos, y el sintoma
 * engaña: parece un error de codigo o un sensor defectuoso.
 *
 * SIRVE PARA LAS DOS PLACAS
 * -------------------------
 * El mismo archivo compila en Raspberry Pi Pico y en ESP32. Los pines se
 * eligen solos segun la placa seleccionada en el IDE.
 *
 * QUE NECESITAS ANTES DE CARGARLO
 * -------------------------------
 *   - Arduino IDE con el soporte de tu placa instalado:
 *       Pico  -> gestor de tarjetas "Raspberry Pi Pico/RP2040" (de Earle Philhower)
 *       ESP32 -> gestor de tarjetas "esp32" (de Espressif)
 *   - La libreria "VL53L1X" de Pololu
 *       Gestor de librerias -> buscar "VL53L1X" -> instalar la de Pololu
 *   - Los sensores conectados segun docs/conexiones.md
 *   - Alimentacion: solo el cable USB. La bateria NO hace falta todavia.
 *
 * COMO SE USA
 * -----------
 *   1. Conecta la placa por USB.
 *   2. Carga este programa.
 *   3. Abre el Monitor Serie a 115200 baudios.
 *   4. Deberias ver los cinco sensores inicializarse y luego sus distancias.
 *
 * ========================================================================= */

#include <Wire.h>      // Comunicacion I2C: es la que maneja los dos cables SDA y SCL
#include <VL53L1X.h>   // Libreria de Pololu para los sensores de distancia


/* ============================================================================
 * SECCION 1 — CONFIGURACION DE PINES SEGUN LA PLACA
 * ============================================================================
 * Estas lineas no se ejecutan: se resuelven al compilar. El compilador mira
 * que placa elegiste en el IDE y se queda solo con el bloque que corresponde.
 * Asi un mismo archivo sirve para las dos rutas del proyecto.
 */

#if defined(ARDUINO_ARCH_RP2040)
  // ---------- RASPBERRY PI PICO ----------
  #define NOMBRE_PLACA "Raspberry Pi Pico (RP2040)"

  const uint8_t PIN_SDA = 4;    // GP4  — datos del bus I2C
  const uint8_t PIN_SCL = 5;    // GP5  — reloj del bus I2C

  // Un pin XSHUT por sensor. El orden es de izquierda a derecha vistos
  // desde atras del robot, mirando hacia donde mira la cuña.
  const uint8_t PIN_XSHUT[5] = { 6, 7, 8, 9, 10 };   // GP6 a GP10

#elif defined(ARDUINO_ARCH_ESP32)
  // ---------- ESP32 DEVKIT V1 ----------
  #define NOMBRE_PLACA "ESP32 DevKit V1"

  const uint8_t PIN_SDA = 21;   // GPIO 21 — datos del bus I2C
  const uint8_t PIN_SCL = 22;   // GPIO 22 — reloj del bus I2C

  // OJO: evitamos GPIO 6 a 11 (van a la memoria flash interna) y GPIO 12
  // (si queda en alto al encender, el ESP32 no arranca).
  const uint8_t PIN_XSHUT[5] = { 4, 16, 17, 23, 13 };

#else
  #error "Placa no soportada. Elige Raspberry Pi Pico o ESP32 en el IDE."
#endif


/* ============================================================================
 * SECCION 2 — PARAMETROS QUE PUEDES AJUSTAR
 * ========================================================================= */

const uint8_t  NUM_SENSORES   = 5;      // Cuantos sensores hay conectados.
                                        // Si todavia te falta uno, pon 4 aqui
                                        // y el programa funciona igual.

const uint8_t  DIRECCION_BASE = 0x30;   // La primera direccion nueva. Los
                                        // sensores quedaran en 0x30, 0x31,
                                        // 0x32, 0x33 y 0x34.
                                        // No usamos 0x40 porque ahi va el
                                        // INA219, ni 0x3C que es del OLED.

const uint16_t TIEMPO_MEDIDA_US = 20000; // Cuanto tiempo dedica el sensor a
                                         // cada medicion, en microsegundos.
                                         // Mas tiempo = mas alcance y menos
                                         // ruido, pero mas lento.
                                         // 20000 us = 20 ms = 50 lecturas/s.

const uint16_t PERIODO_MS = 20;          // Cada cuanto repite la medicion.
                                         // No debe ser menor que el tiempo
                                         // de medida de arriba.

const uint32_t VELOCIDAD_I2C = 400000;   // 400 kHz (modo rapido). El VL53L1X
                                         // lo soporta. Si el bus da errores,
                                         // bajar a 100000 para descartar que
                                         // sea problema de cableado.


/* ============================================================================
 * SECCION 3 — VARIABLES GLOBALES
 * ========================================================================= */

// Un objeto por sensor. Cada uno guarda su direccion y su estado.
VL53L1X sensor[NUM_SENSORES];

// Para saber cuales arrancaron bien y cuales no.
bool sensorOK[NUM_SENSORES];

// Nombres legibles, solo para que el monitor serie se entienda.
const char* NOMBRE_SENSOR[5] = {
  "IZQUIERDO", "IZQ-CENTRO", "CENTRO", "DER-CENTRO", "DERECHO"
};


/* ============================================================================
 * SECCION 4 — ARRANQUE
 * ========================================================================= */

void setup() {

  // ---- 4.1 Monitor serie ----
  Serial.begin(115200);

  // Esperamos hasta 3 segundos a que se abra el monitor serie. Sin esto, los
  // primeros mensajes se pierden porque el programa arranca antes de que la
  // computadora termine de conectarse.
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F(" MiniSumo UVM 2026 - Prueba 01"));
  Serial.println(F(" Sensores de distancia VL53L1X"));
  Serial.print (F(" Placa: "));
  Serial.println(F(NOMBRE_PLACA));
  Serial.println(F("========================================"));
  Serial.println();


  // ---- 4.2 Apagar TODOS los sensores antes de tocar el bus ----
  //
  // Este es el paso clave. Si encendemos el bus con los cinco sensores vivos,
  // los cinco responden en 0x29 y ya no hay forma de distinguirlos.
  //
  // Poniendo XSHUT en BAJO el sensor se apaga y suelta el bus por completo.

  Serial.println(F("[1/4] Apagando todos los sensores..."));

  for (uint8_t i = 0; i < NUM_SENSORES; i++) {
    pinMode(PIN_XSHUT[i], OUTPUT);     // El pin sera una salida
    digitalWrite(PIN_XSHUT[i], LOW);   // y la ponemos en BAJO = sensor apagado
  }

  delay(50);   // Un respiro para que todos terminen de apagarse


  // ---- 4.3 Arrancar el bus I2C ----

  Serial.println(F("[2/4] Iniciando bus I2C..."));

  #if defined(ARDUINO_ARCH_RP2040)
    // En el Pico hay que decirle a la libreria que pines usar ANTES de begin()
    Wire.setSDA(PIN_SDA);
    Wire.setSCL(PIN_SCL);
    Wire.begin();
  #else
    // En el ESP32 los pines se pasan directo a begin()
    Wire.begin(PIN_SDA, PIN_SCL);
  #endif

  Wire.setClock(VELOCIDAD_I2C);

  Serial.print(F("      SDA = pin "));  Serial.print(PIN_SDA);
  Serial.print(F(" | SCL = pin "));     Serial.print(PIN_SCL);
  Serial.print(F(" | "));               Serial.print(VELOCIDAD_I2C / 1000);
  Serial.println(F(" kHz"));
  Serial.println();


  // ---- 4.4 Despertar los sensores UNO POR UNO ----
  //
  // Esta es la secuencia que hace que todo funcione. Para cada sensor:
  //
  //   a) Levantamos su XSHUT -> ese sensor despierta en la direccion 0x29.
  //      Los demas siguen apagados, asi que 0x29 esta libre y no hay choque.
  //
  //   b) Le hablamos en 0x29 y le damos su direccion nueva.
  //
  //   c) Ahora 0x29 vuelve a quedar libre para el siguiente.
  //
  // Si te saltas el orden o levantas dos a la vez, dos sensores quedan con la
  // misma direccion y el bus se vuelve impredecible.

  Serial.println(F("[3/4] Despertando sensores uno por uno:"));
  Serial.println();

  for (uint8_t i = 0; i < NUM_SENSORES; i++) {

    // (a) Despertar SOLO este sensor
    digitalWrite(PIN_XSHUT[i], HIGH);
    delay(10);    // El sensor tarda unos milisegundos en estar listo.
                  // Sin esta pausa, a veces no responde al primer intento.

    // Decimos a la libreria con que bus debe hablar
    sensor[i].setBus(&Wire);

    // Timeout: si un sensor no contesta, no queremos que el programa se
    // quede colgado para siempre esperandolo.
    sensor[i].setTimeout(500);

    // (b) init() despierta el sensor y carga su configuracion por defecto.
    //     Devuelve false si el sensor no contesta: cable suelto, sin
    //     alimentacion, o XSHUT mal conectado.
    if (!sensor[i].init()) {

      sensorOK[i] = false;

      Serial.print(F("      Sensor "));
      Serial.print(i + 1);
      Serial.print(F(" ("));
      Serial.print(NOMBRE_SENSOR[i]);
      Serial.println(F(") -> NO RESPONDE"));
      Serial.println(F("        Revisa: alimentacion 3.3 V, SDA, SCL y su cable XSHUT."));

      continue;   // Pasamos al siguiente sin tocar su direccion
    }

    // (c) Le damos su direccion definitiva.
    //     A partir de esta linea, este sensor ya no responde en 0x29.
    uint8_t direccionNueva = DIRECCION_BASE + i;
    sensor[i].setAddress(direccionNueva);

    // Configuracion de la medicion:
    //   Long   = alcance largo (hasta ~4 m). Las otras opciones son Short y Medium.
    //            Short tiene mas inmunidad a la luz ambiente pero menos alcance.
    sensor[i].setDistanceMode(VL53L1X::Long);
    sensor[i].setMeasurementTimingBudget(TIEMPO_MEDIDA_US);

    // Modo continuo: el sensor mide solo, sin que haya que pedirselo cada vez.
    // Asi el bucle principal solo recoge el ultimo valor y no se bloquea.
    sensor[i].startContinuous(PERIODO_MS);

    sensorOK[i] = true;

    Serial.print(F("      Sensor "));
    Serial.print(i + 1);
    Serial.print(F(" ("));
    Serial.print(NOMBRE_SENSOR[i]);
    Serial.print(F(") -> OK, ahora en 0x"));
    Serial.println(direccionNueva, HEX);
  }

  Serial.println();


  // ---- 4.5 Resumen ----

  uint8_t buenos = 0;
  for (uint8_t i = 0; i < NUM_SENSORES; i++) {
    if (sensorOK[i]) buenos++;
  }

  Serial.print(F("[4/4] Listos "));
  Serial.print(buenos);
  Serial.print(F(" de "));
  Serial.print(NUM_SENSORES);
  Serial.println(F(" sensores."));
  Serial.println();

  if (buenos == 0) {
    Serial.println(F("  NINGUN SENSOR RESPONDIO. Lo mas probable:"));
    Serial.println(F("   - SDA y SCL intercambiados."));
    Serial.println(F("   - Faltan los pull-ups de 4.7 k a 3.3 V."));
    Serial.println(F("   - Los sensores no tienen alimentacion."));
    Serial.println(F("   - Masa no compartida entre la placa y los sensores."));
  }
  else if (buenos < NUM_SENSORES) {
    Serial.println(F("  Algunos no respondieron. Si fallan siempre LOS MISMOS,"));
    Serial.println(F("  el problema esta en su cableado. Si falla uno distinto"));
    Serial.println(F("  en cada arranque, sospecha de los pull-ups: con varios"));
    Serial.println(F("  modulos en paralelo el bus deja de cumplir sus tiempos."));
    Serial.println(F("  Hay que desoldarlos de todos menos uno."));
  }

  Serial.println();
  Serial.println(F("Distancias en milimetros. 'T' = timeout (sin respuesta)."));
  Serial.println(F("Fuera de alcance el sensor reporta un valor alto, no cero."));
  Serial.println();

  delay(1000);
}


/* ============================================================================
 * SECCION 5 — BUCLE PRINCIPAL
 * ============================================================================
 * Solo lee y muestra. Aqui todavia no hay logica de combate: esta prueba
 * sirve para confirmar que los cinco sensores ven lo que deben ver.
 *
 * QUE DEBES COMPROBAR:
 *   - Pon la mano a 10 cm frente a un sensor: debe marcar cerca de 100.
 *   - Muevela a 30 cm: debe marcar cerca de 300.
 *   - Tapa un sensor con el dedo: solo ESE debe cambiar. Si cambian dos,
 *     tienes dos sensores con la misma direccion.
 *   - Prueba con algo negro mate: el alcance baja bastante. Es normal, y es
 *     la razon por la que el INA219 sirve de respaldo para detectar contacto.
 * ========================================================================= */

void loop() {

  for (uint8_t i = 0; i < NUM_SENSORES; i++) {

    Serial.print(NOMBRE_SENSOR[i]);
    Serial.print(F(": "));

    if (!sensorOK[i]) {
      Serial.print(F("----"));          // Este sensor no arranco
    }
    else {
      // read(false) devuelve la ultima medicion sin esperar a que termine
      // una nueva. Como el sensor esta en modo continuo, siempre hay un
      // valor fresco disponible.
      uint16_t mm = sensor[i].read(false);

      if (sensor[i].timeoutOccurred()) {
        Serial.print(F("T"));           // El sensor dejo de contestar
      } else {
        Serial.print(mm);
      }
    }

    Serial.print(F("\t"));              // Tabulador, para que quede en columnas
  }

  Serial.println();

  delay(100);   // 10 veces por segundo: suficiente para leerlo con los ojos.
                // En el firmware de combate esto no lleva delay y el lazo
                // corre a 200 Hz.
}
