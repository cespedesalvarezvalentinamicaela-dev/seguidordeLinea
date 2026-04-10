#include <Arduino.h>
#include <QTRSensors.h>

// ===================== SENSORES =====================

#define NUM_SENSORS 8
QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORS];
#define PIN_EMISOR_IR 2

// ===================== MOTORES =====================

#define ENA  23
#define IN1  22
#define IN2  21
#define ENB  19
#define IN3  18
#define IN4   5
#define VEL_MAX 255

// LEDC PWM
#define LEDC_FREQ      5000
#define LEDC_RES       8
#define LEDC_CANAL_IZQ 0
#define LEDC_CANAL_DER 1

// ===================== ESTADO =====================

bool  sensorOn  = false;
bool  calibrado = false;
bool  motorOn   = false;
unsigned long ultimaImpresion = 0;
int   intervaloMs = 300;
int   velocidad   = 150;
int   offsetIzq   = 0;
int   offsetDer   = 0;

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq + offsetIzq, -VEL_MAX, VEL_MAX);
  velDer = constrain(velDer + offsetDer, -VEL_MAX, VEL_MAX);

  if (velIzq >= 0) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else             { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  ledcWrite(LEDC_CANAL_IZQ, abs(velIzq));

  if (velDer >= 0) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
  else             { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  ledcWrite(LEDC_CANAL_DER, abs(velDer));

  motorOn = true;
}

void detener()
{
  ledcWrite(LEDC_CANAL_IZQ, 0);
  ledcWrite(LEDC_CANAL_DER, 0);
  motorOn = false;
}

// ===================== SENSORES =====================

void imprimirBarras(uint16_t valores[], uint16_t umbral)
{
  Serial.print(F("  ["));
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(valores[i] > umbral ? F("##") : F(".."));
    if (i < NUM_SENSORS - 1) Serial.print(F("|"));
  }
  Serial.println(F("]"));
}

void mostrarLive()
{
  if (calibrado)
  {
    uint16_t posicion = qtr.readLineBlack(sensorValues);
    int err = (int)posicion - 3500;
    Serial.println(F("══════════════════════════════════════════════"));
    Serial.print(F("  POS: ")); Serial.print(posicion);
    Serial.print(F("  ERR: "));
    if (err >= 0) Serial.print(F("+"));
    Serial.println(err);
    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      if (sensorValues[i] < 100) Serial.print(F(" "));
      if (sensorValues[i] < 10)  Serial.print(F(" "));
      Serial.print(sensorValues[i]);
      if (i < NUM_SENSORS - 1) Serial.print(F(" | "));
    }
    Serial.println();
    imprimirBarras(sensorValues, 400);
  }
  else
  {
    qtr.read(sensorValues);
    Serial.println(F("══════════════════════════════════════════════"));
    Serial.println(F("  [SIN CALIBRAR]"));
    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      Serial.print(sensorValues[i]);
      if (i < NUM_SENSORS - 1) Serial.print(F("|"));
    }
    Serial.println();
    imprimirBarras(sensorValues, 500);
  }

  // Estado motores
  int vi = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int vd = constrain(velocidad + offsetDer, 0, VEL_MAX);
  Serial.print(F("  M: ")); Serial.print(motorOn ? F("ON ") : F("OFF"));
  Serial.print(F("  V:")); Serial.print(velocidad);
  Serial.print(F("  IZQ:")); Serial.print(offsetIzq); Serial.print(F("->")); Serial.print(vi);
  Serial.print(F("  DER:")); Serial.print(offsetDer); Serial.print(F("->")); Serial.println(vd);
  Serial.println(F("══════════════════════════════════════════════"));
}

void mostrarSensores()
{
  if (calibrado)
  {
    uint16_t posicion = qtr.readLineBlack(sensorValues);
    Serial.println(F("─────────────────────────────────────────────"));
    Serial.print(F("  POS: ")); Serial.print(posicion);
    Serial.print(F("  ERR: ")); Serial.println((int)posicion - 3500);
    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      if (sensorValues[i] < 100) Serial.print(F(" "));
      if (sensorValues[i] < 10)  Serial.print(F(" "));
      Serial.print(sensorValues[i]);
      if (i < NUM_SENSORS - 1) Serial.print(F(" | "));
    }
    Serial.println();
    imprimirBarras(sensorValues, 400);
  }
  else
  {
    qtr.read(sensorValues);
    Serial.println(F("─────────────────────────────────────────────"));
    Serial.println(F("  [SIN CALIBRAR] Valores RAW:"));
    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      Serial.print(sensorValues[i]);
      if (i < NUM_SENSORS - 1) Serial.print(F("|"));
    }
    Serial.println();
    imprimirBarras(sensorValues, 500);
  }
}

