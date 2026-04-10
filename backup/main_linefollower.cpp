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

// ===================== OFFSETS (desde EEPROM) =====================

int offsetIzq = 0;
int offsetDer = 0;
#define EEPROM_OFFSET_IZQ 0
#define EEPROM_OFFSET_DER 1

// ===================== PARÁMETROS PID =====================

int velocidadBase = 200;
int velocidadMax  = 255;

float Kp = 0.35;
float Ki = 0.0003;
float Kd = 0.20;

int  error         = 0;
int  errorAnterior = 0;
long integral      = 0;

// ===================== DEBUG =====================
// Cambiar a 1 para ver telemetría por Serial mientras está conectado
#define DEBUG_MODO_SENSORES 0
unsigned long ultimaImpresion = 0;
const unsigned long INTERVALO_DEBUG = 200;

// ===================== PROTOTIPOS =====================

void moverMotores(int velIzq, int velDer);
void imprimirDebug(uint16_t posicion, int error, int correccion, int velIzq, int velDer);

// ===================== MOTORES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // Aplicar offsets — mínimo 0 para no invertir el sentido
  velIzq = constrain(velIzq + offsetIzq, 0, velocidadMax);
  velDer = constrain(velDer + offsetDer, 0, velocidadMax);

  // Motor izquierdo (lógica invertida por cableado físico)
  if (velIzq >= 0) { digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  }
  else             { digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); }
  analogWrite(ENA, abs(velIzq));

  // Motor derecho
  if (velDer >= 0) { digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  }
  else             { digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); }
  analogWrite(ENB, abs(velDer));
}

void imprimirDebug(uint16_t posicion, int error, int correccion, int velIzq, int velDer)
{
  unsigned long ahora = millis();
  if (ahora - ultimaImpresion < INTERVALO_DEBUG) return;
  ultimaImpresion = ahora;
  Serial.print(F("POS:")); Serial.print(posicion);
  Serial.print(F(" ERR:")); Serial.print(error);
  Serial.print(F(" CORR:")); Serial.print(correccion);
  Serial.print(F(" VL:")); Serial.print(velIzq);
  Serial.print(F(" VR:")); Serial.println(velDer);
}

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);

  // Cargar offsets calibrados desde EEPROM
  int8_t ei = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  int8_t ed = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);
  offsetIzq = (ei == (int8_t)0xFF) ? 0 : ei;
  offsetDer = (ed == (int8_t)0xFF) ? 0 : ed;

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Emisor IR
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Configuración QTR
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, 2, 3}, NUM_SENSORS);
  qtr.setEmitterPin(7);

  Serial.println(F("\n╔════════════════════════════════════╗"));
  Serial.println(F("║     LINE FOLLOWER - Iniciando      ║"));
  Serial.println(F("╚════════════════════════════════════╝"));
  Serial.print(F("  Offset IZQ: ")); Serial.println(offsetIzq);
  Serial.print(F("  Offset DER: ")); Serial.println(offsetDer);

  // ── FASE 1: Calibración de sensores (2.5s) ──
  Serial.println(F("\n[1/3] Calibrando sensores (2.5s)..."));
  Serial.println(F("      Mueve el robot sobre negro y blanco."));

  for (int i = 0; i < 250; i++)
  {
    qtr.calibrate();
    delay(10);
    if (i % 50 == 0) Serial.print(F("."));
  }
  Serial.println(F("\n      OK"));

  // Lecturas iniciales para estabilizar
  for (int i = 0; i < 20; i++)
  {
    qtr.readLineBlack(sensorValues);
    delay(20);
  }

  // ── FASE 2: Countdown — tiempo para colocar en pista ──
  Serial.println(F("\n[2/3] Coloca el robot en la pista..."));
  for (int i = 5; i > 0; i--)
  {
    Serial.print(F("      Arranca en "));
    Serial.print(i);
    Serial.println(F("s..."));
    delay(1000);
  }

  // ── FASE 3: Arranque ──
  error = 0;
  errorAnterior = 0;
  integral = 0;

  Serial.println(F("\n[3/3] ARRANCANDO!\n"));
}

// ===================== LOOP =====================

void loop()
{
  uint16_t posicion = qtr.readLineBlack(sensorValues);

  error = posicion - 3500;

  // Detección de línea — umbral 500 filtra ruido
  bool lineaDetectada = false;
  int  sensoresActivos = 0;

  for (int i = 0; i < NUM_SENSORS; i++)
  {
    if (sensorValues[i] > 500)
    {
      lineaDetectada = true;
      sensoresActivos++;
    }
  }

  // Sin línea → girar hacia el último lado conocido
  if (!lineaDetectada || sensoresActivos < 2)
  {
    if (DEBUG_MODO_SENSORES) Serial.println(F("SIN LINEA"));
    if (errorAnterior > 0)
      moverMotores(120, 60);
    else
      moverMotores(60, 120);
    return;
  }

  // ═══════════════ PID ═══════════════

  integral += error;
  integral = constrain(integral, -600, 600);

  int derivada   = error - errorAnterior;
  int correccion = (int)(Kp * error + Ki * integral + Kd * derivada);
  correccion     = constrain(correccion, -150, 150);

  errorAnterior = error;

  // Velocidad adaptativa según curvatura
  int velocidadActual;
  if      (abs(error) < 200)  velocidadActual = 200;  // recto
  else if (abs(error) > 2000) velocidadActual = 120;  // curva cerrada
  else                        velocidadActual = 160;  // curva normal

  int velIzq = constrain(velocidadActual + correccion, 0, velocidadMax);
  int velDer = constrain(velocidadActual - correccion, 0, velocidadMax);

  moverMotores(velIzq, velDer);

  if (DEBUG_MODO_SENSORES)
    imprimirDebug(posicion, error, correccion, velIzq, velDer);
}
