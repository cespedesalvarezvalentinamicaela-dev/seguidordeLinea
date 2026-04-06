#include <Arduino.h>
#include <QTRSensors.h>
#include <EEPROM.h>

// ===================== MOTORES =====================
#define ENA 5
#define IN1 8
#define IN2 9
#define ENB 6
#define IN3 10
#define IN4 11

// ===================== OFFSET (EEPROM) =====================
int offsetIzq = 0;
int offsetDer = 0;
#define EEPROM_OFFSET_IZQ 0
#define EEPROM_OFFSET_DER 1

// ===================== PARAMETROS =====================
int velocidadBase = 200;
int velocidadMax = 255;
int velocidadPrueba = 150;

// CALIBRACION
bool calibrando = true;
int intentosCalib = 0;
const int MAX_INTENTOS = 5;
long tiempoInicio = 0;
const int DURACION_PRUEBA = 3000; // 3 segundos

// ===================== SETUP =====================
void setup()
{
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Cargar offsets actuales
  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);

  delay(500);
}

// ===================== MOVER MOTORES =====================
void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // Aplicar offsets
  velIzq += offsetIzq;
  velDer += offsetDer;

  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // Motor izquierdo
  if (velIzq >= 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  analogWrite(ENA, abs(velIzq));

  // Motor derecho
  if (velDer >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  analogWrite(ENB, abs(velDer));
}

void detenerMotores()
{
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

// ===================== CALIBRACION =====================
void calibrarMotores()
{
  if (intentosCalib >= MAX_INTENTOS) {
    // Guardar offsets actuales
    EEPROM.write(EEPROM_OFFSET_IZQ, (int8_t)offsetIzq);
    EEPROM.write(EEPROM_OFFSET_DER, (int8_t)offsetDer);
    calibrando = false;
    return;
  }

  intentosCalib++;

  // Prueba: avanzar recto
  tiempoInicio = millis();
  while (millis() - tiempoInicio < DURACION_PRUEBA) {
    moverMotores(velocidadPrueba, velocidadPrueba);
    delay(10);
  }

  // Parar
  detenerMotores();
  delay(500);

  // En esta version: auto-ajusta incrementalmente
  // Intenta aplicar pequeñas mejoras cada vez
  if (intentosCalib < MAX_INTENTOS) {
    // Incrementa izquierdo ligeramente para probar
    offsetIzq += 1;
    delay(1000);
  }
}

// ===================== LOOP =====================
void loop()
{
  if (calibrando) {
    calibrarMotores();
  } else {
    // Despues de calibrar: avanza recto
    moverMotores(velocidadBase, velocidadBase);
  }
}
