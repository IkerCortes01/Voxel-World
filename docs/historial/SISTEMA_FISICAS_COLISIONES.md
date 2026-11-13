# Sistema de Físicas y Colisiones Mejorado - Voxel World

## 🎯 Problema Resuelto

### **ANTES (Problema):**
```
Jugador caminando diagonalmente hacia una esquina:
    [Bloque A]  [Bloque B]
         ║
         ║  ← Jugador ATASCADO (no puede pasar)
         ║
      [Suelo]    [Suelo]
```
❌ El jugador se quedaba **completamente detenido** al tocar esquinas
❌ No podías moverte entre bloques cercanos
❌ Las colisiones eran imprecisas

### **AHORA (Solucionado):**
```
Jugador caminando diagonalmente hacia una esquina:
    [Bloque A]  [Bloque B]
         ↘
          →  ← Jugador SE DESLIZA (sliding)
         ↗
      [Suelo]    [Suelo]
```
✅ El jugador **se desliza** suavemente a lo largo de las paredes
✅ Puedes moverte entre bloques en esquinas
✅ Colisiones AABB precisas

---

## 🔧 Mejoras Implementadas

### **1. Estructura AABB (Axis-Aligned Bounding Box)**

```cpp
struct AABB {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    // Verifica intersección precisa entre dos cajas
    bool intersects(const AABB& other) const;

    // Expande la caja por un margen
    AABB expand(float margin) const;
};
```

**Ventajas:**
- ✅ Detección precisa de colisiones
- ✅ Representación clara de límites
- ✅ Fácil de debuggear

---

### **2. Hitbox del Jugador Optimizada**

```cpp
const float WIDTH = 0.5f;        // Ancho de la hitbox
const float HEIGHT = 1.8f;       // Altura total
const float EYE_HEIGHT = 1.62f;  // Altura de la cámara
```

**Cambios:**
- **WIDTH:** 0.6f → 0.5f (más estrecho)
- **Ventaja:** Permite pasar más fácilmente entre bloques

**Visualización:**
```
Vista Superior del Jugador:

    0.5 bloques ancho
    ←─────→
    ┌─────┐
    │     │ ← Hitbox del jugador (0.5 x 0.5)
    │  ●  │    Centro = posición
    │     │
    └─────┘
```

**Vista Lateral:**
```
    1.8 bloques altura
    ↕
    ┌─────┐
    │     │
    │  👁  │ ← Ojo (1.62 desde el suelo)
    │     │
    │  ●  │ ← Centro del jugador
    │     │
    └─────┘
    Suelo
```

---

### **3. Sistema de Sliding (Deslizamiento)**

#### **Algoritmo:**

```
1. Intentar movimiento completo (X, Y, Z simultáneamente)
   ├─ Si NO hay colisión → Mover libremente ✓
   └─ Si HAY colisión → Activar sliding ↓

2. Probar cada eje individualmente:
   ├─ Eje X: ¿Puedo mover solo en X?
   ├─ Eje Y: ¿Puedo mover solo en Y? (gravedad/salto)
   └─ Eje Z: ¿Puedo mover solo en Z?

3. Aplicar movimientos permitidos

4. Probar combinaciones diagonales:
   ├─ Si X bloqueado pero Z libre → Intentar deslizar en Z
   └─ Si Z bloqueado pero X libre → Intentar deslizar en X
```

#### **Ejemplo Práctico:**

**Caso 1: Esquina de Pared**
```
Vista Superior:

Bloque █████
        █
        █  ↘ Jugador va diagonal (SE)
        █
Bloque █████

Resultado:
1. Movimiento diagonal bloqueado
2. Sistema detecta: X bloqueado, Z libre
3. Jugador SE DESLIZA hacia Z (sur)
4. Movimiento suave y natural ✓
```

**Caso 2: Pasillo Estrecho**
```
█████ █████
█     ○     █  ← Jugador (○) puede pasar
█████ █████

Hitbox: 0.5 bloques
Espacio: 2.0 bloques (1 bloque de aire entre paredes)
Resultado: ✓ Pasa cómodamente
```

---

### **4. Detección de Colisiones Mejorada**

```cpp
bool checkAABBCollision(const Vec3& pos, float width, float height, World& world) {
    AABB playerBox = getPlayerAABB(pos, width, height);

    // Solo verificar bloques cercanos (optimización)
    int minX = floor(playerBox.minX - 0.1f);
    int maxX = floor(playerBox.maxX + 0.1f);
    // ... mismo para Y, Z

    // Verificar intersección AABB precisa
    for (cada bloque cercano) {
        if (bloque sólido) {
            AABB blockBox = getBlockAABB(x, y, z);
            if (playerBox.intersects(blockBox)) {
                return true; // Colisión detectada
            }
        }
    }

    return false; // Sin colisión
}
```

**Mejoras:**
- ✅ Solo verifica bloques cercanos (eficiente)
- ✅ Intersección AABB matemáticamente precisa
- ✅ Margen de 0.1 para evitar errores de punto flotante

---

## 📊 Comparación Técnica

| Característica | ANTES | AHORA |
|----------------|-------|-------|
| **Detección de colisión** | floor/ceil impreciso | AABB preciso |
| **Hitbox WIDTH** | 0.6 bloques | 0.5 bloques |
| **Sliding en esquinas** | ❌ No | ✅ Sí |
| **Movimiento diagonal** | Se detiene | Se desliza |
| **Precisión** | Baja | Alta |
| **Sensación** | Trabado | Suave |

---

## 🎮 Casos de Uso Resueltos

