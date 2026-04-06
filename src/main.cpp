#include <Arduino.h>
#include <EEPROM.h>

// ===================== MOTORES =====================
#define ENA 5
#define IN1 8
#define IN2 9
#define ENB 6
#define IN3 10
#define IN4 11

// ===================== LED (opcional) =====================
#define LED_PIN 13

// ===================== OFFSET (EEPROM) =====================
int offsetIzq = 0;
int offsetDer = 0;
#define EEPROM_OFFSET_IZQ 0
#define EEPROM_OFFSET_DER 1

// ===================== PARAMETROS =====================
int velocidadBase = 180;
int velocidadMax = 255;

// ESTADOS
unsigned long tiempoInicio = 0;
const int ESPERA_INICIAL = 3000;  // 3 segundos de espera
const int DURACION_MOVIMIENTO = 5000; // 5 segundos de movimiento
bool yaMovio = false;

// ===================== SETUP =====================
void setup()
{
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED indicador de inicio

  // Cargar offsets desde EEPROM
  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);

  tiempoInicio = millis();
  
  // Espera inicial: LED parpadeante
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

  // Motor derecho (invertido)
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

// ===================== LOOP =====================
void loop()
{
  unsigned long ahora = millis();
  unsigned long tiempoTranscurrido = ahora - tiempoInicio;

  // FASE 1: Espera inicial (3 segundos)
  if (tiempoTranscurrido < ESPERA_INICIAL) {
    detenerMotores();
    
    // Parpadeo del LED cada 500ms
    if ((tiempoTranscurrido / 500) % 2 == 0) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }
    return;
  }

  // FASE 2: Movimiento (5 segundos)
  if (tiempoTranscurrido < (ESPERA_INICIAL + DURACION_MOVIMIENTO)) {
    digitalWrite(LED_PIN, HIGH);  // LED fijo durante movimiento
    moverMotores(velocidadBase, velocidadBase);
    yaMovio = true;
    return;
  }

  // FASE 3: Parado
  if (yaMovio) {
    detenerMotores();
    digitalWrite(LED_PIN, LOW);
  }
}
