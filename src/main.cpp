#include <Arduino.h>
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
bool enModoCaliberacion = false;

// Prototipos
void mostrarOffsets();
void moverMotores(int velIzq, int velDer);
void detenerMotores();
void procesarComando(char cmd);

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

  // Cargar offsets desde EEPROM
  offsetIzq = (int8_t)EEPROM.read(EEPROM_OFFSET_IZQ);
  offsetDer = (int8_t)EEPROM.read(EEPROM_OFFSET_DER);

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  CALIBRACION DE MOTORES - OFFSETS     ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  mostrarOffsets();
  
  Serial.println("COMANDOS:");
  Serial.println("  'C' -> Activar modo calibracion");
  Serial.println("  'I' -> Aumentar offset izq (+1)");
  Serial.println("  'J' -> Disminuir offset izq (-1)");
  Serial.println("  'D' -> Aumentar offset der (+1)");
  Serial.println("  'F' -> Disminuir offset der (-1)");
  Serial.println("  'S' -> Guardar en EEPROM");
  Serial.println("  'R' -> Reset offsets a 0\n");
  
  delay(1000);
}

// ===================== MOSTRAR OFFSETS =====================
void mostrarOffsets()
{
  Serial.println("\n┌─────────────────────────┐");
  Serial.print("│ Izquierdo: ");
  Serial.print(offsetIzq);
  if(offsetIzq >= 0) Serial.print("  ");
  Serial.println("│");
  Serial.print("│ Derecho:   ");
  Serial.print(offsetDer);
  if(offsetDer >= 0) Serial.print("  ");
  Serial.println("│");
  Serial.println("└─────────────────────────┘\n");
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

// ===================== PROCESAR COMANDOS =====================
void procesarComando(char cmd)
{
  switch(cmd)
  {
    case 'C':
    case 'c':
      enModoCaliberacion = !enModoCaliberacion;
      if(enModoCaliberacion) {
        Serial.println("\n→ MODO CALIBRACION ACTIVADO");
        Serial.println("→ Robot avanzara recto...\n");
      } else {
        Serial.println("\n→ Modo calibracion desactivado\n");
        detenerMotores();
      }
      break;

    case 'I':
    case 'i':
      offsetIzq++;
      if(offsetIzq > 20) offsetIzq = 20;
      Serial.print("Offset Izq: ");
      Serial.println(offsetIzq);
      break;

    case 'J':
    case 'j':
      offsetIzq--;
      if(offsetIzq < -20) offsetIzq = -20;
      Serial.print("Offset Izq: ");
      Serial.println(offsetIzq);
      break;

    case 'D':
    case 'd':
      offsetDer++;
      if(offsetDer > 20) offsetDer = 20;
      Serial.print("Offset Der: ");
      Serial.println(offsetDer);
      break;

    case 'F':
    case 'f':
      offsetDer--;
      if(offsetDer < -20) offsetDer = -20;
      Serial.print("Offset Der: ");
      Serial.println(offsetDer);
      break;

    case 'S':
    case 's':
      EEPROM.write(EEPROM_OFFSET_IZQ, (int8_t)offsetIzq);
      EEPROM.write(EEPROM_OFFSET_DER, (int8_t)offsetDer);
      Serial.println("\n✓ Offsets guardados en EEPROM");
      mostrarOffsets();
      detenerMotores();
      enModoCaliberacion = false;
      break;

    case 'R':
    case 'r':
      offsetIzq = 0;
      offsetDer = 0;
      Serial.println("\n✓ Offsets resetados a 0");
      mostrarOffsets();
      break;

    case '?':
      Serial.println("\nCOMARDOS DISPONIBLES:");
      Serial.println("  C -> Calibracion ON/OFF");
      Serial.println("  I/J -> Offset izquierdo");
      Serial.println("  D/F -> Offset derecho");
      Serial.println("  S -> Guardar");
      Serial.println("  R -> Reset\n");
      break;
  }
}

// ===================== LOOP =====================
void loop()
{
  // Procesar comandos seriales
  if(Serial.available())
  {
    char cmd = Serial.read();
    if(cmd != '\n' && cmd != '\r') {
      procesarComando(cmd);
    }
  }

  // En modo calibracion: avanzar recto
  if(enModoCaliberacion)
  {
    moverMotores(velocidadBase, velocidadBase);
  }
  else
  {
    detenerMotores();
  }

  delay(50);
}
