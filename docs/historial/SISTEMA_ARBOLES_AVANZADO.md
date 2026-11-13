# Sistema de Árboles Avanzado - Voxel World

## 🌳 Tamaños de Árboles Implementados

El sistema ahora genera tres tipos de árboles con diferentes alturas y copas:

---

## 📏 Especificaciones de Árboles

### **1. Árbol PEQUEÑO (Tipo 0)**

#### Características:
- **Altura del tronco:** 4 bloques (FIJO, no varía)
- **Distribución:** ~50% de los árboles generados
- **Hojas:** Compactas, concentradas en la parte superior

#### Estructura Visual:
```
    [Hojas]              ← y=5 (1 bloque arriba del tronco)
  [Hojas][Hojas]         ← y=4 (tope del tronco)
  [Hojas][Hojas]         ← y=3
   [H][Tronco][H]        ← y=2 (muy pocas hojas en base)
      [Tronco]           ← y=1
      [Tronco]           ← y=0 (base, SIN hojas)
```

#### Distribución de Hojas:
- **Base (y=0-1):** Sin hojas
- **Medio (y=2):** Pocas hojas (solo en los lados N/S/E/W, no en esquinas)
- **Medio-alto (y=3):** Anillo completo de hojas
- **Copa (y=4):** Radio 2 bloques
- **Punta (y=5):** Radio 1 bloque

---

### **2. Árbol MEDIANO (Tipo 1)**

#### Características:
- **Altura del tronco:** 10-14 bloques (RANDOM)
- **Distribución:** ~35% de los árboles generados
- **Hojas:** Progresivas, crecen en radio hacia arriba

#### Estructura Visual (ejemplo con 12 bloques):
```
     [Hojas]                    ← y=14 (radio 1)
   [Hojas][Hojas]               ← y=13 (radio 2)
 [Hojas][Hojas][Hojas]          ← y=12 (radio 3, tope del tronco)
 [Hojas][Tronco][Hojas]         ← y=11 (radio 3)
 [Hojas][Tronco][Hojas]         ← y=10 (radio 3)
 [Hojas][Tronco][Hojas]         ← y=9 (radio 3)
  [H][Tronco][H]                ← y=8 (radio 2)
  [H][Tronco][H]                ← y=7 (radio 2)
   [Tronco]                     ← y=6 (radio 1)
   [Tronco]                     ← y=5 (radio 1)
   [Tronco]                     ← y=4 (radio 1)
   [Tronco]                     ← y=3 (SIN hojas, muy pocas)
   [Tronco]                     ← y=2 (SIN hojas)
   [Tronco]                     ← y=1 (SIN hojas)
   [Tronco]                     ← y=0 (base, SIN hojas)
```

#### Distribución de Hojas:
- **Base (y=0-2):** Sin hojas
- **Parte baja (y=3-5):** Sin hojas o muy pocas (solo desde y=3)
- **Parte media (y=6-8):** Radio 1-2 bloques
- **Parte alta (y=9-11):** Radio 3 bloques
- **Copa superior (3 capas):** Radios 3 → 2 → 1

---

### **3. Árbol GRANDE (Tipo 2)**

#### Características:
- **Altura del tronco:** 23-30 bloques (RANDOM)
- **Distribución:** ~15% de los árboles generados (RAROS)
- **Hojas:** Copa masiva, extremadamente densa

#### Estructura Visual (ejemplo con 26 bloques):
```
        [Hojas]                        ← y=30 (radio 2)
      [Hojas][Hojas]                   ← y=29 (radio 3)
    [Hojas][Hojas][Hojas]              ← y=28 (radio 4)
  [Hojas][Hojas][Hojas][Hojas]         ← y=27 (radio 5)
[Hojas][Hojas][Hojas][Hojas][Hojas]    ← y=26 (radio 6, tope del tronco)
[Hojas][Hojas][Tronco][Hojas][Hojas]   ← y=25 (radio 6)
[Hojas][Hojas][Tronco][Hojas][Hojas]   ← y=24 (radio 6)
[Hojas][Hojas][Tronco][Hojas][Hojas]   ← y=23 (radio 6)
  [Hojas][Tronco][Hojas]               ← y=22 (radio 5)
  [Hojas][Tronco][Hojas]               ← y=21 (radio 5)
    [H][Tronco][H]                     ← y=20 (radio 4)
    [H][Tronco][H]                     ← y=19 (radio 4)
    [H][Tronco][H]                     ← y=18 (radio 4)
    [H][Tronco][H]                     ← y=17 (radio 4)
    [H][Tronco][H]                     ← y=16 (radio 4)
    [H][Tronco][H]                     ← y=15 (radio 4)
    [H][Tronco][H]                     ← y=14 (radio 3)
     [Tronco]                          ← y=13 (radio 3)
     [Tronco]                          ← y=12 (radio 3)
     [Tronco]                          ← y=11 (radio 3)
     [Tronco]                          ← y=10 (radio 2)
     [Tronco]                          ← y=9 (radio 2)
     [Tronco]                          ← y=8 (radio 2)
     [Tronco]                          ← y=7 (radio 2)
     [Tronco]                          ← y=6 (radio 2)
     [Tronco]                          ← y=5 (SIN hojas, empieza desde y=5)
     [Tronco]                          ← y=4 (SIN hojas)
     [Tronco]                          ← y=3 (SIN hojas)
     [Tronco]                          ← y=2 (SIN hojas)
     [Tronco]                          ← y=1 (SIN hojas)
     [Tronco]                          ← y=0 (base, SIN hojas)
```

