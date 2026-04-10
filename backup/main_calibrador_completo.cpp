#include <Arduino.h>
#include <QTRSensors.h>
#include <EEPROM.h>

// ===================== SENSORES =====================

#define NUM_SENSORS 8

QTRSensors qtr;
uint16_t sensorValues[NUM_SENSORS];

// ===================== MOTORES =====================

#define ENA 5
#define IN1 8
#define IN2 9
#define ENB 6
#define IN3 10
#define IN4 11
#define VEL_MAX 255

// ===================== EEPROM =====================

#define EEPROM_OFFSET_IZQ 0
#define EEPROM_OFFSET_DER 1

// ===================== ESTADO =====================

bool  sensorOn    = false;   // Lectura continua de sensores
bool  motorOn     = false;   // Motores encendidos
bool  calibrado   = false;
unsigned long ultimaImpresion = 0;
int   intervaloMs = 300;

int   offsetIzq   = 0;
int   offsetDer   = 0;
int   velocidad   = 180;

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -VEL_MAX, VEL_MAX);
  velDer = constrain(velDer, -VEL_MAX, VEL_MAX);

  if (velIzq >= 0) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else             { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  analogWrite(ENA, abs(velIzq));

  if (velDer >= 0) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
  else             { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  analogWrite(ENB, abs(velDer));
}

void detener()
{
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  motorOn = false;
}

void aplicarMotores()
{
  int velIzq = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int velDer = constrain(velocidad + offsetDer, 0, VEL_MAX);
  moverMotores(velIzq, velDer);
  motorOn = true;
}

// ===================== DISPLAY =====================

// Barra visual de posición de línea (0-7000 → 16 caracteres)
void imprimirBarraPosicion(uint16_t posicion)
{
  int pos = map(posicion, 0, 7000, 0, 15);
  Serial.print(F("  |"));
  for (int i = 0; i < 16; i++)
    Serial.print(i == pos ? F("O") : F("-"));
  Serial.println(F("|"));
}

// Fila de barras por sensor
void imprimirBarrasSensores(uint16_t valores[], uint16_t umbral)
{
  Serial.print(F("  ["));
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(valores[i] > umbral ? F("##") : F(".."));
    if (i < NUM_SENSORS - 1) Serial.print(F("|"));
  }
  Serial.println(F("]"));
}

// Display combinado: sensores + ruedas en una sola pantalla
void mostrarLive()
{
  uint16_t posicion = 0;
  bool lineaDetectada = false;

  if (calibrado)
  {
    posicion = qtr.readLineBlack(sensorValues);
    for (int i = 0; i < NUM_SENSORS; i++)
      if (sensorValues[i] > 400) { lineaDetectada = true; break; }
  }
  else
  {
    qtr.read(sensorValues);
  }

  int velIzq = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int velDer = constrain(velocidad + offsetDer, 0, VEL_MAX);

  Serial.println(F("══════════════════════════════════════════════"));

  // Fila 1: sensores numéricos
  Serial.print(F("  S: "));
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    if (calibrado)
    {
      // Valores calibrados 0-1000, mostrar en 3 dígitos
      if (sensorValues[i] < 100) Serial.print(F(" "));
      if (sensorValues[i] < 10)  Serial.print(F(" "));
      Serial.print(sensorValues[i]);
    }
    else
    {
      Serial.print(sensorValues[i]);
    }
    if (i < NUM_SENSORS - 1) Serial.print(F("|"));
  }
  Serial.println();

  // Fila 2: barras visuales
  imprimirBarrasSensores(sensorValues, calibrado ? 400 : 500);

  // Fila 3: posición de línea (solo si calibrado)
  if (calibrado)
  {
    int err = (int)posicion - 3500;
    Serial.print(F("  POS: "));
    Serial.print(posicion);
    Serial.print(F("  ERR: "));
    if (err >= 0) Serial.print(F("+"));
    Serial.print(err);
    Serial.print(F("  Linea: "));
    Serial.println(lineaDetectada ? F("SI") : F("NO"));
    imprimirBarraPosicion(posicion);
  }
  else
  {
    Serial.println(F("  [Sin calibrar - usa CAL]"));
  }

  // Fila 4: estado de ruedas
  Serial.print(F("  M: "));
  Serial.print(motorOn ? F("ON ") : F("OFF"));
  Serial.print(F("  V:"));   Serial.print(velocidad);
  Serial.print(F("  IZQ:+")); Serial.print(offsetIzq);
  Serial.print(F("->"));     Serial.print(velIzq);
  Serial.print(F("  DER:+")); Serial.print(offsetDer);
  Serial.print(F("->"));     Serial.println(velDer);

  Serial.println(F("══════════════════════════════════════════════"));
}

