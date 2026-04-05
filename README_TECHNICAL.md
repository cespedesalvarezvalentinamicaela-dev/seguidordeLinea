# Technical Handover Document: Arduino Uno Line Follower (PID)

This project is a high-performance line follower robot built on the **PlatformIO** ecosystem for **Arduino Uno**. It uses a **PID (Proportional, Integral, Derivative)** control algorithm to follow a line using an 8-sensor reflectance array.

## 1. Hardware Architecture

### Controller
- **Board**: Arduino Uno (Atmega328P).
- **Architecture**: AVR.

### Sensors (QTR-8RC Variant)
- **Library**: `pololu/QTRSensors @ ^4.0.0`.
- **Logic**: Reflectance sensors. `readLineBlack` is used (High values for Black).
- **Pinout**:
    - **VCC/GND**: 5V/GND from Arduino.
    - **IR (Emitter)**: Pin **4** (Digital). Controlled via `qtr.setEmitterPin(4)`.
    - **D1 to D8 (Output)**: `A0, A1, A2, A3, A4, A5, 2, 3`. (8-sensor total).

### Actuators (L298N Motor Driver)
- **Motor A (Left)**: ENA (Pin 5, PWM), IN1 (Pin 8), IN2 (Pin 9).
- **Motor B (Right)**: ENB (Pin 6, PWM), IN3 (Pin 10), IN4 (Pin 11).
- **Polarity Note**: Right motor logic is inverted in software (`src/main.cpp`) to ensure both wheels rotate forward for positive speed values.

## 2. Control Logic (PID Algorithm)

### Error Calculation
- **Range**: 0 to 7000 (3500 is center).
- **Error**: `posicion - 3500`.

### PID Constants (Current Tuning)
- `Kp = 0.055`: Proportional response.
- `Ki = 0.0005`: Integral accumulation (anti-windup limited to ±1500).
- `Kd = 0.22`: Derivative damping.
- **Correction**: `correccion = (Kp * error) + (Ki * integral) + (Kd * derivada)`. Limited to ±150 for motor stability.

### Speed Adaptation
- **Straight**: If `abs(error) < 300`, `speed = 230`.
- **Tight Curve**: If `abs(error) > 2000`, `speed = 140`.
- **Base Speed**: `170`.

### Line Loss Recovery
If all sensors read < 200 (white), the robot enters a recovery state:
- If `errorAnterior > 0`: Rotate CW (`180, -180`).
- If `errorAnterior < 0`: Rotate CCW (`-180, 180`).

## 3. PlatformIO Configuration (`platformio.ini`)
- **Environment**: `[env:uno]`.
- **Upload Port**: Hardcoded to `COM9` to resolve permission conflicts, can be changed back to auto-detect if needed.
- **Monitor Speed**: `9600`.

## 4. Debugging & Maintenance
The `loop()` includes a serial debug block that prints:
- Individual sensor values (`SENSORS: ...`).
- Calculated Position, Error, and PID Correction.
- Target motor speeds (IZQ, DER).

### Future Improvements
1.  **PID Tuning**: Adjust `Kp` for speed and `Kd` for oscillation damping.
2.  **Start Button**: Add a digital input check in `setup()` to wait for a trigger before starting calibration.
3.  **EEPROM Storage**: Save calibration data to avoid re-calibrating every power cycle.

---
**Prepared by Antigravity AI for Handover.**