#### Distribución de Hojas:
- **Base (y=0-4):** Sin hojas
- **Parte baja (y=5-8):** Sin hojas o muy pocas
- **Parte media-baja (y=9-14):** Radio 2-3 bloques
- **Parte media (y=15-20):** Radio 4 bloques
- **Parte alta (y=21-25):** Radio 5-6 bloques
- **Copa superior (5 capas):** Radios 6 → 5 → 4 → 3 → 2

---

## 📊 Tabla Comparativa

| Característica | Pequeño | Mediano | Grande |
|----------------|---------|---------|--------|
| **Altura tronco** | 4 bloques (fijo) | 10-14 bloques | 23-30 bloques |
| **Variación altura** | ❌ No | ✅ Sí (random) | ✅ Sí (random) |
| **Radio máximo hojas** | 2 bloques | 3 bloques | 6 bloques |
| **Hojas en base** | Muy pocas (y≥2) | Muy pocas (y≥3) | Muy pocas (y≥5) |
| **Capas copa superior** | 2 capas | 3 capas | 5 capas |
| **Probabilidad** | ~50% | ~35% | ~15% |
| **Bloques de hojas** | ~20-30 | ~100-150 | ~400-600 |
| **Bloques totales** | ~24-34 | ~110-164 | ~423-630 |

---

## 🎲 Sistema de Generación

### **Algoritmo de Selección:**

```cpp
float treeNoise = noise.octaveNoise(worldX * 0.1f, 0, worldZ * 0.1f, 1);

if (treeNoise > 0.85f) { // ~7.5% de probabilidad de árbol
    float sizeNoise = noise.octaveNoise(worldX * 0.05f, 50, worldZ * 0.05f, 1);

    if (sizeNoise < 0.0f) {
        tipoArbol = 0; // Pequeño (50%)
    } else if (sizeNoise < 0.7f) {
        tipoArbol = 1; // Mediano (35%)
    } else {
        tipoArbol = 2; // Grande (15%)
    }

    generarArbol(worldX, terrainHeight + 1, worldZ, tipoArbol, alturaVariante);
}
```

### **Distribución Estadística:**

En un área de 1000 árboles:
- **Pequeños:** ~500 árboles (4 bloques cada uno)
- **Medianos:** ~350 árboles (10-14 bloques cada uno)
- **Grandes:** ~150 árboles (23-30 bloques cada uno)

---

## 🌿 Reglas de Colocación de Hojas

### **Principios Generales:**

1. **Nunca reemplazar el tronco central** (dx=0, dz=0)
2. **Base del árbol sin hojas** (primeros 2-5 bloques según tamaño)
3. **Radio progresivo hacia arriba** (crece conforme subes)
4. **Copa densa en la parte superior** (múltiples capas arriba del tronco)
5. **Solo reemplazar BLOCK_AIR** (no reemplazar otros bloques)

### **Cálculo de Radio por Altura:**

#### Pequeño:
- Base (y=2): Radio 1 (solo N/S/E/W, no esquinas)
- Medio (y=3): Radio 1 (completo)
- Copa (y=4): Radio 2
- Punta (y=5): Radio 1

#### Mediano:
- Base (y=0-2): Sin hojas
- Baja (y=3-5): Radio 1
- Media (y=6 hasta altura/2): Radio 1
- Alta (altura/2 hasta 3altura/4): Radio 2
- Muy alta (3altura/4 hasta tope): Radio 3
- Copa superior: 3 → 2 → 1