void calibrarSensores()
{
  bool motoresEstaban = motorOn;
  if (motorOn) detener();

  Serial.println(F("\n>> Calibrando (3s)... mueve sobre la linea y el fondo."));
  for (int i = 0; i < 300; i++)
  {
    qtr.calibrate();
    delay(10);
    if (i % 60 == 0)
    {
      Serial.print(F("   "));
      for (int j = 0; j < i / 30; j++) Serial.print(F("="));
      Serial.print(F("> "));
      Serial.print(i / 3);
      Serial.println(F("%"));
    }
  }
  calibrado = true;
  Serial.println(F("   100%  >> Completada.\n"));
  Serial.print(F("  Min: "));
  for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(qtr.calibrationOn.minimum[i]); if (i < NUM_SENSORS - 1) Serial.print(F("|")); }
  Serial.println();
  Serial.print(F("  Max: "));
  for (int i = 0; i < NUM_SENSORS; i++) { Serial.print(qtr.calibrationOn.maximum[i]); if (i < NUM_SENSORS - 1) Serial.print(F("|")); }
  Serial.println();

  if (motoresEstaban) moverMotores(velocidad, velocidad);
}

// ===================== TEST MOTORES =====================

void testMotores()
{
  Serial.println(F("\n>> TEST MOTORES:"));
  Serial.println(F("   Adelante 2s..."));
  moverMotores(velocidad, velocidad);
  delay(2000);

  Serial.println(F("   Stop 1s..."));
  detener();
  delay(1000);

  Serial.println(F("   Atras 2s..."));
  moverMotores(-velocidad, -velocidad);
  delay(2000);

  Serial.println(F("   Stop 1s..."));
  detener();
  delay(1000);

  Serial.println(F("   Giro izquierda 1s..."));
  moverMotores(velocidad/2, velocidad);
  delay(1000);

  Serial.println(F("   Giro derecha 1s..."));
  moverMotores(velocidad, velocidad/2);
  delay(1000);

  detener();
  Serial.println(F(">> Test completado.\n"));
}

// ===================== AYUDA =====================

void imprimirAyuda()
{
  Serial.println(F("\n╔════════════════════════════════════════════╗"));
  Serial.println(F("║   TESTING ESPduino-32 - SENSORES + MOTORES ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  --- SENSORES ---                          ║"));
  Serial.println(F("║  CAL          Calibrar sensores (3s)       ║"));
  Serial.println(F("║  READ         Lectura unica sensores       ║"));
  Serial.println(F("║  START        Lectura continua sensores    ║"));
  Serial.println(F("║  LIVE         Sensores + estado motores    ║"));
  Serial.println(F("║  PAUSE        Pausa display continuo       ║"));
  Serial.println(F("║  INT <ms>     Intervalo display (def 300)  ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  --- MOTORES ---                           ║"));
  Serial.println(F("║  TEST         Secuencia automatica motores ║"));
  Serial.println(F("║  GO           Adelante con vel actual      ║"));
  Serial.println(F("║  BACK         Atras con vel actual         ║"));
  Serial.println(F("║  MSTOP        Para motores                 ║"));
  Serial.println(F("║  L+  L-       Offset izq +5 / -5          ║"));
  Serial.println(F("║  R+  R-       Offset der +5 / -5          ║"));
  Serial.println(F("║  V <n>        Velocidad (0-255, def 150)   ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  STOP   Para todo  | HELP  Esta ayuda      ║"));
  Serial.println(F("╚════════════════════════════════════════════╝\n"));
}

