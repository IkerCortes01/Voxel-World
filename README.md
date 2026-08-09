# Voxel World — alpha 1.0.0

Es un juego independiente, hecho en su propio motor y con sistemas propios.

Motor voxel en C++20 con OpenGL, tipo sandbox de bloques.

---

## Compilar

Requiere Visual Studio 2022 (o cualquier toolchain con soporte C++20) y CMake 3.20+.

```batch
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

El ejecutable queda en `build\bin\Release\VoxelWorld.exe`.

GLFW viene incluido como código fuente en `external/glfw`, así que no hay
que instalar dependencias externas.

---

## Estructura

```
src/
  main.cpp            Motor: render, mundo, chunks, UI, guardado
  terrain/            Generación procedural de terreno (modular)
  player/             Character controller (modular)
  audio/              Mezclador de sonido y síntesis de pasos
  ui/                 Texturas de item, renderizado 2D e iconos
  ChunkSystem.cpp     Sistema de chunks
  SaveSystem.cpp      Persistencia de mundos
  PalettedStorage.h   Almacenamiento comprimido por paletas
include/              Cabeceras compartidas
resourcepacks/        Texturas
documentación/        Notas técnicas
```

---

## Sistemas

### Generación de terreno (`src/terrain/`)

Arquitectura multicapa, no un único Perlin:

| Módulo | Responsabilidad |
|---|---|
| `NoiseSystem.h` | Perlin, Simplex, Cellular, FBM, Ridged, Billow, domain warping |
| `ClimateGenerator.h` | 6 mapas climáticos independientes |
| `BiomeGenerator.h` | Selección de bioma y mezcla en fronteras |
| `TerrainGenerator.h` | Composición del relieve en 7 capas |
| `MountainGenerator.h` | Cordilleras, valles y acantilados |
| `OceanGenerator.h` | Batimetría, taludes y plataformas |
| `BeachGenerator.h` | Playas por geometría (altura + pendiente) |
| `RiverGenerator.h` | Ríos por la línea cero de un campo de ruido |
| `CaveGenerator.h` | Cuevas combinando 5 técnicas de ruido |
| `DecorationSystem.h` | Árboles (Poisson Disk), dunas y minerales |
| `ChunkGenerator.h` | Voxelización |

Determinista: la misma seed produce siempre el mismo mundo, incluso
generando chunks desde varios hilos.

### Character controller (`src/player/`)

Física con **timestep fijo a 120 Hz**, de modo que el movimiento es idéntico
a cualquier framerate. Módulos separados para colisiones, gravedad, salto,
movimiento, cámara e input.

Incluye: caminar, correr (doble toque de W), saltar, agacharse, nadar,
escaleras, subida automática de escalones y deslizamiento contra paredes.

### Audio (`src/audio/`)

Mezclador propio sobre `waveOut` con hasta 16 voces simultáneas. Los sonidos
de pisada se **sintetizan en código** (no hay archivos de audio), con
espectro y envolvente distintos por material.

### UI (`src/ui/`)

`ItemTextureManager` resuelve `ItemID → TextureID` con caché positiva y
negativa, garantizando que nunca se hace I/O de disco durante el render y
que siempre hay una textura válida. Los iconos de bloque se dibujan como
cubos isométricos 3D.

---

## Controles

| Tecla | Acción |
|---|---|
| WASD | Moverse |
| W W (doble toque) | Correr |
| Espacio | Saltar / nadar hacia arriba |
| Shift | Agacharse |
| Ctrl | Correr |
| V | Volar (solo en creativo) |
| E | Inventario |
| 1-9 | Seleccionar slot |
| Q | Tirar item |
| F3 | Depuración del controlador |
| F6 | Depuración de la hotbar |
| F11 | Pantalla completa |
| ESC | Pausa |

---

## Estado

Alpha. El juego compila, arranca y es jugable.

**Sistemas activos:** terreno, chunks con streaming, física, colisiones,
inventario y crafteo, guardado, audio, niebla volumétrica, fluidos (agua y
lava), vegetación.

**Sistemas presentes pero desactivados:**
- Iluminación por voxel: implementada (~550 líneas) pero desactivada en
  runtime. Mientras tanto el color de luz es blanco fijo.
- Greedy meshing: implementado pero desactivado.

**Limitaciones conocidas:**
- Altura del mundo limitada a 128 bloques.
- OpenGL 2.1 fixed-function, sin shaders.
- `src/TerrainGeneration/` es código muerto de una versión anterior del
  generador; el activo es `src/terrain/`.

---

## Créditos

Trabajo de reestructuración y correcciones de rendimiento integradas desde
[Voxel-Genesis](https://github.com/Scram-Consulting/Voxel-Genesis).
