// =====================================================================
//  Mini Sumo - ESP32 + OLED SSD1306
//  Menú en pantalla controlado solo con el botón BOOT:
//    pulsación corta = siguiente opción, pulsación larga = seleccionar
//  El Monitor Serie (115200) sigue funcionando como respaldo.
//  Librerías: "Adafruit SSD1306" y "Adafruit GFX"
// =====================================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "esp_arduino_version.h"
#include "config.h"

#define ES_TOF (SENSOR_OPONENTE == SENSOR_VL53L0X || SENSOR_OPONENTE == SENSOR_VL53L1X)
#if SENSOR_OPONENTE == SENSOR_VL53L0X
#include <VL53L0X.h>
VL53L0X tof;
#elif SENSOR_OPONENTE == SENSOR_VL53L1X
#include <VL53L1X.h>
VL53L1X tof;
#endif

enum Lado { IZQ, DER };
enum SensorLinea { DEL_IZQ, DEL_DER, TRA_IZQ, TRA_DER, N_LINEA };
enum Pulso { NINGUNO, CORTO, LARGO };
enum Estado { BUSCAR, ATACAR, ESCAPE_ATRAS, ESCAPE_GIRO, ESCAPE_ADELANTE, PATRULLAR };

const uint8_t pinesLinea[N_LINEA] = {
  PIN_LINEA_DEL_IZQ, PIN_LINEA_DEL_DER, PIN_LINEA_TRA_IZQ, PIN_LINEA_TRA_DER
};
const char* nombresLinea[N_LINEA] = { "DelIzq", "DelDer", "TraIzq", "TraDer" };
const char* siglasLinea[N_LINEA]  = { "DI", "DD", "TI", "TD" };
int umbralLinea[N_LINEA];

Preferences prefs;
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool hayPantalla = false;
bool hayToF = false;
uint16_t distanciaMM = 0;   // última distancia válida del ToF (0 = nada en rango)

#define BLANCO SSD1306_WHITE
#define NEGRO  SSD1306_BLACK

// =====================================================================
//  Motores
// =====================================================================
void pwmInit(uint8_t pin, uint8_t canal) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)canal;
  ledcAttach(pin, PWM_FREQ, PWM_RES);
#else
  ledcSetup(canal, PWM_FREQ, PWM_RES);
  ledcAttachPin(pin, canal);
#endif
}

void pwmWrite(uint8_t pin, uint8_t canal, uint32_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)canal;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(canal, duty);
#endif
}

void motoresInit() {
  pinMode(PIN_IZQ_IN1, OUTPUT); pinMode(PIN_IZQ_IN2, OUTPUT);
  pinMode(PIN_DER_IN1, OUTPUT); pinMode(PIN_DER_IN2, OUTPUT);
  pwmInit(PIN_IZQ_PWM, 0);
  pwmInit(PIN_DER_PWM, 1);
#if PUENTE_H == DRIVER_TB6612
  pinMode(PIN_STBY, OUTPUT);
  digitalWrite(PIN_STBY, HIGH);
#endif
}

// vel: -255 (atrás) ... 0 (freno) ... 255 (adelante)
void motor(Lado lado, int vel) {
  vel = constrain(vel, -255, 255);
  bool invertir = (lado == IZQ) ? INVERTIR_IZQ : INVERTIR_DER;
  if (invertir) vel = -vel;

  uint8_t in1 = (lado == IZQ) ? PIN_IZQ_IN1 : PIN_DER_IN1;
  uint8_t in2 = (lado == IZQ) ? PIN_IZQ_IN2 : PIN_DER_IN2;
  uint8_t pwm = (lado == IZQ) ? PIN_IZQ_PWM : PIN_DER_PWM;

  if (vel > 0)      { digitalWrite(in1, HIGH); digitalWrite(in2, LOW);  }
  else if (vel < 0) { digitalWrite(in1, LOW);  digitalWrite(in2, HIGH); }
  else              { digitalWrite(in1, HIGH); digitalWrite(in2, HIGH); } // freno
  pwmWrite(pwm, lado == IZQ ? 0 : 1, vel == 0 ? 255 : abs(vel));
}

void mover(int izq, int der) { motor(IZQ, izq); motor(DER, der); }
void parar() { mover(0, 0); }

// =====================================================================
//  Sensores
// =====================================================================
int leerLineaCruda(uint8_t i) {
  long suma = 0;
#if LINEA_DIGITAL
  return digitalRead(pinesLinea[i]) ? 4095 : 0;
#else
  for (uint8_t k = 0; k < 4; k++) suma += analogRead(pinesLinea[i]);
  return suma / 4;
#endif
}

bool esBlanco(uint8_t i) {
  int v = leerLineaCruda(i);
  return BLANCO_ES_BAJO ? (v < umbralLinea[i]) : (v > umbralLinea[i]);
}

