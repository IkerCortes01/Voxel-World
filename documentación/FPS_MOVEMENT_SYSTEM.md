# Sistema de Movimiento FPS Estándar

## 📐 Implementación Basada en Principios Públicos

Este sistema usa principios matemáticos estándar de la industria de videojuegos para movimiento de cámara primera persona (FPS).

## 🎯 Vectores de Movimiento

### Sistema de Coordenadas:
```
     -Z (Norte)
        ↑
        |
-X ← --[●]-- → +X
(Oeste) |    (Este)
        ↓
     +Z (Sur)
```

### Funciones de Dirección:

```cpp
Vec3 getMovementForward() const {
    // Forward solo usa yaw (rotación horizontal)
    float rad = yaw * PI / 180.0f;
    return Vec3(-sin(rad), 0, -cos(rad));
}

Vec3 getMovementRight() const {
    // Right es perpendicular a forward (90° derecha)
    float rad = yaw * PI / 180.0f;
    return Vec3(cos(rad), 0, -sin(rad));
}
```

### Tabla de Direcciones:

| Yaw | Forward | Right | Descripción |
|-----|---------|-------|-------------|
| 0° | (0, 0, -1) | (1, 0, 0) | Mirando Norte |
| 90° | (-1, 0, 0) | (0, 0, -1) | Mirando Oeste |
| 180° | (0, 0, 1) | (-1, 0, 0) | Mirando Sur |
| 270° | (1, 0, 0) | (0, 0, 1) | Mirando Este |

## 🎮 Mapeo de Teclas:

```cpp
Vec3 moveDir(0, 0, 0);
if (keys['W']) moveDir += forward;   // Adelante
if (keys['S']) moveDir -= forward;   // Reversa
if (keys['D']) moveDir += right;     // Derecha
if (keys['A']) moveDir -= right;     // Izquierda

// Normalizar para velocidad constante en diagonales
if (moveDir.length() > 0) {
    moveDir = moveDir.normalize();
    velocity.x = moveDir.x * WALK_SPEED;
    velocity.z = moveDir.z * WALK_SPEED;
}
```

## ✅ Características:

1. **Movimiento relativo a yaw** (rotación horizontal)
2. **Pitch no afecta movimiento** (mirar arriba/abajo no cambia dirección)
3. **Normalización de diagonales** (W+A misma velocidad que W solo)
4. **Vectores perpendiculares** (A/D siempre 90° respecto a W)
5. **Separación cámara/movimiento** (getForward para cámara, getMovementForward para WASD)

## 📊 Combinaciones de Teclas:

| Input | Ángulo | Dirección |
|-------|--------|-----------|
| W | 0° | Forward |
| W+D | 45° | Forward + Right |
| D | 90° | Right |
| S+D | 135° | -Forward + Right |
| S | 180° | -Forward |
| S+A | 225° | -Forward - Right |
| A | 270° | -Right |
| W+A | 315° | Forward - Right |

## 🔧 Principios Matemáticos:

### Rotación 2D en el plano XZ:
```
x' = x * cos(θ) - z * sin(θ)
z' = x * sin(θ) + z * cos(θ)
```

### Forward Vector (θ = yaw):
```
forward.x = -sin(yaw)
forward.y = 0
forward.z = -cos(yaw)
```

### Right Vector (θ = yaw + 90°):
```
right.x = cos(yaw)
right.y = 0
right.z = -sin(yaw)
```

## 🎮 Comportamiento:

- ✅ W siempre adelante (relativo a yaw)
- ✅ S siempre reversa
- ✅ A/D siempre lateral
- ✅ Girar cámara actualiza direcciones instantáneamente
- ✅ No hay "efecto reversa" al girar
- ✅ Pitch no afecta movimiento horizontal

---

**Sistema FPS estándar implementado correctamente.** 🎮
