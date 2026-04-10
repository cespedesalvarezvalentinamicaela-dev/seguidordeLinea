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

#define LEDC_FREQ      5000
#define LEDC_RES       8
#define LEDC_CANAL_IZQ 0
#define LEDC_CANAL_DER 1

// ===================== OFFSETS =====================

int offsetIzq = 0;
int offsetDer = -5;   // rueda derecha calibrada a 195

// ===================== PID =====================

int   velocidadBase = 200;

float Kp = 0.08f;
float Ki = 0.0001f;
float Kd = 0.05f;

int  pidError    = 0;
int  pidErrorAnt = 0;
long pidIntegral = 0;

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq + offsetIzq, -VEL_MAX, VEL_MAX);
  velDer = constrain(velDer + offsetDer, -VEL_MAX, VEL_MAX);

  digitalWrite(IN1, velIzq >= 0 ? HIGH : LOW);
  digitalWrite(IN2, velIzq >= 0 ? LOW  : HIGH);
  ledcWrite(LEDC_CANAL_IZQ, abs(velIzq));

  digitalWrite(IN3, velDer >= 0 ? HIGH : LOW);
  digitalWrite(IN4, velDer >= 0 ? LOW  : HIGH);
  ledcWrite(LEDC_CANAL_DER, abs(velDer));
}

void detener()
{
  ledcWrite(LEDC_CANAL_IZQ, 0);
  ledcWrite(LEDC_CANAL_DER, 0);
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

  // Calibracion automatica (3s)
  Serial.println(F("\n[1/3] Calibrando sensores (3s)..."));
  Serial.println(F("      Mueve el robot sobre negro y blanco."));
  for (int i = 0; i < 300; i++)
  {
    qtr.calibrate();
    delay(10);
    if (i % 60 == 0) {
      Serial.print(F("      "));
      Serial.print(i / 3);
      Serial.println(F("%"));
    }
  }
  Serial.println(F("      100% - OK"));

  // Lectura inicial para estabilizar
  for (int i = 0; i < 20; i++) { qtr.readLineBlack(sensorValues); delay(20); }

  // Countdown
  Serial.println(F("\n[2/3] Coloca el robot en la pista..."));
  for (int i = 5; i > 0; i--)
  {
    Serial.print(F("      Arranca en ")); Serial.print(i); Serial.println(F("s..."));
    delay(1000);
  }

  // Reset PID
  pidError = 0; pidErrorAnt = 0; pidIntegral = 0;
  Serial.println(F("\n[3/3] ARRANCANDO!\n"));
}

// ===================== LOOP =====================

void loop()
{
  uint16_t posicion = qtr.readLineBlack(sensorValues);
  pidError = (int)posicion - 3500;

  // Deteccion de linea
  bool lineaDetectada = false;
  int  activos        = 0;
  for (int i = 0; i < NUM_SENSORS; i++)
    if (sensorValues[i] > 500) { lineaDetectada = true; activos++; }

  // Sin linea: girar hacia el ultimo lado conocido
  if (!lineaDetectada || activos < 2)
  {
    if (pidErrorAnt > 0) moverMotores(velocidadBase / 2, -velocidadBase / 2);
    else                 moverMotores(-velocidadBase / 2,  velocidadBase / 2);
    return;
  }

  // PID
  pidIntegral += pidError;
  pidIntegral  = constrain(pidIntegral, -600, 600);

  int derivada = pidError - pidErrorAnt;
  int correccion = constrain(
    (int)(Kp * pidError + Ki * pidIntegral + Kd * derivada),
    -150, 150);
  pidErrorAnt = pidError;

  // Velocidad adaptativa
  int velBase = (abs(pidError) < 200)  ? velocidadBase
              : (abs(pidError) > 2000) ? max(80, velocidadBase - 80)
              :                          max(80, velocidadBase - 40);

  int velIzq = constrain(velBase - correccion, 0, VEL_MAX);
  int velDer = constrain(velBase + correccion, 0, VEL_MAX);

  moverMotores(velIzq, velDer);

  // Debug por Serial
  static unsigned long ultimaPrint = 0;
  if (millis() - ultimaPrint >= 200)
  {
    ultimaPrint = millis();
    Serial.print(F("POS:")); Serial.print(posicion);
    Serial.print(F(" ERR:")); Serial.print(pidError);
    Serial.print(F(" CORR:")); Serial.print(correccion);
    Serial.print(F(" VL:")); Serial.print(velIzq);
    Serial.print(F(" VR:")); Serial.println(velDer);
  }
}