// ToF: arranca la medición continua (cada 20 ms). Requiere Wire ya iniciado.
void oponenteInit() {
#if SENSOR_OPONENTE == SENSOR_DIGITAL
  pinMode(PIN_IR_OPONENTE, INPUT);
#elif SENSOR_OPONENTE == SENSOR_VL53L0X
  tof.setTimeout(50);
  hayToF = tof.init();
  if (hayToF) {
    tof.setMeasurementTimingBudget(20000);
    tof.startContinuous();
  }
#elif SENSOR_OPONENTE == SENSOR_VL53L1X
  tof.setTimeout(50);
  hayToF = tof.init();
  if (hayToF) {
    tof.setDistanceMode(VL53L1X::Short);
    tof.setMeasurementTimingBudget(20000);
    tof.startContinuous(20);
  }
#endif
#if ES_TOF
  if (!hayToF) Serial.println("AVISO: no se encontro el sensor ToF (I2C 0x29). Revisa SDA=21, SCL=22.");
#endif
}

// Lee el ToF solo si hay un dato nuevo: nunca bloquea el bucle de pelea.
void actualizarToF() {
#if SENSOR_OPONENTE == SENSOR_VL53L0X
  if (!hayToF) return;
  if ((tof.readReg(VL53L0X::RESULT_INTERRUPT_STATUS) & 0x07) == 0) return;
  uint16_t mm = tof.readRangeContinuousMillimeters();
  distanciaMM = (tof.timeoutOccurred() || mm >= 8000) ? 0 : mm;
#elif SENSOR_OPONENTE == SENSOR_VL53L1X
  if (!hayToF || !tof.dataReady()) return;
  tof.read(false);
  distanciaMM = (tof.ranging_data.range_status == VL53L1X::RangeValid) ? tof.ranging_data.range_mm : 0;
#endif
}

// Sharp: 0-4095 · digital: 0/1 · ToF: milímetros (0 = nada en rango)
int leerIRCrudo() {
#if SENSOR_OPONENTE == SENSOR_SHARP
  return analogRead(PIN_IR_OPONENTE);
#elif SENSOR_OPONENTE == SENSOR_DIGITAL
  return digitalRead(PIN_IR_OPONENTE);
#else
  actualizarToF();
  return distanciaMM;
#endif
}

// Lectura instantánea, sin filtrar
bool hayOponente() {
#if SENSOR_OPONENTE == SENSOR_SHARP
  return leerIRCrudo() > UMBRAL_IR;
#elif SENSOR_OPONENTE == SENSOR_DIGITAL
  return IR_ACTIVO_BAJO ? (leerIRCrudo() == LOW) : (leerIRCrudo() == HIGH);
#else
  int mm = leerIRCrudo();
  return mm > 0 && mm < DISTANCIA_ATAQUE_MM;
#endif
}

void cargarUmbrales() {
  prefs.begin("sumo", true);
  for (uint8_t i = 0; i < N_LINEA; i++) {
    char clave[4] = { 'u', char('0' + i), 0 };
    umbralLinea[i] = prefs.getInt(clave, UMBRAL_LINEA_DEFECTO);
  }
  prefs.end();
}

void guardarUmbrales() {
  prefs.begin("sumo", false);
  for (uint8_t i = 0; i < N_LINEA; i++) {
    char clave[4] = { 'u', char('0' + i), 0 };
    prefs.putInt(clave, umbralLinea[i]);
  }
  prefs.end();
}

// Voltaje de la batería en volts (0 si no hay divisor/batería, p. ej. solo USB).
// Se promedia y se actualiza como máximo cada 500 ms.
float voltajeBateria() {
#if USAR_BATERIA
  static float v = 0;
  static uint32_t tLectura = 0;
  if (tLectura == 0 || millis() - tLectura > 500) {
    uint32_t mv = 0;
    for (uint8_t k = 0; k < 16; k++) mv += analogReadMilliVolts(PIN_BATERIA);
    float leido = (mv / 16.0f / 1000.0f) * (BAT_R_ARRIBA + BAT_R_ABAJO) / BAT_R_ABAJO * BAT_AJUSTE;
    v = (leido < 2.0f) ? 0 : (v == 0 ? leido : v * 0.7f + leido * 0.3f);
    tLectura = millis();
  }
  return v;
#else
  return 0;
#endif
}

int porcentajeBateria(float v) {
  return constrain((int)((v - BAT_VACIA) / (BAT_LLENA - BAT_VACIA) * 100), 0, 100);
}

bool bateriaBaja() {
  float v = voltajeBateria();
  return v > 0 && v < BAT_BAJA;
}

// =====================================================================
//  Botón y utilidades
// =====================================================================
bool botonPresionado() { return digitalRead(PIN_BOTON) == LOW; }

