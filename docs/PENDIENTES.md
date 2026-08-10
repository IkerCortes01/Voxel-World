# Pendientes técnicos

> Documento vivo. Origen: auditoría de ingeniería del 2026-08-08
> (`AUDITORIA-INGENIERIA-2026-08-08.md`) más lo descubierto durante la
> ejecución de su plan de acción, completado el 2026-08-10. Los 10 hallazgos
> críticos y el plan de acción original están **cerrados**; esto es lo que
> queda, verificado contra el código actual (no contra recuerdos de la
> auditoría).

Cada punto lleva: dónde está, por qué importa y esfuerzo estimado.

---

## 1. Arquitectura (el frente grande)

### 1.1 Desmonte del monolito — EN CURSO, largo plazo
`src/main.cpp` tiene **14.853 líneas**. Extraídos hasta ahora: `BlockType.h`,
`WorldName.h`, `Inventory.h` (con tests). Orden sugerido para seguir, de menor
a mayor acoplamiento:

1. `CraftingSystem` (línea ~3323) — como Inventory, es lógica pura testeable.
2. `SoundManager` (~530) y `TextureManager` (~2120) — autocontenidos, pero
   tocan waveOut/OpenGL: extraer sin test unitario, protegidos por smoke test.
3. Ruido/terreno restante en main.cpp.
4. UI/menús (los `render*Screen` y sus handlers de click).
5. `World` al final, dividido en ChunkStore + Mesher + WorldRenderer +
   Persistence (hoy es una God Class de ~4.500 líneas con ≥10
   responsabilidades).

Regla: una clase por commit, compilar + tests + arrancar el juego en cada paso.

### 1.2 Estado global
`g_gameState` se usa desde funciones libres por todo el árbol; `GameState`
mezcla ~90 campos de modelo y de UI. Va cayendo solo conforme avance 1.1
(inyectar por parámetro al extraer). No intentar un big-bang.

### 1.3 Tabla de datos de bloques
Añadir un bloque hoy toca **7 switches** (`case BLOCK_STONE` aparece en 7
sitios: textura, drops, dureza, transparencia…). Una tabla `BlockInfo[]`
indexada por el enum convierte "añadir un bloque" en añadir una fila.
Esfuerzo: medio. Valor: alto si se van a añadir bloques.

---

## 2. Código muerto nuevo (dejado por la evolución reciente)

La reactivación de la iluminación por skylight per-chunk dejó **obsoleto el
sistema de iluminación global** que nunca llegó a usarse:

- `calculateWorldLightingThreaded()`, `calculateSkylight()`,
  `propagateSunlight()`, `propagateTorchlight()`, el BFS de `LightNode` y
  `processLightingQueue` (~400+ líneas en main.cpp).
- `lightingThread` / `lightingQueue` / `lightingMutex` (12 referencias): el
  hilo se declara y se joinea pero ya nada lo lanza.

Además, muerto de antes y aún presente:

- `ObjectPool.h` — incluido en main.cpp:97 pero `ObjectPool<` tiene **cero**
  instanciaciones.
- `AdaptiveQuality.h` — archivo huérfano, sin include.
- `include/ChunkSystem.h` + `src/ChunkSystem.cpp` — se compila y enlaza; el
  uso real es mínimo (la auditoría midió ~980 líneas enlazadas sin usar).
- `include/ImprovedSaveSystem.h`, `include/PerformanceGuarantee.h` — revisar
  si algo los referencia aún.

Esfuerzo: bajo (borrar con verificación de referencias). Valor: menos ruido
para el desmonte del monolito.

---

## 3. Robustez y seguridad (restos menores)

### 3.1 `system("rmdir /S /Q ...")` en deleteWorld — main.cpp:~12802
Borra la carpeta del mundo ejecutando un comando de shell construido por
concatenación. El nombre ya pasa por la validación de `WorldName` (sin
traversal), pero sigue siendo un `system()` evitable: sustituir por
`std::filesystem::remove_all` (que además ya se usa como fallback).
Esfuerzo: bajo.

### 3.2 `saves/` relativo al directorio de trabajo — 6 usos
Los recursos (texturas/sonidos) ya se resuelven desde el exe
(`getGameRootPath()`), pero `saves/` sigue siendo relativo al CWD: lanzar el
juego desde otra carpeta crea una segunda copia de mundos. Unificar con
`gamePath("saves/")`. **Ojo**: decidir qué pasa con los mundos ya existentes
en la copia "equivocada" (¿migración al primer arranque?). Esfuerzo: bajo-medio.

