# Sistema de Semillas Aleatorias - Voxel World

## 🌱 ¿Qué son las Semillas?

Las **semillas** (seeds) son números que determinan cómo se genera el mundo. Con el mismo número de semilla, siempre se generará el mismo mundo exacto.

---

## 🎲 Cómo Funciona

### **Generación Automática:**
Cada vez que inicias el juego, se genera automáticamente una **semilla aleatoria única** basada en el tiempo actual (milisegundos desde epoch).

### **Visualización de la Semilla:**

#### **1. En Consola (al iniciar):**
```
======================================
  VOXEL WORLD - SANDBOX INFINITO
======================================
Seed del mundo: 1735689234
Guarda esta semilla para regenerar este mundo!
======================================
```

#### **2. En el Título de la Ventana:**
```
Voxel World - Sandbox Infinito | FPS: 60 | Pos: 3.2, 65.0, -15.7 | Seed: 1735689234
```

---

## 🔧 Cómo Usar una Semilla Específica

Para regenerar un mundo específico con una semilla conocida:

### **Opción 1: Modificar el Código (Temporal)**

En `main.cpp`, línea ~964, modifica:

```cpp
// ANTES (semilla aleatoria):
g_gameState = new GameState();

// DESPUÉS (semilla específica):
// Primero crea el GameState con constructor por defecto
g_gameState = new GameState();
// Luego reemplaza el World con tu semilla
g_gameState->world = World(1735689234);  // Usar tu semilla aquí
```

### **Opción 2: Modificar GameState (Permanente)**

En `main.cpp`, línea ~862, en el constructor de GameState:

```cpp
// ANTES:
struct GameState {
    Player player;
    World world;  // Usa semilla aleatoria por defecto
    // ...
};

// DESPUÉS:
struct GameState {
    Player player;
    World world = World(1735689234);  // Tu semilla específica aquí
    // ...
};
```

---

## 📊 Características del Sistema

### ✅ **Ventajas:**

1. **Reproducibilidad:** El mismo seed genera el mismo mundo exacto
2. **Compartible:** Puedes compartir seeds con otros jugadores
3. **Exploración:** Encuentra mundos interesantes y guarda sus seeds
4. **Debugging:** Útil para probar características en el mismo mundo

### 🎯 **Ejemplos de Seeds:**

| Seed | Descripción |
|------|-------------|
| `12345` | Seed de prueba por defecto |
| `-1` | Genera semilla aleatoria (valor especial) |
| `1000000` | Seed numérico cualquiera |

---

## 🔬 Detalles Técnicos

### **Algoritmo de Generación:**

```cpp
static int generarSemillaAleatoria() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return static_cast<int>(millis % 2147483647); // Limitar a int positivo
}
```

### **Rango de Semillas:**
- **Mínimo:** 0
- **Máximo:** 2,147,483,647 (INT_MAX)

### **Uso en Perlin Noise:**

La semilla se usa para inicializar el **generador de Perlin Noise**, que controla:
- 🏔️ Altura del terreno (continentalidad)
- 🌊 Erosión y detalles
- 🏔️ Picos y colinas
- 🌳 Distribución de árboles
- 🌿 Colocación de hierba
- 🏖️ Ubicación de playas

---

## 🎮 Casos de Uso

### **1. Exploración Normal:**
```
Ejecutar juego → Ver seed en consola → Explorar
Si te gusta el mundo → Guardar el seed
```

### **2. Compartir Mundos:**
```
Jugador A: "Mi seed es 1234567, tiene montañas increíbles!"
Jugador B: Usa seed 1234567 → Explora el mismo mundo
```

### **3. Speedruns/Desafíos:**
```
Establecer un seed fijo para competir en el mismo mundo
```

### **4. Testing/Desarrollo:**
```
Usar seed fijo para probar nuevas características en el mismo terreno
```

---

## 🚀 Futuras Expansiones Posibles

El sistema de semillas está preparado para:

- 📁 **Guardar seed con el mundo** (en world.dat)
- 🎨 **Interfaz para ingresar seed** (menú de inicio)
- 🔍 **Buscador de seeds** (encontrar mundos con características específicas)
- 🌍 **Seeds preestablecidos** (mundos diseñados especialmente)
- 📊 **Análisis de seeds** (detectar qué seeds tienen más árboles, montañas, etc.)

---

## ✨ Ejemplo de Sesión

```
$ ./VoxelWorld.exe

======================================
  VOXEL WORLD - SANDBOX INFINITO
======================================
Seed del mundo: 1735689234
Guarda esta semilla para regenerar este mundo!
======================================

[El jugador explora y encuentra un mundo increíble con montañas altas]

[Ventana muestra: "... | Seed: 1735689234"]

Jugador: "¡Este mundo es perfecto! Voy a guardar el seed: 1735689234"

[Más tarde, para volver al mismo mundo, usa el seed 1735689234]
```

---

## 🎯 Conclusión

El sistema de semillas aleatorias está **completamente funcional** y permite:
- ✅ Generar mundos únicos en cada sesión
- ✅ Reproducir mundos específicos usando seeds
- ✅ Compartir seeds entre jugadores
- ✅ Visualizar la seed actual en todo momento

**¡Explora mundos infinitos con semillas únicas!** 🌍🎮