void esperarSoltarBoton() {
  while (botonPresionado()) delay(10);
  delay(50);
}

// Bloquea mientras el botón está presionado (solo se usa en el menú)
Pulso leerBoton() {
  if (!botonPresionado()) return NINGUNO;
  delay(20);
  if (!botonPresionado()) return NINGUNO;   // rebote
  uint32_t t0 = millis();
  while (botonPresionado()) {
    if (millis() - t0 > T_PULSO_LARGO_MS) {
      digitalWrite(PIN_LED, HIGH);            // aviso: ya es pulsación larga
      if (hayPantalla) oled.invertDisplay(true);
      esperarSoltarBoton();
      if (hayPantalla) oled.invertDisplay(false);
      digitalWrite(PIN_LED, LOW);
      return LARGO;
    }
    delay(5);
  }
  delay(30);
  return CORTO;
}

void vaciarSerial() { while (Serial.available()) Serial.read(); }

// Cualquier tecla en el monitor o el botón BOOT detiene la prueba
bool abortar() {
  if (Serial.available()) { vaciarSerial(); return true; }
  if (botonPresionado()) { esperarSoltarBoton(); return true; }
  return false;
}

// delay que se puede interrumpir; devuelve true si se abortó
bool esperar(uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    if (abortar()) return true;
    delay(5);
  }
  return false;
}

// Espera un "OK" (botón o Enter en el monitor)
void esperarOK() {
  vaciarSerial();
  while (!Serial.available() && !botonPresionado()) delay(10);
  vaciarSerial();
  esperarSoltarBoton();
}

// =====================================================================
//  Pantalla
// =====================================================================
void pantallaInit() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  hayPantalla = oled.begin(SSD1306_SWITCHCAPVCC, OLED_DIRECCION);
  if (!hayPantalla) {
    Serial.println("AVISO: no se encontro el OLED. Revisa SDA=21, SCL=22 y la direccion (0x3C/0x3D).");
    return;
  }
  oled.setTextWrap(false);
}

void textoCentrado(const char* t, uint8_t tam, int16_t y) {
  int16_t ancho = strlen(t) * 6 * tam - tam;
  oled.setTextSize(tam);
  oled.setCursor((128 - ancho) / 2, y);
  oled.print(t);
}

// Icono de pila (esquina superior derecha) con su voltaje.
// Con batería baja parpadea; sin divisor conectado muestra "USB".
void dibujarBateria() {
  float v = voltajeBateria();
  oled.setTextSize(1);
  oled.setTextColor(BLANCO);
  if (v == 0) {
    oled.setCursor(110, 0);
    oled.print("USB");
    return;
  }
  bool baja = v < BAT_BAJA;
  if (baja && (millis() / 400) % 2) return;   // parpadeo
  char buf[6];
  snprintf(buf, sizeof(buf), "%.1fV", v);
  oled.setCursor(128 - 17 - 4 - strlen(buf) * 6, 0);
  oled.print(buf);
  // cuerpo 15x8 + borne 2x4
  oled.drawRect(111, 0, 15, 8, BLANCO);
  oled.fillRect(126, 2, 2, 4, BLANCO);
  int relleno = porcentajeBateria(v) * 11 / 100;
  oled.fillRect(113, 2, max(relleno, 1), 4, BLANCO);
}

void encabezado(const char* titulo) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(BLANCO);
  oled.setCursor(0, 0);
  oled.print(titulo);
  dibujarBateria();
  oled.drawFastHLine(0, 10, 128, BLANCO);
}

// Ayuda de controles, centrada y separada por una línea punteada
void pie(const char* texto) {
  for (int16_t x = 0; x < 128; x += 4) oled.drawPixel(x, 54, BLANCO);
  oled.setTextColor(BLANCO);
  textoCentrado(texto, 1, 56);
}

// Pantalla genérica de texto: título + hasta 3 líneas + pie
void pantallaTexto(const char* titulo, const char* l1, const char* l2,
                   const char* l3, const char* textoPie) {
  if (!hayPantalla) return;
  encabezado(titulo);
  oled.setTextSize(1);
  oled.setCursor(0, 15); oled.print(l1);
  oled.setCursor(0, 27); oled.print(l2);
  oled.setCursor(0, 39); oled.print(l3);
  pie(textoPie);
  oled.display();
}