### 3.3 Funciones C inseguras
16 `sprintf`, 1 `strcpy`, 1 `strcat`, y `_CRT_SECURE_NO_WARNINGS` en
CMakeLists silenciando justo esos avisos. Ninguno recibe datos de archivo hoy
(buffers locales de formato), pero migrar a `snprintf`/`std::format` y quitar
la macro cerraría la puerta. Esfuerzo: bajo, mecánico.

### 3.4 El `.vxr` no reutiliza sectores
`RegionFile` siempre apila los chunks re-guardados al final: el archivo crece
sin límite con el uso. No corrompe nada; desperdicia disco. Arreglo real:
mapa de sectores libres o compactación periódica. Esfuerzo: medio-alto.
Mitigación barata: compactar al cerrar el mundo si el desperdicio supera un
umbral.

---

## 4. Calidad de build y proceso

### 4.1 Sin CI — no existe `.github/workflows/`
La verificación es manual. Un workflow mínimo de GitHub Actions (runner
Windows: configurar CMake, compilar Release, correr ctest) protege `main` en
cada push. Esfuerzo: bajo. Valor: alto.

### 4.2 `/W4` solo en los tests
El juego compila con el nivel de warnings por defecto. Subirlo a `/W4` +
`/permissive-` sacará una tanda de avisos que limpiar una vez y mantendrá el
resto a raya. Esfuerzo: medio la primera vez.

### 4.3 ASAN en Debug
`/fsanitize=address` en la config Debug para cazar corrupciones de memoria en
desarrollo. Esfuerzo: bajo (una línea de CMake), coste solo en Debug.

---

## 5. Motor y render (evolución, no deuda)

- **Torchlight y luz con color** — `LightVoxel` ya reserva los bits
  (torchlight 5b + RGB 2b×3) y `getBlockEmission()` existe con la tabla
  esbozada. Encaja como flood-fill análogo al del skylight.
- **Propagación de luz entre chunks** — hoy el gradiente se corta en la
  frontera del chunk (concesión deliberada para que los workers no toquen
  estado compartido). Solución: pasada de "costura" en el hilo principal al
  integrar un chunk, re-derramando desde los bordes de los vecinos.
- **Smooth lighting + oclusión ambiental** — interpolar la luz en los 4
  vértices de cada cara (media de las 4 celdas adyacentes) en vez de luz
  plana por cara. Es el salto visual grande que queda. Nota: reduce la
  fusión del greedy (la clave de fusión pasa a ser por vértice).
- **HUD/menús en modo inmediato** — el mundo va por VBOs; la UI sigue en
  glBegin/glEnd. Migrar cuando moleste en el profiler, no antes.
- **Shaders** — todo el render es fixed-function OpenGL 2.1. Una migración a
  shaders básicos habilitaría niebla/luz más ricas, pero es un proyecto en
  sí mismo.
- **Altura del mundo 128** — subirla toca generación, paletas, save y luz;
  hacerlo antes de que existan muchos mundos guardados o con migración.

---

## 6. Experiencia y contenido (fuera del ámbito de la auditoría)

- Font bitmap propio sin acentos ("Coloca items aqui") y UI mezclando
  español e inglés en la misma pantalla. Un atlas de fuente eliminaría
  ~1.000 líneas de `renderChar` y habilitaría tildes.
- Strings de UI hardcodeados: extraer a una tabla indexada por enum aunque
  solo haya un idioma.

---

## Registro de decisiones asumidas (para no re-litigarlas)

1. **Lecturas fuera del chunk durante la generación devuelven aire.** Era la
   fuente del indeterminismo terreno-según-orden-de-exploración. Afecta ~1 de
   13 chunks en columnas de borde con decoración.
2. **La luz no cruza chunks** (ver 5). Misma familia de concesión que la 1.
3. **`add()` del inventario recorta el exceso** en vez de devolver lo
   añadido; los stacks infinitos usan centinela en `count` para no romper el
   formato de guardado. Fijado por tests.
4. **El formato de guardado de chunks no cambió** al eliminar el array
   espejo: `exportBlocks/importBlocks` reproducen el layout `[x][y][z]` byte
   a byte.
