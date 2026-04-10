# Calibrador WiFi — Guía de Uso

Programa de calibración interactiva para el robot seguidor de línea basado en **ESPduino-32 (ESP32 D1 R32)**.
Permite calibrar sensores QTR y ajustar offsets de velocidad de ruedas desde cualquier dispositivo
con WiFi y navegador web — sin cable USB.

---

## Hardware

### Sensores QTR-8RC

| Sensor | GPIO ESP32 | Notas |
|--------|------------|-------|
| S1     | GPIO 16    |       |
| S2     | GPIO 17    |       |
| S3     | GPIO 25    |       |
| S4     | GPIO 26    |       |
| S5     | GPIO 32    | GPIO15 descartado (pull-up bootstrapping) |
| S6     | GPIO 14    |       |
| S7     | GPIO 4     |       |
| S8     | GPIO 13    |       |
| Emisor IR | GPIO 2  | Activo HIGH |
| **VCC** | **3.3V** | **IMPORTANTE: NO conectar a 5V** |

> Los sensores QTR deben alimentarse a **3.3V** para que las señales de salida sean compatibles
> con los GPIO del ESP32 (máx. 3.3V). Con 5V los sensores dejan de leer correctamente.

### Motores L298N

| Función        | GPIO ESP32 |
|----------------|------------|
| ENA (PWM izq)  | GPIO 23    |
| IN1            | GPIO 22    |
| IN2            | GPIO 21    |
| ENB (PWM der)  | GPIO 19    |
| IN3            | GPIO 18    |
| IN4            | GPIO 5     |

PWM via LEDC (canales 0 y 1, frecuencia 5kHz, resolución 8 bits).

---

## Conexión WiFi

El ESP32 levanta un **Access Point** propio al arrancar:

| Campo       | Valor              |
|-------------|--------------------|
| Red WiFi    | `Robot-Calibrador` |
| Contraseña  | `12345678`         |
| URL         | `http://192.168.4.1` |

1. Conecta tu móvil o PC a la red `Robot-Calibrador`
2. Abre el navegador en `http://192.168.4.1`
3. La página carga automáticamente

La IP también se imprime por Serial Monitor al arrancar (9600 baud).

---

## Interfaz Web

### Panel de estado
Muestra en tiempo real:
- Calibrado: SI / NO
- Motores: ON / OFF
- Velocidad actual
- Offset IZQ y DER con velocidad efectiva resultante

### Sensores
- **Barras verticales** por sensor: altura proporcional al valor (0=blanco, 1000=negro)
- Barra verde = sensor activo (valor > 400)
- **POS**: posición de la línea (0=extremo izq, 3500=centro, 7000=extremo der)
- **ERR**: error respecto al centro (negativo=línea a la izquierda, positivo=a la derecha)
- **Punto deslizante**: posición visual de la línea en el array

> **Orientación física**: S8 está a la izquierda y S1 a la derecha del robot.
> Las barras y el punto deslizante respetan esta orientación — lo que ves en pantalla
> corresponde directamente a lo que ves mirando el robot desde arriba.

### Controles

| Botón / Control | Acción |
|-----------------|--------|
| **CAL (3s)**    | Calibra sensores durante 3 segundos. Mover robot sobre negro y blanco. |
| **READ**        | Lectura puntual de sensores |
| **GO**          | Arranca motores adelante con velocidad y offsets actuales |
| **BACK**        | Arranca motores atrás |
| **STOP**        | Para motores |
| **TEST**        | Secuencia automática: adelante 2s, atrás 2s, giro izq, giro der |
| **Slider Vel**  | Ajusta velocidad base (0–255). Default: 200 |
| **IZQ +5 / -5** | Sube o baja offset del motor izquierdo en ±5 |
| **DER +5 / -5** | Sube o baja offset del motor derecho en ±5 |
| **STOP TODO**   | Para motores (botón rojo) |