// Dibuja el robot visto desde arriba con sus 4 sensores de línea
// (círculo relleno = ve blanco) y el sensor IR al frente.
void dibujarRobot(int16_t cx, int16_t cy, bool linea[N_LINEA], bool oponente) {
  oled.drawRect(cx - 14, cy - 12, 28, 24, BLANCO);
  const int px[N_LINEA] = { cx - 9, cx + 9, cx - 9, cx + 9 };
  const int py[N_LINEA] = { cy - 7, cy - 7, cy + 7, cy + 7 };
  for (uint8_t i = 0; i < N_LINEA; i++) {
    if (linea[i]) oled.fillCircle(px[i], py[i], 3, BLANCO);
    else          oled.drawCircle(px[i], py[i], 3, BLANCO);
  }
  // IR: triángulo al frente
  if (oponente) oled.fillTriangle(cx - 5, cy - 14, cx + 5, cy - 14, cx, cy - 20, BLANCO);
  else          oled.drawTriangle(cx - 5, cy - 14, cx + 5, cy - 14, cx, cy - 20, BLANCO);
}

// =====================================================================
//  Pruebas individuales
// =====================================================================
void pantallaMotor(const char* titulo, int vel) {
  if (!hayPantalla) return;
  encabezado(titulo);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", vel);
  textoCentrado(buf, 3, 16);
  textoCentrado(vel > 0 ? "ADELANTE" : vel < 0 ? "ATRAS" : "FRENO", 1, 44);
  pie("BOOT: salir");
  oled.display();
}

void pruebaMotor(Lado lado) {
  const char* titulo = (lado == IZQ) ? "MOTOR IZQ" : "MOTOR DER";
  Serial.printf("\n[%s] Ruedas AL AIRE. Tecla o BOOT para salir.\n", titulo);
  pantallaTexto(titulo, "Ruedas AL AIRE", "Inicia en 2 s...", "", "BOOT: salir");
  if (esperar(2000)) { parar(); return; }

  const int pasos[] = { 120, 255, 0, -120, -255, 0 };
  for (int v : pasos) {
    Serial.printf("  vel = %4d  %s\n", v, v > 0 ? "(debe ir ADELANTE)" : v < 0 ? "(debe ir ATRAS)" : "(freno)");
    pantallaMotor(titulo, v);
    motor(lado, v);
    if (esperar(1500)) break;
  }
  parar();
  Serial.println("  Si giro al reves, cambia INVERTIR_IZQ / INVERTIR_DER en config.h");
  pantallaTexto(titulo, "Termino.", "Giro al reves? cambia", "INVERTIR_ en config.h", "BOOT: volver");
  esperarOK();
}

void pruebaMovimientos() {
  Serial.println("\n[Movimientos] Tecla o BOOT para salir.");
  pantallaTexto("MOVIMIENTOS", "Ruedas al aire o", "piso con espacio.", "Inicia en 2 s...", "BOOT: salir");
  if (esperar(2000)) { parar(); return; }

  struct { const char* nombre; int izq, der; } movs[] = {
    { "ADELANTE",   180,  180 },
    { "ATRAS",     -180, -180 },
    { "GIRO IZQ",  -180,  180 },
    { "GIRO DER",   180, -180 },
    { "ARCO IZQ",   100,  220 },
    { "ARCO DER",   220,  100 },
  };
  for (auto& m : movs) {
    Serial.printf("  %s\n", m.nombre);
    if (hayPantalla) {
      encabezado("MOVIMIENTOS");
      textoCentrado(m.nombre, 2, 24);
      pie("BOOT: salir");
      oled.display();
    }
    mover(m.izq, m.der);
    if (esperar(1200)) break;
    parar();
    if (esperar(400)) break;
  }
  parar();
}

void pruebaLinea() {
  Serial.println("\n[Sensores de linea] Valores 0-4095. Tecla o BOOT para salir.");
  Serial.printf("  Umbrales: %d %d %d %d\n", umbralLinea[0], umbralLinea[1], umbralLinea[2], umbralLinea[3]);
  while (!abortar()) {
    int val[N_LINEA];
    bool blanco[N_LINEA];
    for (uint8_t i = 0; i < N_LINEA; i++) {
      val[i] = leerLineaCruda(i);
      blanco[i] = BLANCO_ES_BAJO ? (val[i] < umbralLinea[i]) : (val[i] > umbralLinea[i]);
      Serial.printf("%s:%4d %-6s  ", nombresLinea[i], val[i], blanco[i] ? "BLANCO" : "negro");
    }
    Serial.println();

    if (hayPantalla) {
      encabezado("LINEA");
      dibujarRobot(64, 34, blanco, false);
      oled.setCursor(0, 20);  oled.print(val[DEL_IZQ]);
      oled.setCursor(98, 20); oled.print(val[DEL_DER]);
      oled.setCursor(0, 42);  oled.print(val[TRA_IZQ]);
      oled.setCursor(98, 42); oled.print(val[TRA_DER]);
      pie("BOOT: salir");
      oled.display();
    }
    delay(120);
  }
}