void imprimirEstado()
{
  int vi = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int vd = constrain(velocidad + offsetDer, 0, VEL_MAX);
  Serial.println(F("─────────────────────────────────────────────"));
  Serial.print(F("  Calibrado : ")); Serial.println(calibrado ? "SI" : "NO");
  Serial.print(F("  Motores   : ")); Serial.println(motorOn   ? "ON" : "OFF");
  Serial.print(F("  Velocidad : ")); Serial.println(velocidad);
  Serial.print(F("  Offset IZQ: ")); Serial.print(offsetIzq); Serial.print(F(" -> ")); Serial.println(vi);
  Serial.print(F("  Offset DER: ")); Serial.print(offsetDer); Serial.print(F(" -> ")); Serial.println(vd);
  Serial.println(F("─────────────────────────────────────────────"));
}

// ===================== COMANDOS =====================

void procesarComando(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if      (cmd == "CAL")   { sensorOn = false; calibrarSensores(); }
  else if (cmd == "READ")  { mostrarSensores(); }
  else if (cmd == "START") { sensorOn = true;  Serial.println(F(">> Lectura continua ON")); }
  else if (cmd == "LIVE")  { sensorOn = true;  Serial.println(F(">> Modo LIVE ON")); }
  else if (cmd == "PAUSE") { sensorOn = false; Serial.println(F(">> Display pausado")); }
  else if (cmd == "STOP")  { sensorOn = false; detener(); Serial.println(F(">> Todo detenido")); }
  else if (cmd == "TEST")  { sensorOn = false; testMotores(); }
  else if (cmd == "GO")    { moverMotores(velocidad, velocidad); Serial.println(F(">> Adelante")); }
  else if (cmd == "BACK")  { moverMotores(-velocidad, -velocidad); Serial.println(F(">> Atras")); }
  else if (cmd == "MSTOP") { detener(); Serial.println(F(">> Motores OFF")); }
  else if (cmd == "STATUS"){ imprimirEstado(); }
  else if (cmd == "HELP")  { imprimirAyuda(); }
  else if (cmd == "L+") { offsetIzq = constrain(offsetIzq + 5, -127, 127); Serial.print(F(">> IZQ: ")); Serial.println(offsetIzq); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "L-") { offsetIzq = constrain(offsetIzq - 5, -127, 127); Serial.print(F(">> IZQ: ")); Serial.println(offsetIzq); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "R+") { offsetDer = constrain(offsetDer + 5, -127, 127); Serial.print(F(">> DER: ")); Serial.println(offsetDer); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd == "R-") { offsetDer = constrain(offsetDer - 5, -127, 127); Serial.print(F(">> DER: ")); Serial.println(offsetDer); if (motorOn) moverMotores(velocidad, velocidad); }
  else if (cmd.startsWith("V "))
  {
    velocidad = constrain(cmd.substring(2).toInt(), 0, 255);
    Serial.print(F(">> Velocidad: ")); Serial.println(velocidad);
    if (motorOn) moverMotores(velocidad, velocidad);
  }
  else if (cmd.startsWith("INT "))
  {
    int val = cmd.substring(4).toInt();
    if (val >= 50 && val <= 5000) { intervaloMs = val; Serial.print(F(">> Intervalo: ")); Serial.print(intervaloMs); Serial.println(F(" ms")); }
    else Serial.println(F(">> Rango: 50-5000 ms"));
  }
  else { Serial.print(F(">> Desconocido: ")); Serial.println(cmd); }
}

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);
  delay(500);

  // Motores
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  ledcSetup(LEDC_CANAL_IZQ, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(ENA, LEDC_CANAL_IZQ);
  ledcSetup(LEDC_CANAL_DER, LEDC_FREQ, LEDC_RES);
  ledcAttachPin(ENB, LEDC_CANAL_DER);
  detener();

  // Sensores
  pinMode(PIN_EMISOR_IR, OUTPUT);
  digitalWrite(PIN_EMISOR_IR, HIGH);
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){16, 17, 25, 26, 32, 14, 4, 13}, NUM_SENSORS);
  qtr.setEmitterPin(PIN_EMISOR_IR);

  imprimirAyuda();
  imprimirEstado();
}

// ===================== LOOP =====================

void loop()
{
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    procesarComando(cmd);
  }

  if (sensorOn)
  {
    unsigned long ahora = millis();
    if (ahora - ultimaImpresion >= (unsigned long)intervaloMs)
    {
      ultimaImpresion = ahora;
      motorOn ? mostrarLive() : mostrarSensores();
    }
  }
}
