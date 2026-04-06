// DIAGNÓSTICO RÁPIDO DE SENSORES
// Copia este código en main.cpp temporalmente para diagnosticar

#include <Arduino.h>

void setup()
{
  Serial.begin(9600);
  delay(1000);
  
  Serial.println("\n===== DIAGNÓSTICO DE SENSORES =====\n");
  
  uint8_t pinesAnalogicos[] = {A0, A1, A2, A3, A4, A5, 2, 3};
  
  for (int i = 0; i < 8; i++)
  {
    int valor = analogRead(pinesAnalogicos[i]);
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(" (Pin ");
    Serial.print(pinesAnalogicos[i]);
    Serial.print("): ");
    Serial.println(valor);
  }
  
  Serial.println("\n======== PASA ALGO NEGRO POR CADA SENSOR ========\n");
}

void loop()
{
  uint8_t pinesAnalogicos[] = {A0, A1, A2, A3, A4, A5, 2, 3};
  
  Serial.println("Lecturas actuales:");
  for (int i = 0; i < 8; i++)
  {
    int valor = analogRead(pinesAnalogicos[i]);
    Serial.print("S");
    Serial.print(i);
    Serial.print(":");
    Serial.print(valor);
    Serial.print(" ");
  }
  Serial.println();
  
  delay(500);
}