void pruebaIR() {
  Serial.println("\n[Sensor oponente] Acerca y aleja un objeto al frente. Tecla o BOOT para salir.");
#if ES_TOF
  const int escalaMax = 1000;           // barra de 0 a 1000 mm
  const int umbral = DISTANCIA_ATAQUE_MM;
  const char* unidad = " mm";
#else
  const int escalaMax = 4095;
  const int umbral = UMBRAL_IR;
  const char* unidad = "";
#endif
  while (!abortar()) {
    int lectura = leerIRCrudo();
    bool ve = hayOponente();
    digitalWrite(PIN_LED, ve);
    Serial.printf("  lectura: %4d%s   %s\n", lectura, unidad, ve ? "<<< OPONENTE >>>" : "-");

    if (hayPantalla) {
      encabezado("OPONENTE");
      char buf[12];
#if ES_TOF
      if (!hayToF)          snprintf(buf, sizeof(buf), "SIN ToF");
      else if (lectura == 0) snprintf(buf, sizeof(buf), "---");
      else                  snprintf(buf, sizeof(buf), "%dmm", lectura);
      // ToF: barra llena = cerca
      int nivel = lectura == 0 ? 0 : escalaMax - constrain(lectura, 0, escalaMax);
      int marca = map(escalaMax - umbral, 0, escalaMax, 0, 126);
#elif SENSOR_OPONENTE == SENSOR_DIGITAL
      snprintf(buf, sizeof(buf), "%s", ve ? "SI" : "NO");
      int nivel = ve ? escalaMax : 0;
      int marca = 63;
#else
      snprintf(buf, sizeof(buf), "%d", lectura);
      int nivel = constrain(lectura, 0, escalaMax);
      int marca = map(umbral, 0, escalaMax, 0, 126);
#endif
      textoCentrado(buf, 2, 14);
      oled.drawRect(0, 34, 128, 8, BLANCO);
      oled.fillRect(1, 35, map(nivel, 0, escalaMax, 0, 126), 6, BLANCO);
      oled.drawFastVLine(1 + marca, 31, 14, BLANCO);
      if (ve) textoCentrado("OPONENTE!", 1, 45);
      pie("BOOT: salir");
      oled.display();
    }
    delay(ES_TOF ? 30 : 120);
  }
  digitalWrite(PIN_LED, LOW);
}

int promedioLinea(uint8_t i) {
  long suma = 0;
  for (uint8_t k = 0; k < 20; k++) { suma += leerLineaCruda(i); delay(5); }
  return suma / 20;
}

void calibrarLinea() {
  int negro[N_LINEA], blanco[N_LINEA];
  Serial.println("\n[Calibracion] Sensores sobre NEGRO y presiona Enter o BOOT.");
  pantallaTexto("CALIBRAR", "Paso 1 de 2:", "4 sensores sobre", "NEGRO", "BOOT: leer");
  esperarOK();
  for (uint8_t i = 0; i < N_LINEA; i++) negro[i] = promedioLinea(i);

  Serial.println("  Sensores sobre el BORDE BLANCO y presiona Enter o BOOT.");
  pantallaTexto("CALIBRAR", "Paso 2 de 2:", "4 sensores sobre", "BLANCO (borde)", "BOOT: leer");
  esperarOK();
  for (uint8_t i = 0; i < N_LINEA; i++) blanco[i] = promedioLinea(i);

  bool ok = true;
  if (hayPantalla) {
    encabezado("CALIBRADO");
    oled.setCursor(0, 13);
    oled.print("    NEGR BLAN UMBR");
  }
  for (uint8_t i = 0; i < N_LINEA; i++) {
    umbralLinea[i] = (negro[i] + blanco[i]) / 2;
    bool malo = abs(negro[i] - blanco[i]) < 300;
    if (malo) ok = false;
    Serial.printf("  %s  negro=%4d  blanco=%4d  umbral=%4d %s\n", nombresLinea[i],
                  negro[i], blanco[i], umbralLinea[i], malo ? "<- POCA DIFERENCIA, revisa altura/cableado" : "");
    if (hayPantalla) {
      oled.setCursor(0, 22 + i * 8);
      oled.printf("%s%c %4d %4d %4d", siglasLinea[i], malo ? '!' : ' ', negro[i], blanco[i], umbralLinea[i]);
    }
  }
  guardarUmbrales();
  Serial.println(ok ? "  Umbrales guardados." : "  Umbrales guardados, pero revisa los sensores marcados.");
  if (hayPantalla) {
    pie(ok ? "Guardado  BOOT:volver" : "! revisar BOOT:volver");
    oled.display();
  }
  esperarOK();
}

// =====================================================================
//  Lógica de pelea (usada por la prueba en conjunto y por combate)
// =====================================================================
const char* nombresEstado[] = { "BUSCAR", "ATACAR", "ESCAPE_ATRAS", "ESCAPE_GIRO", "ESCAPE_ADELANTE", "PATRULLAR" };
const char* nombresCortos[] = { "BUSCAR", "ATACAR", "ATRAS", "GIRO", "AVANZA", "RONDA" };

