#include <Arduino.h>
#include <EEPROM.h>

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

int offsetIzq   = 0;
int offsetDer   = 0;
int velocidad   = 180;   // Velocidad de prueba (0-255)
bool motorOn    = false;

// ===================== FUNCIONES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -VEL_MAX, VEL_MAX);
  velDer = constrain(velDer, -VEL_MAX, VEL_MAX);

  // Motor izquierdo
  if (velIzq >= 0) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else             { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  analogWrite(ENA, abs(velIzq));

  // Motor derecho (lógica invertida por cableado físico)
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

void guardarEEPROM()
{
  EEPROM.write(EEPROM_OFFSET_IZQ, (int8_t)offsetIzq);
  EEPROM.write(EEPROM_OFFSET_DER, (int8_t)offsetDer);
}

void imprimirEstado()
{
  int velIzq = constrain(velocidad + offsetIzq, 0, VEL_MAX);
  int velDer = constrain(velocidad + offsetDer, 0, VEL_MAX);

  Serial.println(F("─────────────────────────────────────"));
  Serial.print(F("  Offset IZQ : ")); Serial.println(offsetIzq);
  Serial.print(F("  Offset DER : ")); Serial.println(offsetDer);
  Serial.print(F("  Velocidad  : ")); Serial.println(velocidad);
  Serial.print(F("  Vel IZQ efectiva: ")); Serial.println(velIzq);
  Serial.print(F("  Vel DER efectiva: ")); Serial.println(velDer);
  Serial.print(F("  Motores    : ")); Serial.println(motorOn ? "ENCENDIDOS" : "APAGADOS");
  Serial.println(F("─────────────────────────────────────"));
}

void imprimirAyuda()
{
  Serial.println(F("\n╔═══════════════════════════════════════╗"));
  Serial.println(F("║     CALIBRADOR DE RUEDAS - AYUDA      ║"));
  Serial.println(F("╠═══════════════════════════════════════╣"));
  Serial.println(F("║  GO          Arranca motores           ║"));
  Serial.println(F("║  STOP        Para motores              ║"));
  Serial.println(F("║  L <n>       Offset izquierdo (-127/127)║"));
  Serial.println(F("║  R <n>       Offset derecho (-127/127) ║"));
  Serial.println(F("║  V <n>       Velocidad base (0-255)    ║"));
  Serial.println(F("║  L+  L-      Sube/baja offset izq x5  ║"));
  Serial.println(F("║  R+  R-      Sube/baja offset der x5  ║"));
  Serial.println(F("║  SAVE        Guarda offsets en EEPROM  ║"));
  Serial.println(F("║  LOAD        Carga offsets de EEPROM   ║"));
  Serial.println(F("║  RESET       Pone offsets a 0          ║"));
  Serial.println(F("║  STATUS      Muestra estado actual     ║"));
  Serial.println(F("║  HELP        Muestra esta ayuda        ║"));
  Serial.println(F("╚═══════════════════════════════════════╝\n"));
}

void procesarComando(String cmd)
{
  cmd.trim();
  cmd.toUpperCase();

  if (cmd == "GO")
  {
    aplicarMotores();
    Serial.println(F(">> Motores ENCENDIDOS"));
    imprimirEstado();
  }
  else if (cmd == "STOP")
  {
    detener();
    Serial.println(F(">> Motores DETENIDOS"));
  }
  else if (cmd == "SAVE")
  {
    guardarEEPROM();
    Serial.println(F(">> Offsets guardados en EEPROM"));
    imprimirEstado();
  }
  else if (cmd == "LOAD")
  {
    offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
    offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);
    Serial.println(F(">> Offsets cargados desde EEPROM"));
    imprimirEstado();
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "RESET")
  {
    offsetIzq = 0;
    offsetDer = 0;
    Serial.println(F(">> Offsets reiniciados a 0"));
    imprimirEstado();
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "STATUS")
  {
    imprimirEstado();
  }
  else if (cmd == "HELP")
  {
    imprimirAyuda();
  }
  else if (cmd == "L+")
  {
    offsetIzq = constrain(offsetIzq + 5, -127, 127);
    Serial.print(F(">> Offset IZQ: ")); Serial.println(offsetIzq);
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "L-")
  {
    offsetIzq = constrain(offsetIzq - 5, -127, 127);
    Serial.print(F(">> Offset IZQ: ")); Serial.println(offsetIzq);
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "R+")
  {
    offsetDer = constrain(offsetDer + 5, -127, 127);
    Serial.print(F(">> Offset DER: ")); Serial.println(offsetDer);
    if (motorOn) aplicarMotores();
  }
  else if (cmd == "R-")
  {
    offsetDer = constrain(offsetDer - 5, -127, 127);
    Serial.print(F(">> Offset DER: ")); Serial.println(offsetDer);
    if (motorOn) aplicarMotores();
  }
  else if (cmd.startsWith("L "))
  {
    int val = cmd.substring(2).toInt();
    offsetIzq = constrain(val, -127, 127);
    Serial.print(F(">> Offset IZQ: ")); Serial.println(offsetIzq);
    if (motorOn) aplicarMotores();
  }
  else if (cmd.startsWith("R "))
  {
    int val = cmd.substring(2).toInt();
    offsetDer = constrain(val, -127, 127);
    Serial.print(F(">> Offset DER: ")); Serial.println(offsetDer);
    if (motorOn) aplicarMotores();
  }
  else if (cmd.startsWith("V "))
  {
    int val = cmd.substring(2).toInt();
    velocidad = constrain(val, 0, 255);
    Serial.print(F(">> Velocidad: ")); Serial.println(velocidad);
    if (motorOn) aplicarMotores();
  }
  else
  {
    Serial.print(F(">> Comando desconocido: "));
    Serial.println(cmd);
    Serial.println(F("   Escribe HELP para ver los comandos"));
  }
}

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  detener();

  // Leer offsets guardados
  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);

  // Si EEPROM está vacía (0xFF), empezar en 0
  if (offsetIzq == (int8_t)0xFF) offsetIzq = 0;
  if (offsetDer == (int8_t)0xFF) offsetDer = 0;

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
}
