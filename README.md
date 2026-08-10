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

GLFW y doctest vienen incluidos como código fuente en `external/`, así que no
hay que instalar dependencias externas ni tener red.

### Tests

```batch
ctest --test-dir build -C Release --output-on-failure
```

Cubren la lógica que protege los datos del jugador —compresión de chunks,
CRC32, validación al deserializar, paletas, nombres de mundo y determinismo del
generador por semilla— sin necesidad de arrancar OpenGL.

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
  BlockType.h         Enum de bloques (compartido con tests y paletas)
  WorldName.h         Validación de nombres de mundo
include/              Cabeceras compartidas
tests/                Tests unitarios (doctest)
external/             GLFW, stb, doctest (vendorizados)
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

Determinista: la misma seed produce siempre el mismo mundo. El generador está
escrito para ser seguro entre hilos, aunque hoy la generación corre en el hilo
principal (ver *Estado*).

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
| F7 | Overlay de rendimiento (FPS, ms por fase, chunks) |
| F11 | Pantalla completa |
| ESC | Pausa |

---

## Estado

Alpha. El juego compila, arranca y es jugable.

**Sistemas activos:** terreno, chunks con streaming, física, colisiones,
inventario y crafteo, guardado, audio, niebla volumétrica, fluidos (agua y
lava), vegetación.

**Diagnóstico.** El juego escribe un log completo en
`%LOCALAPPDATA%\VoxelWorld\log.txt`, y los errores de los que no se puede
continuar se muestran en un cuadro de diálogo en vez de cerrar la ventana en
silencio. Si una sesión termina en crash, la siguiente lo avisa al arrancar.

**Generación asíncrona.** Los chunks nuevos se generan en 2 hilos de trabajo
(`World::GenContext`): mientras un chunk se genera, sus lecturas y escrituras
se resuelven contra él mismo y no contra el mapa global, así que la generación
no toca estructuras compartidas y el hilo de render no se detiene al explorar.
Los chunks ya guardados se cargan de disco en el hilo principal (es rápido).

**Iluminación.** Skylight real por chunk con sombras suaves: pasada vertical
más flood-fill estilo Minecraft (la luz pierde 1 nivel por bloque al doblar
esquinas, así los bordes de las sombras se difuminan en gradiente). Cada cara
muestrea la luz del bloque de aire que la toca — tapar un bloque oscurece su
cara superior, no sus laterales. La luz se calcula en los hilos de generación
y se recalcula al modificar bloques. Torchlight y luz con color quedan como
evolución futura.

**Greedy meshing.** Las caras coplanares contiguas con la misma textura y la
misma luz se fusionan en un solo quad, respetando el gradiente de sombras (los
quads se parten donde cambia el nivel de luz). Agua, lava y vegetación
conservan su render propio.

**Autoguardado.** Cada 2 minutos, solo los chunks modificados, encolados a dos
hilos de guardado sin bloquear el frame. El guardado al salir sigue siendo
completo y bloqueante.

**Memoria.** Los bloques viven únicamente en los subchunks con paleta; el
volcado crudo solo existe de forma transitoria al guardar/cargar (el formato
en disco no cambió).

**Limitaciones conocidas:**
- Altura del mundo limitada a 128 bloques.
- OpenGL 2.1 fixed-function, sin shaders.
- La propagación de luz no cruza fronteras de chunk (un gradiente que caiga
  justo en el borde se corta ahí).

**Formato de guardado.** Versión 2. Los mundos creados con la versión 1 se
siguen leyendo (hay un decodificador legacy y un test que lo cubre); los
guardados nuevos ya usan el RLE con escape y CRC32 real.

---

## Créditos

Trabajo de reestructuración y correcciones de rendimiento integradas desde
[Voxel-Genesis](https://github.com/Scram-Consulting/Voxel-Genesis).