void pantallaPelea(const char* titulo, Estado estado, bool linea[N_LINEA], bool oponente, uint32_t ms) {
  if (!hayPantalla) return;
  encabezado(titulo);
  dibujarRobot(22, 36, linea, oponente);
  oled.setTextSize(2);
  oled.setCursor(44, 22);
  oled.print(nombresCortos[estado]);
  oled.setTextSize(1);
  oled.setCursor(44, 44);
  oled.printf("%lu.%lu s", (unsigned long)(ms / 1000), (unsigned long)((ms / 100) % 10));
  pie("BOOT: detener");
  oled.display();
}

void pelea(float factor, uint32_t esperaInicio, bool actualizarPantalla) {
  auto v = [factor](int vel) { return (int)(vel * factor); };
  const char* titulo = factor < 1.0f ? "CONJUNTO" : "COMBATE";

  Serial.printf("\n[%s] Arranca en %lu ms. Tecla o BOOT para detener.\n", titulo, (unsigned long)esperaInicio);
  esperarSoltarBoton();

  // Cuenta regresiva: LED parpadeando y número grande en pantalla
  uint32_t t0 = millis();
  int ultimoSeg = -1;
  while (millis() - t0 < esperaInicio) {
    uint32_t transcurrido = millis() - t0;
    digitalWrite(PIN_LED, (transcurrido / 250) % 2);
    int seg = (esperaInicio - transcurrido + 999) / 1000;
    if (seg != ultimoSeg && hayPantalla) {
      ultimoSeg = seg;
      encabezado(titulo);
      char buf[4];
      snprintf(buf, sizeof(buf), "%d", seg);
      textoCentrado(buf, 4, 18);
      pie("BOOT: cancelar");
      oled.display();
    }
    if (abortar()) {
      digitalWrite(PIN_LED, LOW);
      Serial.println("  Cancelado.");
      return;
    }
    delay(5);
  }
  digitalWrite(PIN_LED, HIGH);

  if (!actualizarPantalla && hayPantalla) {
    encabezado(titulo);
    textoCentrado("A PELEAR!", 2, 24);
    pie("BOOT: detener");
    oled.display();
  }

  Estado estado = BUSCAR, anterior = PATRULLAR;
  uint32_t inicio = millis(), tEstado = inicio, tBusqueda = inicio, tPantalla = 0;
  uint32_t tVistoDesde = 0;          // desde cuándo lo ve sin interrupción (0 = no lo ve)
  uint32_t tUltimoConfirmado = 0;    // última vez que el oponente estaba confirmado
  uint32_t tUltimoContacto = inicio; // para decidir cuándo patrullar
  uint8_t rachaBlanco[N_LINEA] = { 0 };
  int sentidoBusqueda = 1;   // 1 = gira a la derecha, -1 = izquierda
  int sentidoEscape = 1;

  while (!abortar()) {
    uint32_t ahora = millis();

    // Línea filtrada: "blanco" solo si se repite CONFIRMA_LINEA veces seguidas
    bool linea[N_LINEA];
    for (uint8_t i = 0; i < N_LINEA; i++) {
      rachaBlanco[i] = esBlanco(i) ? min(rachaBlanco[i] + 1, 255) : 0;
      linea[i] = rachaBlanco[i] >= CONFIRMA_LINEA;
    }
    bool di = linea[DEL_IZQ], dd = linea[DEL_DER], ti = linea[TRA_IZQ], td = linea[TRA_DER];

    // Oponente filtrado: confirmado tras T_CONFIRMA y recordado T_MEMORIA
    if (hayOponente()) {
      if (tVistoDesde == 0) tVistoDesde = ahora;
      if (ahora - tVistoDesde >= T_CONFIRMA_OPONENTE_MS) tUltimoConfirmado = ahora;
    } else {
      tVistoDesde = 0;
    }
    bool oponente = tUltimoConfirmado != 0 && ahora - tUltimoConfirmado < T_MEMORIA_OPONENTE_MS;
    if (oponente) tUltimoContacto = ahora;

    // 1) Prioridad máxima: no salir del dohyo
    if ((di || dd) && estado != ESCAPE_ATRAS && estado != ESCAPE_GIRO) {
      // borde a la izquierda -> girar a la derecha y viceversa
      sentidoEscape = (di && !dd) ? 1 : (dd && !di) ? -1 : sentidoBusqueda;
      estado = ESCAPE_ATRAS; tEstado = ahora; tUltimoContacto = ahora;
    } else if ((ti || td) && estado != ESCAPE_ADELANTE) {
      estado = ESCAPE_ADELANTE; tEstado = ahora; tUltimoContacto = ahora;
    }

    // 2) Ejecutar estado
    switch (estado) {
      case ESCAPE_ATRAS:
        mover(-v(VEL_ESCAPE), -v(VEL_ESCAPE));
        if (ahora - tEstado > T_RETROCESO_MS) { estado = ESCAPE_GIRO; tEstado = ahora; }
        break;

      case ESCAPE_GIRO:
        mover(v(VEL_ESCAPE) * sentidoEscape, -v(VEL_ESCAPE) * sentidoEscape);
        if (oponente || ahora - tEstado > T_GIRO_ESCAPE_MS) { estado = BUSCAR; tEstado = ahora; }
        break;

      case ESCAPE_ADELANTE:
        mover(v(VEL_ESCAPE), v(VEL_ESCAPE));
        if (ahora - tEstado > T_RETROCESO_MS) { estado = BUSCAR; tEstado = ahora; }
        break;

      case PATRULLAR:
        // avanza en arco para cambiar de posición y buscar desde otro ángulo
        mover(v(VEL_BUSQUEDA), v(VEL_BUSQUEDA) * 2 / 3);
        if (oponente) { estado = ATACAR; tEstado = ahora; }
        else if (ahora - tEstado > T_PATRULLA_MS) {
          estado = BUSCAR; tEstado = ahora; tUltimoContacto = ahora;
        }
        break;

      case BUSCAR:
      case ATACAR:
        if (oponente) {
          estado = ATACAR;
          mover(v(VEL_ATAQUE), v(VEL_ATAQUE));
        } else if (ahora - tUltimoContacto > T_SIN_OPONENTE_MS) {
          estado = PATRULLAR; tEstado = ahora;
        } else {
          estado = BUSCAR;
          if (ahora - tBusqueda > T_CAMBIO_BUSQUEDA) { sentidoBusqueda = -sentidoBusqueda; tBusqueda = ahora; }
          mover(v(VEL_BUSQUEDA) * sentidoBusqueda, -v(VEL_BUSQUEDA) * sentidoBusqueda);
        }
        break;
    }

    if (estado != anterior) {
      Serial.printf("  %6lu ms  %-16s linea[%d%d%d%d] IR=%d\n", (unsigned long)(ahora - inicio),
                    nombresEstado[estado], di, dd, ti, td, leerIRCrudo());
      anterior = estado;
    }
    if (actualizarPantalla && ahora - tPantalla > REFRESCO_PANTALLA_MS) {
      pantallaPelea(titulo, estado, linea, oponente, ahora - inicio);
      tPantalla = ahora;
    }
    delay(2);
  }

  parar();
  digitalWrite(PIN_LED, LOW);
  uint32_t duracion = millis() - inicio;
  Serial.printf("  Detenido tras %lu ms.\n", (unsigned long)duracion);
  if (hayPantalla) {
    char buf[22];
    snprintf(buf, sizeof(buf), "Duro %lu.%lu s", (unsigned long)(duracion / 1000), (unsigned long)((duracion / 100) % 10));
    pantallaTexto(titulo, "Detenido.", buf, "", "BOOT: volver");
    esperarOK();
  }
}