Los cambios de velocidad y offset tienen efecto inmediato si los motores están encendidos.

### Simulacion PID

| Botón / Control | Acción |
|-----------------|--------|
| **INICIAR PID** | Arranca el seguidor de línea con control PID. Requiere calibración previa. |
| **PARAR PID**   | Detiene el PID y para los motores. |
| **Slider Kp**   | Ganancia proporcional (0–1.00, default 0.35) |
| **Slider Ki**   | Ganancia integral (0–0.001, default 0.0003) |
| **Slider Kd**   | Ganancia derivativa (0–1.00, default 0.20) |

Panel de telemetría en tiempo real (visible mientras el PID está activo):

| Campo  | Significado |
|--------|-------------|
| `ERR`  | Error de posición respecto al centro (negativo=línea a la izq, positivo=a la der) |
| `CORR` | Corrección calculada por el PID (se suma/resta a la velocidad base) |
| `VL`   | Velocidad efectiva rueda izquierda (0–255) |
| `VR`   | Velocidad efectiva rueda derecha (0–255) |

El PID usa velocidad adaptativa según la curvatura detectada:
- `abs(ERR) < 200` → velocidad base (recto)
- `abs(ERR) < 2000` → velocidad base − 40 (curva)
- `abs(ERR) >= 2000` → velocidad base − 80 (curva cerrada)

Si pierde la línea, gira hacia el último lado conocido hasta recuperarla.

### D-pad paso a paso

Cruz de 4 botones (↑ ↓ ← →) para mover el robot en pasos cortos y observar la respuesta de los sensores en tiempo real.

| Botón | Acción |
|-------|--------|
| ↑     | Avanza adelante durante el tiempo configurado y para |
| ↓     | Avanza atrás durante el tiempo configurado y para |
| ←     | Gira a la izquierda (motor izq atrás, motor der adelante) y para |
| →     | Gira a la derecha (motor izq adelante, motor der atrás) y para |
| **Slider Duracion** | Tiempo de cada paso: 50ms – 2000ms (default 300ms) |

Uso típico: calibrar con `CAL`, colocar el robot sobre la pista y avanzar paso a paso con ↑ observando qué sensores se activan.

---

## Flujo de calibración recomendado

### 1. Calibrar sensores
Pulsa **CAL (3s)** y mueve el robot lentamente sobre la línea negra y el fondo blanco durante 3 segundos.

Valores esperados tras calibración:
- Min por sensor: ~127 (blanco)
- Max por sensor: ~2500 (negro)

### 2. Verificar sensores sobre la pista
Coloca el robot sobre la línea y observa las barras en tiempo real:
- 2–3 sensores activos simultáneamente (barra verde)
- POS entre 0 y 7000, ERR = POS − 3500

### 3. Calibrar ruedas
```
Slider → 200  →  GO  →  observar si el robot se desvía
```
- Se va a la derecha → **IZQ +5** o **DER -5**
- Se va a la izquierda → **DER +5** o **IZQ -5**

Repetir hasta que avance recto.

---

## Serial como fallback

Si el cable USB está conectado, también se aceptan comandos por Serial (9600 baud):

```
CAL   GO   BACK   MSTOP   STOP   TEST
V <n>   L+   L-   R+   R-   RESET_OFF
STEP_FWD   STEP_BACK   STEP_LEFT   STEP_RIGHT
PASO <ms>
PID_ON   PID_OFF
KP <valor>   KI <valor>   KD <valor>
```

---

## Notas técnicas

- GPIO15 descartado para S5: tiene pull-up interno de bootstrapping que interfiere
  con el timing RC del QTR (siempre lee 2500).
- GPIO 36–39 son input-only en ESP32: no válidos para sensores QTR-RC (necesitan
  drive para cargar el condensador).
- La página web hace polling a `/status` cada 300ms — funciona bien en redes locales.
- Durante la calibración (`CAL`) se llama `server.handleClient()` periódicamente
  para mantener la web responsiva.
