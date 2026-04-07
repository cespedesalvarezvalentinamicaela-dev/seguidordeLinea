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

// ===================== OFFSET (CALIBRACIÓN) =====================
int offsetIzq = 0;
int offsetDer = 0;
#define EEPROM_OFFSET_IZQ 0
#define EEPROM_OFFSET_DER 1

// ===================== PARÁMETROS PID =====================

int velocidadBase = 200;  // Velocidad base
int velocidadMax  = 255;

// PID AJUSTADO para curvas
float Kp = 0.065;  // Control proporcional (aumentado para curvas)
float Ki = 0.0003; // Control integral (recuperado)
float Kd = 0.08;   // Control derivativo (aumentado)

int  error         = 0;
int  errorAnterior = 0;
long integral      = 0;

// ===================== DEBUG =====================
#define DEBUG_MODO_SENSORES 0  // 1 = ON, 0 = OFF
unsigned long ultimaImpresion = 0;
const unsigned long INTERVALO_DEBUG = 200;

// Prototipos de funciones
void moverMotores(int velIzq, int velDer);
void imprimirDebug(uint16_t posicion, int error, int correccion, int velIzq, int velDer);

// ===================== SETUP =====================

void setup()
{
  Serial.begin(9600);

  // Cargar offsets desde EEPROM (calibracion previa)
  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);
  
  // Si no hay calibracion previa, usar valores por defecto
  if (offsetIzq == 255) offsetIzq = 5;      // Izq: +5
  if (offsetDer == 255) offsetDer = -45;    // Der: -45

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Activar emisor IR en Pin 7
  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  // Configuración QTR
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2, A3, A4, A5, 2, 3}, NUM_SENSORS);
  qtr.setEmitterPin(7);

  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  LINE FOLLOWER - Iniciando...      ║");
  Serial.println("╚════════════════════════════════════╝\n");

  // Calibración mejorada
  Serial.println("Calibrando sensores por 2.5 segundos...");
  Serial.println("(Mueve el robot sobre blanco y negro)");
  
  for (int i = 0; i < 250; i++)
  {
    qtr.calibrate();
    delay(10);
    if (i % 50 == 0) Serial.print(".");
  }
  
  Serial.println("\n✓ Calibración completada");
  Serial.println("═════════════════════════════════════\n");
  
  // Lectura inicial para estabilizar
  Serial.println("Estabilizando sensores...");
  for (int i = 0; i < 20; i++)
  {
    qtr.readLineBlack(sensorValues);
    delay(20);
  }
  
  // Initializar PID con valores iniciales
  error = 0;
  errorAnterior = 0;
  integral = 0;
  
  Serial.println("✓ Sistema listo\n");
  delay(1000);
}

// ===================== FUNCIONES =====================

void moverMotores(int velIzq, int velDer)
{
  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // APLICAR OFFSETS DE CALIBRACIÓN
  velIzq += offsetIzq;
  velDer += offsetDer;

  // Re-constrain después de aplicar offsets
  velIzq = constrain(velIzq, -velocidadMax, velocidadMax);
  velDer = constrain(velDer, -velocidadMax, velocidadMax);

  // Motor izquierdo
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

  // Motor derecho (INVERTIDO)
  if (velDer >= 0)
  {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  else
  {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }
  analogWrite(ENB, abs(velDer));
}

void imprimirDebug(uint16_t posicion, int error, int correccion, int velIzq, int velDer)
{
  unsigned long ahora = millis();
  if (ahora - ultimaImpresion < INTERVALO_DEBUG) return;
  ultimaImpresion = ahora;

  Serial.print("POS:");
  Serial.print(posicion);
  Serial.print(" | ERR:");
  Serial.print(error);
  Serial.print(" | CORR:");
  Serial.print(correccion);
  Serial.print(" | VL:");
  Serial.print(velIzq);
  Serial.print(" VR:");
  Serial.println(velDer);
}

// ===================== LOOP =====================

void loop()
{
  uint16_t posicion = qtr.readLineBlack(sensorValues);
  
  error = posicion - 3500;

  // Detectar si hay línea (umbral mejorado)
  bool lineaDetectada = false;
  int sensorActivoCount = 0;
  
  for (int i = 0; i < NUM_SENSORS; i++)
  {
    if (sensorValues[i] > 400)  // Threshold aumentado (era 200)
    {
      lineaDetectada = true;
      sensorActivoCount++;
    }
  }

  if (DEBUG_MODO_SENSORES)
  {
    imprimirDebug(posicion, error, 0, 0, 0);
  }

  // Si no detecta línea → girar suavemente SIN MARCHA ATRÁS
  if (!lineaDetectada || sensorActivoCount < 2)  // Requiere al menos 2 sensores
  {
    if (DEBUG_MODO_SENSORES) Serial.println("→ SIN LINEA - VIRANDO SUAVE");
    
    // Girar hacia donde estaba el error, pero sin marcha atrás
    if (errorAnterior > 0)
      moverMotores(120, 60);  // Gira a la derecha lentamente
    else
      moverMotores(60, 120);  // Gira a la izquierda lentamente
    return;
  }

  // ═══════════════════ PID CONTROL ═══════════════════

  // Integral con anti-windup
  integral += error;
  integral = constrain(integral, -600, 600);  // Rango ampliado para curvas

  // Derivada
  int derivada = error - errorAnterior;
  
  // Cálculo de corrección
  int correccion = (Kp * error) + (Ki * integral) + (Kd * derivada);
  correccion = constrain(correccion, -100, 100);  // Límite para curvas

  errorAnterior = error;

  // Velocidad adaptativa
  int velocidadActual = velocidadBase;
  
  if (abs(error) < 200)        velocidadActual = 240;  // Recto
  else if (abs(error) > 2000)  velocidadActual = 140;  // Curva cerrada
  else                         velocidadActual = 190;  // Curva normal

  int velIzq = velocidadActual + correccion;
  int velDer = velocidadActual - correccion;

  // Asegurar que nunca va hacia atrás (ambos motores siempre positivos)
  velIzq = constrain(velIzq, 0, velocidadMax);
  velDer = constrain(velDer, 0, velocidadMax);

  moverMotores(velIzq, velDer);

  if (DEBUG_MODO_SENSORES)
  {
    imprimirDebug(posicion, error, correccion, velIzq, velDer);
  }
}