// =====================================================================
//  Batería (pantalla de prueba)
// =====================================================================
void pruebaBateria() {
  Serial.println("\n[Bateria] Tecla o BOOT para salir.");
  while (!abortar()) {
    float v = voltajeBateria();
    Serial.printf("  %.2f V  (%.2f V por celda)  %d%%\n", v, v / 2, porcentajeBateria(v));
    if (hayPantalla) {
      encabezado("BATERIA");
      if (v == 0) {
        textoCentrado("Sin lectura", 1, 20);
        textoCentrado("Revisa el divisor", 1, 32);
        textoCentrado("en GPIO33", 1, 42);
      } else {
        char buf[12];
        snprintf(buf, sizeof(buf), "%.2fV", v);
        textoCentrado(buf, 2, 15);
        int pct = porcentajeBateria(v);
        oled.drawRoundRect(4, 34, 120, 9, 2, BLANCO);
        oled.fillRoundRect(6, 36, 116 * pct / 100, 5, 1, BLANCO);
        snprintf(buf, sizeof(buf), "%d%%", pct);
        oled.setTextSize(1);
        oled.setCursor(4, 45);  oled.print(buf);
        snprintf(buf, sizeof(buf), "%.2fV/c", v / 2);
        oled.setCursor(124 - strlen(buf) * 6 + 1, 45); oled.print(buf);
        if (v < BAT_BAJA && (millis() / 400) % 2) {
          oled.fillRect(0, 13, 128, 40, NEGRO);
          textoCentrado("CARGAR", 2, 25);
        }
      }
      pie("BOOT: salir");
      oled.display();
    }
    delay(200);
  }
}

