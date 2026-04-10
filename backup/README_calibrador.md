# Herramienta de Calibración — Guía de Uso

Programa de calibración interactiva para el robot seguidor de línea.
Permite probar sensores QTR y ajustar offsets de velocidad de ruedas
vía Serial Monitor, guardando los resultados en EEPROM.

---

## Hardware

| Pin  | Función              |
|------|----------------------|
| A0–A5, D2, D3 | Sensores QTR-8RC |
| D7   | Emisor IR (activo HIGH) |
| D5 (ENA) | PWM motor izquierdo |
| D8 (IN1) / D9 (IN2) | Dirección motor izquierdo |
| D6 (ENB) | PWM motor derecho |
| D10 (IN3) / D11 (IN4) | Dirección motor derecho |
| EEPROM addr 0 | Offset motor izquierdo (int8_t) |
| EEPROM addr 1 | Offset motor derecho (int8_t)  |

**Motor izquierdo**: lógica invertida (IN1=HIGH → adelante)  
**Motor derecho**: IN3=HIGH → adelante

---

## Comandos Serial (9600 baud)

### Vista combinada
| Comando | Descripción |
|---------|-------------|
| `LIVE`  | Activa vista combinada sensores + ruedas (se actualiza cada 300ms) |
| `PAUSE` | Pausa el display, motores siguen corriendo |
| `STOP`  | Para TODO: motores y display |

### Sensores
| Comando     | Descripción |
|-------------|-------------|
| `CAL`       | Calibra sensores durante 3 segundos. Mover robot sobre negro y blanco. |
| `READ`      | Lectura única de sensores |
| `START`     | Lectura continua solo de sensores (sin motores) |
| `INT <ms>`  | Cambia intervalo del display continuo (50–5000 ms, default 300) |

### Ruedas
| Comando    | Descripción |
|------------|-------------|
| `GO`       | Arranca motores con velocidad y offsets actuales |
| `MSTOP`    | Para motores sin detener el display |
| `V <n>`    | Velocidad base de prueba (0–255, default 180) |
| `L+` / `L-` | Offset motor izquierdo ±5 |
| `R+` / `R-` | Offset motor derecho ±5 |
| `L <n>`    | Offset izquierdo exacto (−127 a 127) |
| `R <n>`    | Offset derecho exacto (−127 a 127) |
| `SAVE`     | Guarda offsets en EEPROM (persiste al apagar) |
| `LOAD`     | Carga offsets guardados desde EEPROM |
| `RESET`    | Pone ambos offsets a 0 |

### General
| Comando  | Descripción |
|----------|-------------|
| `STATUS` | Muestra estado completo del sistema |
| `HELP`   | Muestra menú de ayuda |

---

## Flujo de calibración recomendado

### 1. Calibrar sensores
```
CAL
```
Durante 3 segundos, mover el robot lentamente de lado a lado
sobre la línea negra y el fondo blanco.

Resultado esperado:
- Min por sensor: ~44–50 (blanco)
- Max por sensor: ~2500 (negro)

### 2. Verificar sensores sobre la pista
```
CAL → LIVE
```
Mover el robot sobre la línea. Lecturas correctas para línea de 2cm:
- 2–3 sensores activos simultáneamente (valor ~1000)
- Resto en ~0
- POS entre 0 y 7000, ERR = POS − 3500

### 3. Calibrar ruedas (en vacío)
```
V 150 → GO → LIVE
```
Observar si el robot se desvía. Ajustar:
- Se va a la derecha → `L+` o `R-`
- Se va a la izquierda → `R+` o `L-`

### 4. Guardar
```
SAVE
```
Los offsets quedan grabados en EEPROM y el seguidor de línea
los cargará automáticamente al iniciar.

---

## Interpretación del display LIVE

```
══════════════════════════════════════════════
  S:   0|  0|1000|1000|1000|  0|  0|  0
  [..|..|##|##|##|..|..|..]
  POS: 3000  ERR: -500  Linea: SI
  |-------O--------|
  M: ON  V:150  IZQ:+0->150  DER:+0->150
══════════════════════════════════════════════
```

| Campo | Significado |
|-------|-------------|
| `S:`  | Valores calibrados por sensor (0=blanco, 1000=negro) |
| `[##\|..]` | Barra visual: ## = negro, .. = blanco |
| `POS` | Posición de la línea (0=extremo izq, 3500=centro, 7000=extremo der) |
| `ERR` | Error respecto al centro (negativo=línea a la izq, positivo=a la der) |
| `\|--O--|` | Posición visual de la línea en el array |
| `M:`  | Estado motores ON/OFF |
| `IZQ/DER` | offset aplicado → velocidad efectiva |

---

## Notas de la sesión de calibración

- Línea de pista: **2 cm de ancho** (activa 2–3 sensores)
- Altura óptima del array sobre superficie: **3–6 mm**
- Motor izquierdo: **lógica invertida** respecto al derecho
- S8 puede mostrar lecturas bajas (~19–97) en ciertas superficies: ruido normal, umbral de detección en 500 lo filtra
- Cuando todos los sensores leen 1000 = robot sobre zona completamente negra (intersección)