// Display simple de sensores (sin motores)
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
    imprimirBarrasSensores(sensorValues, 400);
  }
  else
  {
    qtr.read(sensorValues);
    Serial.println(F("─────────────────────────────────────────────"));
    Serial.println(F("  [SIN CALIBRAR]"));
    Serial.print(F("  "));
    for (int i = 0; i < NUM_SENSORS; i++)
    {
      Serial.print(sensorValues[i]);
      if (i < NUM_SENSORS - 1) Serial.print(F(" | "));
    }
    Serial.println();
    imprimirBarrasSensores(sensorValues, 500);
  }
}

void calibrarSensores()
{
  bool motoresEstaban = motorOn;
  if (motorOn) detener();

  Serial.println(F("\n>> Calibrando (3s)... mueve el robot sobre la linea y el fondo."));

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
  Serial.println(F("   100%  >> Calibracion completada.\n"));
  Serial.print(F("  Min: "));
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtr.calibrationOn.minimum[i]);
    if (i < NUM_SENSORS - 1) Serial.print(F("|"));
  }
  Serial.println();
  Serial.print(F("  Max: "));
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(qtr.calibrationOn.maximum[i]);
    if (i < NUM_SENSORS - 1) Serial.print(F("|"));
  }
  Serial.println();

  if (motoresEstaban) aplicarMotores();
}

// ===================== AYUDA Y ESTADO =====================

void imprimirAyuda()
{
  Serial.println(F("\n╔════════════════════════════════════════════╗"));
  Serial.println(F("║         HERRAMIENTA DE CALIBRACION         ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  LIVE   Vista combinada sensores+ruedas    ║"));
  Serial.println(F("║  PAUSE  Pausa la vista continua            ║"));
  Serial.println(F("║  STOP   Para TODO (motores + vista)        ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  --- SENSORES ---                          ║"));
  Serial.println(F("║  CAL          Calibrar sensores (3s)       ║"));
  Serial.println(F("║  READ         Lectura unica                ║"));
  Serial.println(F("║  START        Solo lectura continua        ║"));
  Serial.println(F("║  INT <ms>     Intervalo display (def 300)  ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  --- RUEDAS ---                            ║"));
  Serial.println(F("║  GO           Arranca motores              ║"));
  Serial.println(F("║  MSTOP        Para solo los motores        ║"));
  Serial.println(F("║  L+  L-       Offset izq +5 / -5          ║"));
  Serial.println(F("║  R+  R-       Offset der +5 / -5          ║"));
  Serial.println(F("║  L <n>        Offset izquierdo exacto      ║"));
  Serial.println(F("║  R <n>        Offset derecho exacto        ║"));
  Serial.println(F("║  V <n>        Velocidad base (0-255)       ║"));
  Serial.println(F("║  SAVE         Guarda offsets en EEPROM     ║"));
  Serial.println(F("║  LOAD         Carga offsets de EEPROM      ║"));
  Serial.println(F("║  RESET        Offsets a 0                  ║"));
  Serial.println(F("╠════════════════════════════════════════════╣"));
  Serial.println(F("║  STATUS  Estado general  HELP  Esta ayuda  ║"));
  Serial.println(F("╚════════════════════════════════════════════╝\n"));
}

void imprimirEstado()
{
  int velIzq = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int velDer = constrain(velocidad + offsetDer, 0, VEL_MAX);
  Serial.println(F("─────────────────────────────────────────────"));
  Serial.print(F("  Calibrado : ")); Serial.println(calibrado ? "SI" : "NO");
  Serial.print(F("  Display   : ")); Serial.println(sensorOn ? "ON" : "OFF");
  Serial.print(F("  Intervalo : ")); Serial.print(intervaloMs); Serial.println(F(" ms"));
  Serial.print(F("  Motores   : ")); Serial.println(motorOn ? "ON" : "OFF");
  Serial.print(F("  Velocidad : ")); Serial.println(velocidad);
  Serial.print(F("  Offset IZQ: ")); Serial.print(offsetIzq);
  Serial.print(F("  -> ")); Serial.println(velIzq);
  Serial.print(F("  Offset DER: ")); Serial.print(offsetDer);
  Serial.print(F("  -> ")); Serial.println(velDer);
  Serial.println(F("─────────────────────────────────────────────"));
}

