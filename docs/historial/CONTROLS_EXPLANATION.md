# Sistema de Controles - Voxel World

## 🎮 Movimiento Sincronizado con Cámara

### Cómo Funciona:

El sistema calcula el movimiento **relativo a donde miras**, pero solo en el plano horizontal (no vuelas al mirar arriba/abajo).

```cpp
Vec3 forward = player.getForward();
forward.y = 0;  // Elimina componente vertical
forward = forward.normalize();
Vec3 right = player.getRight();

Vec3 moveDir(0, 0, 0);
if (keys['W']) moveDir = moveDir + forward;   // Adelante
if (keys['S']) moveDir = moveDir - forward;   // Reversa
if (keys['D']) moveDir = moveDir + right;     // Derecha
if (keys['A']) moveDir = moveDir - right;     // Izquierda
```

## 📐 Vectores de Dirección

### Forward (Adelante):
- Calculado según el **yaw** (rotación horizontal)
- Proyectado al suelo (y = 0)
- **W** suma este vector
- **S** resta este vector (reversa)

### Right (Derecha):
- Perpendicular a Forward (90° a la derecha)
- Siempre horizontal
- **D** suma este vector
- **A** resta este vector

## 🔄 Combinaciones de Teclas

Todas las teclas se pueden combinar porque suman al mismo vector `moveDir`:

| Combinación | Resultado |
|-------------|-----------|
| **W solo** | Adelante hacia donde miras |
| **S solo** | Reversa (atrás) |
| **A solo** | Izquierda lateral |
| **D solo** | Derecha lateral |
| **W + A** | Diagonal adelante-izquierda ↖ |
| **W + D** | Diagonal adelante-derecha ↗ |
| **S + A** | Diagonal atrás-izquierda ↙ |
| **S + D** | Diagonal atrás-derecha ↘ |

## 📊 Ejemplos Visuales

### Mirando Norte (yaw = 0):
```
        W (Norte)
         ↑
    A ← [●] → D
         ↓
        S (Sur)
```

### Mirando Este (yaw = 90):
```
        W (Este)
         ↑
    A ← [●] → D
    (Norte)   (Sur)
         ↓
     S (Oeste)
```

### Mirando Sur (yaw = 180):
```
        W (Sur)
         ↑
    A ← [●] → D
         ↓
      S (Norte)
```

## 🎯 Comportamiento Clave

### ✅ LO QUE SÍ HACE:
1. **W siempre va hacia donde miras** (alineado con cámara)
2. **S siempre va en reversa** (opuesto a donde miras)
3. **A/D siempre perpendiculares** a tu vista
4. **Combinaciones funcionan** (W+A = diagonal)
5. **NO vuelas** al mirar arriba/abajo (forward.y = 0)

### ❌ LO QUE NO HACE:
1. **NO es movimiento absoluto** (WASD no son siempre Norte/Sur/Este/Oeste)
2. **NO te mueves hacia arriba/abajo** con WASD (solo con gravedad/salto)

## 🔧 Normalización

Cuando combinas teclas, el vector se normaliza para mantener la misma velocidad:

```cpp
if (moveDir.length() > 0) {
    moveDir = moveDir.normalize();  // Mantiene velocidad constante
    player.velocity.x = moveDir.x * player.WALK_SPEED;
    player.velocity.z = moveDir.z * player.WALK_SPEED;
}
```

**Resultado:** 
- W solo = velocidad 4.3 m/s
- W+D diagonal = velocidad 4.3 m/s (NO más rápido)

## 🎮 Comparación con Minecraft

| Característica | Minecraft | Voxel World |
|----------------|-----------|-------------|
| W hacia cámara | ✅ | ✅ |
| S reversa | ✅ | ✅ |
| A/D lateral | ✅ | ✅ |
| Combinaciones | ✅ | ✅ |
| No volar mirando arriba | ✅ | ✅ |
| Velocidad normalizada | ✅ | ✅ |

---

**¡Sistema de controles 100% funcional como Minecraft!** 🎮
