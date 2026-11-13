# Corrección del Movimiento - Como Minecraft

## 🐛 Problema Anterior

Cuando presionabas **W** y girabas la cámara rápidamente (especialmente 180°), parecía que el jugador se movía de reversa. Esto era porque el vector de movimiento no estaba sincronizado correctamente con la dirección de la cámara.

## ✅ Solución Implementada

### 1. **Separación de Vectores**

Ahora hay DOS funciones para calcular la dirección:

```cpp
Vec3 getForward() const {
    // Para la CÁMARA (incluye pitch para mirar arriba/abajo)
    float yawRad = yaw * 3.14159f / 180.0f;
    float pitchRad = pitch * 3.14159f / 180.0f;
    return Vec3(
        cosf(pitchRad) * sinf(yawRad),
        -sinf(pitchRad),
        cosf(pitchRad) * -cosf(yawRad)
    ).normalize();
}

Vec3 getForwardFlat() const {
    // Para el MOVIMIENTO (solo horizontal, ignora pitch)
    float yawRad = yaw * 3.14159f / 180.0f;
    return Vec3(sinf(yawRad), 0, -cosf(yawRad)).normalize();
}
```

### 2. **Movimiento Usa Vector Plano**

```cpp
Vec3 forward = player.getForwardFlat();  // ✅ Vector plano, no se eleva
Vec3 right = player.getRight();

if (keys['W']) moveDir = moveDir + forward;   // Adelante
if (keys['S']) moveDir = moveDir - forward;   // Reversa
if (keys['D']) moveDir = moveDir + right;     // Derecha
if (keys['A']) moveDir = moveDir - right;     // Izquierda
```

### 3. **Mouse Ajustado**

```cpp
g_gameState->player.yaw += (float)xoffset;  // Suma para consistencia
```

## 🎮 Cómo Funciona Ahora

### Sistema de Coordenadas:
```
        -Z (Norte)
           ↑
           |
-X ← -----[●]----- → +X
(Oeste)    |      (Este)
           ↓
        +Z (Sur)
```

### Yaw (Rotación Horizontal):
- **yaw = 0°** → Mirando Norte (-Z)
- **yaw = 90°** → Mirando Este (+X)
- **yaw = 180°** → Mirando Sur (+Z)
- **yaw = 270°** → Mirando Oeste (-X)

### Cálculo de Vectores:

| Yaw | Forward Flat | Right |
|-----|-------------|-------|
| 0° | (0, 0, -1) | (1, 0, 0) |
| 90° | (1, 0, 0) | (0, 0, 1) |
| 180° | (0, 0, 1) | (-1, 0, 0) |
| 270° | (-1, 0, 0) | (0, 0, -1) |

## 🎯 Comportamiento Corregido

### ✅ ANTES vs AHORA

**ANTES (Problema):**
```
1. Presionas W → Caminas adelante
2. Giras 180° mientras presionas W
3. ❌ Parece que caminas de reversa (vector cambia bruscamente)
```

**AHORA (Correcto):**
```
1. Presionas W → Caminas adelante
2. Giras 180° mientras presionas W
3. ✅ Sigues caminando adelante hacia donde AHORA miras
```

### Ejemplos Prácticos:

#### Ejemplo 1: Girar Mientras Caminas
```
Estado inicial: Mirando Norte, presionando W
→ Te mueves hacia Norte

Giras 90° derecha: Ahora miras Este
→ Te mueves hacia Este (sin parecer reversa)

Giras 180° más: Ahora miras Oeste
→ Te mueves hacia Oeste (suave transición)
```

#### Ejemplo 2: Combinaciones
```
Mirando Norte:
W = Norte, S = Sur, A = Oeste, D = Este

Girando Este (90° derecha):
W = Este, S = Oeste, A = Norte, D = Sur

Girando Sur (180° total):
W = Sur, S = Norte, A = Este, D = Oeste
```

## 📊 Diferencias Clave

| Aspecto | Antes | Ahora |
|---------|-------|-------|
| Vector movimiento | getForward() proyectado | getForwardFlat() directo |
| Componente Y | Eliminado manualmente | Nunca incluido |
| Sincronización | A veces inconsistente | Siempre consistente |
| Yaw mouse | Restaba (invertido) | Suma (correcto) |

## 🚀 Resultado Final

Ahora el movimiento funciona **exactamente como Minecraft**:

- ✅ **W** siempre te mueve hacia donde miras
- ✅ **S** siempre es reversa (opuesto a donde miras)
- ✅ **A/D** siempre perpendiculares a tu vista
- ✅ Girar rápido NO causa movimiento inverso
- ✅ Mirar arriba/abajo NO afecta el movimiento horizontal
- ✅ Combinaciones funcionan suavemente (W+A, W+D, etc.)

## 🎮 Prueba Esto:

1. **Presiona W** → Caminas adelante
2. **Gira 180° con mouse mientras mantienes W presionado**
3. **Resultado:** Sigues caminando adelante (hacia la nueva dirección)
4. **NO hay sensación de ir de reversa** ✅

---

**¡Movimiento corregido! Ahora funciona como Minecraft.** 🎮