// ===================== COMANDOS =====================

void procesarComando(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "LIVE")
  {
    if (motorOn) aplicarMotores();
    sensorOn = true;
    Serial.println(F(">> Modo LIVE ON (PAUSE o STOP para detener)"));
  }
  else if (cmd == "PAUSE")
  {
    sensorOn = false;
    Serial.println(F(">> Display pausado (motores siguen)"));
  }
  else if (cmd == "STOP")
  {
    sensorOn = false;
    detener();
    Serial.println(F(">> TODO detenido"));
  }
  else if (cmd == "CAL")
  {
    sensorOn = false;
    calibrarSensores();
  }
  else if (cmd == "READ")
  {
    mostrarSensores();
  }
  else if (cmd == "START")
  {
    sensorOn = true;
    Serial.println(F(">> Lectura continua ON"));
  }
  else if (cmd.startsWith("INT "))
  {
    int val = cmd.substring(4).toInt();
    if (val >= 50 && val <= 5000)
    {
      intervaloMs = val;
      Serial.print(F(">> Intervalo: ")); Serial.print(intervaloMs); Serial.println(F(" ms"));
    }
    else Serial.println(F(">> Rango: 50-5000 ms"));
  }
  else if (cmd == "GO")
  {
    aplicarMotores();
    Serial.println(F(">> Motores ON"));
  }
  else if (cmd == "MSTOP")
  {
    detener();
    Serial.println(F(">> Motores OFF (display sigue)"));
  }
  else if (cmd == "SAVE")
  {
    EEPROM.write(EEPROM_OFFSET_IZQ, (int8_t)offsetIzq);
    EEPROM.write(EEPROM_OFFSET_DER, (int8_t)offsetDer);
    Serial.println(F(">> Offsets guardados en EEPROM"));
  }
  else if (cmd == "LOAD")
  {
    offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
    offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);
    Serial.println(F(">> Offsets cargados"));
    if (motorOn) aplicarMotores();
    imprimirEstado();
  }
  else if (cmd == "RESET")
  {
    offsetIzq = 0; offsetDer = 0;
    Serial.println(F(">> Offsets a 0"));
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "L+") { offsetIzq = constrain(offsetIzq + 5, -127, 127); Serial.print(F(">> IZQ: ")); Serial.println(offsetIzq); if (motorOn) aplicarMotores(); }
  else if (cmd == "L-") { offsetIzq = constrain(offsetIzq - 5, -127, 127); Serial.print(F(">> IZQ: ")); Serial.println(offsetIzq); if (motorOn) aplicarMotores(); }
  else if (cmd == "R+") { offsetDer = constrain(offsetDer + 5, -127, 127); Serial.print(F(">> DER: ")); Serial.println(offsetDer); if (motorOn) aplicarMotores(); }
  else if (cmd == "R-") { offsetDer = constrain(offsetDer - 5, -127, 127); Serial.print(F(">> DER: ")); Serial.println(offsetDer); if (motorOn) aplicarMotores(); }
  else if (cmd.startsWith("L ")) { offsetIzq = constrain(cmd.substring(2).toInt(), -127, 127); Serial.print(F(">> IZQ: ")); Serial.println(offsetIzq); if (motorOn) aplicarMotores(); }
  else if (cmd.startsWith("R ")) { offsetDer = constrain(cmd.substring(2).toInt(), -127, 127); Serial.print(F(">> DER: ")); Serial.println(offsetDer); if (motorOn) aplicarMotores(); }
  else if (cmd.startsWith("V "))
  {
    velocidad = constrain(cmd.substring(2).toInt(), 0, 255);
    Serial.print(F(">> Velocidad: ")); Serial.println(velocidad);
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "STATUS") { imprimirEstado(); }
  else if (cmd == "HELP")   { imprimirAyuda();  }
  else
  {
    Serial.print(F(">> Desconocido: ")); Serial.println(cmd);
  }
}

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  detener();

  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);
  if (offsetIzq == (int8_t)0xFF) offsetIzq = 0;
  if (offsetDer == (int8_t)0xFF) offsetDer = 0;

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, 2, 3}, NUM_SENSORS);
  qtr.setEmitterPin(7);

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
      // Si hay motores activos → vista combinada, si no → solo sensores
      motorOn ? mostrarLive() : mostrarSensores();
    }
  }
}