// =====================================================================
//  Menú
// =====================================================================
const char* opcionesMenu[] = {
  "COMBATE",
  "Prueba en conjunto",
  "Motor izquierdo",
  "Motor derecho",
  "Movimientos",
  "Sensores de linea",
  "Sensor oponente",
  "Calibrar linea",
  "Bateria",
};
const uint8_t N_MENU = sizeof(opcionesMenu) / sizeof(opcionesMenu[0]);
const uint8_t FILAS_VISIBLES = 3;
const int16_t MENU_Y0 = 14, MENU_ALTO_FILA = 13;
uint8_t seleccion = 0;   // al encender queda en COMBATE: pulsación larga y pelea

void ejecutar(uint8_t opcion) {
  switch (opcion) {
    case 0: pelea(1.0f, ESPERA_INICIO_MS, false); break;
    case 1: pelea(FACTOR_VEL_PRUEBA, ESPERA_PRUEBA_MS, true); break;
    case 2: pruebaMotor(IZQ); break;
    case 3: pruebaMotor(DER); break;
    case 4: pruebaMovimientos(); break;
    case 5: pruebaLinea(); break;
    case 6: pruebaIR(); break;
    case 7: calibrarLinea(); break;
    case 8: pruebaBateria(); break;
  }
  parar();
}

// 3 opciones visibles, la seleccionada en una barra redondeada,
// y una barra de desplazamiento a la derecha que indica la posición.
void dibujarMenu() {
  if (!hayPantalla) return;
  encabezado("MINI SUMO");

  uint8_t primero = constrain((int)seleccion - 1, 0, N_MENU - FILAS_VISIBLES);
  for (uint8_t f = 0; f < FILAS_VISIBLES; f++) {
    uint8_t idx = primero + f;
    int16_t y = MENU_Y0 + f * MENU_ALTO_FILA;
    if (idx == seleccion) {
      oled.fillRoundRect(0, y, 120, MENU_ALTO_FILA - 1, 3, BLANCO);
      oled.setTextColor(NEGRO);
    } else {
      oled.setTextColor(BLANCO);
    }
    oled.setTextSize(1);
    oled.setCursor(6, y + 2);
    oled.print(opcionesMenu[idx]);
  }

  int16_t altoPista = FILAS_VISIBLES * MENU_ALTO_FILA - 1;
  for (int16_t y = MENU_Y0; y < MENU_Y0 + altoPista; y += 2) oled.drawPixel(125, y, BLANCO);
  int16_t altoBarra = max(4, altoPista / N_MENU);
  int16_t yBarra = MENU_Y0 + (altoPista - altoBarra) * seleccion / (N_MENU - 1);
  oled.fillRect(124, yBarra, 3, altoBarra, BLANCO);

  pie("corto: sig  largo: OK");
  oled.display();
}

void pantallaInicio() {
  if (!hayPantalla) return;
  oled.clearDisplay();
  oled.setTextColor(BLANCO);
  textoCentrado("MINI SUMO", 2, 14);
  oled.drawFastHLine(20, 34, 88, BLANCO);
  textoCentrado("ESP32", 1, 40);
  oled.display();
}

void mostrarMenuSerial() {
  Serial.println();
  Serial.println("========== MINI SUMO ESP32 ==========");
  for (uint8_t i = 0; i < N_MENU; i++) Serial.printf("  %d - %s\n", i + 1, opcionesMenu[i]);
  Serial.println("  0 - Parar motores");
  Serial.println(" En el robot: BOOT corto = siguiente, BOOT largo = OK");
  Serial.println("=====================================");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  analogReadResolution(12);
#if LINEA_DIGITAL
  for (uint8_t i = 0; i < N_LINEA; i++) pinMode(pinesLinea[i], INPUT);
#endif
  motoresInit();
  parar();
  cargarUmbrales();
  pantallaInit();
  oponenteInit();
  pantallaInicio();
  delay(1200);
  mostrarMenuSerial();
  dibujarMenu();
}

void loop() {
  static uint32_t tRefresco = 0;

  Pulso p = leerBoton();
  if (p == CORTO) {
    seleccion = (seleccion + 1) % N_MENU;
    dibujarMenu();
  } else if (p == LARGO) {
    ejecutar(seleccion);
    mostrarMenuSerial();
    dibujarMenu();
  }

  if (Serial.available()) {
    char op = Serial.read();
    vaciarSerial();
    if (op == '0') {
      parar();
      Serial.println("Motores detenidos.");
    } else if (op >= '1' && op < '1' + N_MENU) {
      seleccion = op - '1';
      dibujarMenu();
      ejecutar(seleccion);
      mostrarMenuSerial();
      dibujarMenu();
    }
  }

  // refresca el voltaje (y el parpadeo de batería baja)
  if (millis() - tRefresco > 400) {
    dibujarMenu();
    tRefresco = millis();
  }
  delay(5);
}
