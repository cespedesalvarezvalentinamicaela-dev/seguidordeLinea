#include <Arduino.h>
#include <QTRSensors.h>

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

// ===================== PARÁMETROS =====================

int velocidadBase = 170;
int velocidadMax  = 255;

float Kp = 0.055;
float Ki = 0.0005;
float Kd = 0.22;

int  error         = 0;
int  errorAnterior = 0;
long integral      = 0;

// Prototipos de funciones (requeridos para C++ estándar en PlatformIO)
void moverMotores(int velIzq, int velDer);

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

  // Configuración QTR v4.0.0
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){A0,A1,A2,A3,A4,A5,2,3}, NUM_SENSORS);
  qtr.setEmitterPin(4);

  Serial.println("Calibrando sensores...");
  for (int i = 0; i < 250; i++)
  {
    qtr.calibrate();
    delay(10);
  }
  Serial.println("Calibracion terminada");
  delay(1000);
}

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // Motor izquierdo (polaridad invertida)
  if (velIzq >= 0)
  {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }
  else
  {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  analogWrite(ENA, abs(velIzq));

  // Motor derecho (polaridad invertida)
  if (velDer >= 0)
  {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  else
  {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  analogWrite(ENB, abs(velDer));
}

// ===================== LOOP =====================

void loop()
{
  uint16_t posicion = qtr.readLineBlack(sensorValues);

  error = posicion - 3500;

  // --- Serial (Debug Individual Sensors) ---
  Serial.print("SENSORS: ");
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    Serial.print(sensorValues[i]);
    Serial.print("\t");
  }
  Serial.print("| ");

  // --- Detectar línea ---
  bool lineaDetectada = false;
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    if (sensorValues[i] > 200)
    {
      lineaDetectada = true;
      break;
    }
  }

  if (!lineaDetectada)
  {
    Serial.println("SIN LINEA - girando");
    if (errorAnterior > 0)
      moverMotores(180, -180);
    else
      moverMotores(-180, 180);
    return;
  }

  // --- PID ---
  integral += error;
  integral = constrain(integral, -1500, 1500);

  int derivada   = error - errorAnterior;
  int correccion = (Kp * error) + (Ki * integral) + (Kd * derivada);
  correccion     = constrain(correccion, -150, 150);

  errorAnterior = error;

  // --- Velocidad adaptativa ---
  int velocidadActual;
  if      (abs(error) < 300)  velocidadActual = 230;
  else if (abs(error) > 2000) velocidadActual = 140;
  else                        velocidadActual = velocidadBase;

  int velIzq = velocidadActual + correccion;
  int velDer = velocidadActual - correccion;

  moverMotores(velIzq, velDer);

  // --- Serial (PID & Motors) ---
  Serial.print("Pos:"); Serial.print(posicion);
  Serial.print(" Err:"); Serial.print(error);
  Serial.print(" Corr:"); Serial.print(correccion);
  Serial.print(" Vel:"); Serial.print(velocidadActual);
  Serial.print(" IZQ:"); Serial.print(velIzq);
  Serial.print(" DER:"); Serial.println(velDer);
}