### **1. Pasar entre Bloques en Esquina**
```
ANTES:
█████
█     [X] ← Atascado
█████ █████

AHORA:
█████
█     [→] ← Se desliza
█████ █████
```

### **2. Caminar Pegado a Paredes**
```
ANTES: Te detienes al tocar la pared diagonalmente

AHORA: Te deslizas suavemente a lo largo de la pared
```

### **3. Saltar Cerca de Bloques**
```
ANTES: Chocar con bloque lateral cancelaba salto completo

AHORA: Puedes saltar verticalmente incluso tocando paredes laterales
```

### **4. Movimiento en Cuevas/Túneles**
```
ANTES: Difícil navegar en espacios estrechos

AHORA: Movimiento fluido incluso en espacios de 1 bloque de ancho
```

---

## 🔬 Detalles Técnicos

### **Hitbox del Jugador:**

```cpp
AABB getPlayerAABB(const Vec3& pos, float width, float height) {
    float halfWidth = width / 2.0f;  // 0.25 bloques desde el centro
    return AABB(
        pos.x - halfWidth,  // minX
        pos.y,              // minY (pies del jugador)
        pos.z - halfWidth,  // minZ
        pos.x + halfWidth,  // maxX
        pos.y + height,     // maxY (cabeza del jugador)
        pos.z + halfWidth   // maxZ
    );
}
```

### **Hitbox de Bloque:**

```cpp
AABB getBlockAABB(int x, int y, int z) {
    return AABB(
        (float)x,       // minX
        (float)y,       // minY
        (float)z,       // minZ
        (float)x + 1.0f, // maxX
        (float)y + 1.0f, // maxY
        (float)z + 1.0f  // maxZ
    );
}
```

### **Intersección AABB:**

```cpp
bool intersects(const AABB& other) const {
    return (minX < other.maxX && maxX > other.minX) &&  // X se sobrepone
           (minY < other.maxY && maxY > other.minY) &&  // Y se sobrepone
           (minZ < other.maxZ && maxZ > other.minZ);    // Z se sobrepone
}
```

---

## 🎯 Comportamiento del Sistema

### **Flujo de Actualización de Físicas:**

```
1. Aplicar gravedad (si no está en el suelo)
   ↓
2. Calcular dirección de movimiento (WASD)
   ↓
3. Aplicar velocidad de caminar/correr
   ↓
4. Procesar salto (Espacio + onGround)
   ↓
5. Calcular posición deseada (pos + velocity * deltaTime)
   ↓
6. Verificar colisiones:
   ├─ Sin colisión → Mover libremente
   └─ Con colisión → ACTIVAR SLIDING
       ├─ Probar eje X solo
       ├─ Probar eje Y solo
       ├─ Probar eje Z solo
       ├─ Aplicar movimientos válidos
       └─ Probar combinaciones diagonales
   ↓
7. Actualizar posición final
   ↓
8. Verificar onGround (para permitir salto)
```

---

## 🚀 Ejemplos de Código

### **Ejemplo 1: Crear AABB Personalizado**

```cpp
// AABB de una entidad más pequeña
AABB smallEntityBox(
    playerPos.x - 0.25f, playerPos.y, playerPos.z - 0.25f,
    playerPos.x + 0.25f, playerPos.y + 1.0f, playerPos.z + 0.25f
);

// Verificar colisión
if (smallEntityBox.intersects(blockBox)) {
    // Hay colisión
}
```

### **Ejemplo 2: Expandir Hitbox Temporalmente**

```cpp
// Expandir hitbox por 0.1 bloques (para detección temprana)
AABB expandedBox = playerBox.expand(0.1f);

if (expandedBox.intersects(dangerZone)) {
    // Advertir al jugador
}
```

---

## 📈 Mejoras Futuras Posibles

El sistema está preparado para:

1. 🏃 **Sprint:** Aumentar WALK_SPEED cuando se mantiene Shift
2. 🐌 **Agacharse:** Reducir HEIGHT temporalmente
3. 🏊 **Nadar:** Física diferente en agua
4. 🪜 **Escalar:** Permitir trepar bloques
5. 🎿 **Deslizamiento en hielo:** Reducir fricción en ciertos bloques
6. 🌊 **Empuje del agua:** Aplicar fuerzas laterales
7. 🪂 **Paracaídas/Planeo:** Reducir velocidad de caída
8. 🦘 **Doble salto:** Permitir segundo salto en el aire

---

## ✅ Resumen de Mejoras

### **Problemas Resueltos:**
- ✅ Ya no te atascas en esquinas
- ✅ Movimiento suave y natural como Minecraft
- ✅ Puedes pasar entre bloques cercanos
- ✅ Colisiones precisas matemáticamente
- ✅ Hitbox optimizada (más estrecha)

### **Características Añadidas:**
- ✅ Sistema de sliding completo
- ✅ Detección AABB precisa
- ✅ Resolución de colisiones por eje
- ✅ Combinaciones diagonales
- ✅ Código bien estructurado y documentado

### **Rendimiento:**
- ✅ Eficiente (solo verifica bloques cercanos)
- ✅ Sin impacto negativo en FPS
- ✅ Escalable para más entidades

---

## 🎊 Conclusión

El sistema de físicas y colisiones ha sido **completamente renovado** para proporcionar:

- 🎮 **Movimiento fluido** como juegos AAA
- 🏃 **Navegación natural** en cualquier terreno
- 🎯 **Colisiones precisas** sin bugs visuales
- ⚡ **Rendimiento óptimo** sin sacrificar calidad

**¡Las físicas ahora son suaves y precisas como Minecraft!** 🎮✨