#### Grande:
- Base (y=0-4): Sin hojas
- Baja (y=5 hasta altura/3): Radio 2
- Media (altura/3 hasta altura/2): Radio 3
- Media-alta (altura/2 hasta 2altura/3): Radio 4
- Alta (2altura/3 hasta 5altura/6): Radio 5
- Muy alta (5altura/6 hasta tope): Radio 6
- Copa superior: 6 → 5 → 4 → 3 → 2

---

## 🔧 Función `generarArbol()`

### **Parámetros:**

```cpp
void generarArbol(int worldX, int baseY, int worldZ, int tipoArbol, int alturaVariante = 0)
```

- **worldX, worldZ:** Coordenadas del mundo donde colocar el árbol
- **baseY:** Altura del suelo (el tronco empieza en baseY)
- **tipoArbol:** Tipo de árbol (0=Pequeño, 1=Mediano, 2=Grande)
- **alturaVariante:** Semilla para la variación de altura (solo afecta medianos y grandes)

### **Cálculo de Altura:**

```cpp
if (tipoArbol == 0) {
    altura = 4; // Siempre 4
} else if (tipoArbol == 1) {
    altura = 10 + (alturaVariante % 5); // 10, 11, 12, 13, 14
} else if (tipoArbol == 2) {
    altura = 23 + (alturaVariante % 8); // 23-30
}
```

---

## 🎯 Casos de Uso

### **1. Exploración:**
Busca árboles grandes (son raros, ~15%) para orientarte en el mundo.

### **2. Recursos de Madera:**
- **Pequeños:** Rápido (~4 bloques)
- **Medianos:** Moderado (~12 bloques)
- **Grandes:** Lento pero abundante (~27 bloques)

### **3. Construcción:**
Usa árboles grandes como puntos de referencia o estructuras naturales.

### **4. Supervivencia:**
Trepa árboles grandes para ver más lejos (23-30 bloques de altura).

---

## 📈 Estadísticas de Generación

### **Por Chunk (16x16 bloques):**

- **Área de terreno:** 256 bloques de superficie
- **Probabilidad de árbol:** ~7.5% por bloque de pasto
- **Árboles esperados por chunk:** ~15-25 árboles (según cantidad de pasto)

### **Distribución Típica por Chunk:**

| Tipo | Cantidad | Bloques de Madera | Bloques de Hojas |
|------|----------|-------------------|------------------|
| Pequeños | ~8-12 | ~32-48 | ~160-360 |
| Medianos | ~5-9 | ~60-126 | ~500-1350 |
| Grandes | ~2-4 | ~46-120 | ~800-2400 |
| **TOTAL** | ~15-25 | ~138-294 | ~1460-4110 |

---

## ✅ Características Implementadas

### **Listo:**
- ✅ Tres tamaños de árboles con alturas específicas
- ✅ Árboles pequeños con altura fija (4 bloques)
- ✅ Árboles medianos con altura random (10-14 bloques)
- ✅ Árboles grandes con altura random (23-30 bloques)
- ✅ Distribución de hojas concentrada en parte superior
- ✅ Base del tronco sin hojas o con muy pocas
- ✅ Radio progresivo de hojas hacia arriba
- ✅ Copa densa en la parte superior
- ✅ Sistema de generación basado en Perlin Noise
- ✅ Distribución probabilística (50% / 35% / 15%)

### **Futuras Mejoras Posibles:**
- 🔄 Diferentes especies de árboles (roble, pino, abedul)
- 🔄 Árboles caídos (troncos horizontales)
- 🔄 Árboles muertos (sin hojas)
- 🔄 Arbustos pequeños (1-2 bloques)
- 🔄 Árboles con ramas laterales
- 🔄 Frutos en árboles (manzanas, etc.)

---

## 🎮 Cómo Probar

1. **Ejecuta el juego:** `build/bin/Release/VoxelWorld.exe`
2. **Busca terreno con pasto:** Los árboles solo generan en BLOCK_GRASS
3. **Explora:** Verás una mezcla de árboles pequeños, medianos y grandes
4. **Identifica tamaños:**
   - Pequeños: Altura hasta tu cabeza (~4-5 bloques)
   - Medianos: Altura 2-3 veces tu altura (~10-14 bloques)
   - Grandes: Árboles masivos que destacan en el horizonte (~23-30 bloques)

---

## 🌳 Conclusión

El sistema de árboles ahora genera tres tamaños distintos con distribución realista de hojas:

- 🌱 **Pequeños:** Compactos y abundantes (50%)
- 🌲 **Medianos:** Árboles estándar (35%)
- 🌴 **Grandes:** Árboles majestuosos y raros (15%)

**Las hojas se concentran en la parte superior e interior del tronco, dejando la base limpia como en árboles reales.** 🎮✨
