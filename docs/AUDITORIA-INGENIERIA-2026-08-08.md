# Auditoría de Ingeniería de Software

> **Nota de contexto (2026-08-09).** Esta auditoría se hizo sobre
> [Voxel-Genesis](https://github.com/Scram-Consulting/Voxel-Genesis), una línea
> de trabajo distinta de este repositorio. Se conserva aquí porque el
> diagnóstico y el plan de acción aplican al mismo motor, y porque las
> correcciones que salieron de ella están ahora integradas en Voxel World.
>
> Los dos árboles no compartían historia: Voxel World iba por delante en
> contenido de juego (generador modular en `src/terrain/`, character controller
> en `src/player/`, audio y UI propios) y por detrás en integridad de datos,
> observabilidad y tests. La integración consistió en traer lo segundo sin
> perder lo primero.
>
> ### Estado de los hallazgos críticos en Voxel World
>
> | # | Hallazgo | Estado |
> |---|---|---|
> | C1 | Pérdida de datos al reciclar chunks del pool | ✅ Ya estaba corregido en este árbol |
> | C2 | RLE no round-trip safe (el byte `0xFF` corrompía chunks) | ✅ Corregido (formato v2, lectura v1 compatible) |
> | C3 | Deserialización sin validación de límites | ✅ Corregido |
> | C4 | CRC32 decorativo (tabla vacía) y sin escritura atómica | ✅ Corregido (tabla estándar + temp/rename) |
> | C5 | Signal handler que podía corromper un save bueno | ✅ Corregido (marcador async-signal-safe) |
> | C6 | Sin control de versiones | ✅ El proyecto está en git |
> | C7 | Build no reproducible | ✅ `CMakeLists.txt` en la raíz, GLFW vendorizado |
> | C8 | Todos los fallos invisibles | ✅ Corregido (log a archivo + diálogos) |
> | C9 | Cero threading en la generación | ✅ Corregido (port del `GenContext` de Voxel-Genesis, 2 workers) |
> | C10 | Cero tests | ✅ Suite doctest: 37 casos, 2830 aserciones |
>
> Además se corrigieron, fuera de la lista original:
> - `std::isalnum(char)` con bytes >127 (comportamiento indefinido) y los
>   nombres de dispositivo reservados de Windows, ahora en `src/WorldName.h`.
> - Excepciones de parsing sin capturar: un `level.dat` corrupto tiraba la
>   aplicación **al abrir el menú de selección de mundos**, fuera del try/catch
>   del bucle de juego.
> - El `Profiler` se compilaba con cero puntos de uso; ahora mide las cinco
>   funciones que dominan el frame y tiene overlay en F7.
> - `buildChunkMesh` recorría los 65.536 bloques del chunk en cada
>   reconstrucción solo para saber si estaba vacío; la paleta ya lo sabía.
> - Las métricas `Phys/Chunks/Render` del título de la ventana eran siempre
>   `0.0ms`: se imprimían en un punto del bucle anterior a donde se calculaban.
> - La dependencia de orden invisible de `PalettedStorage.h` (Hallazgo
>   Importante #12), resuelta extrayendo `src/BlockType.h`.
>
> Sobre C9: se portó la solución de la fase 5h de Voxel-Genesis
> (`World::GenContext`, contexto de generación por hilo) y se **eliminó** el
> andamiaje inerte que había en este árbol (`workerThreads`, `generationQueue`,
> `chunkGenerationWorker()`) — nadie encolaba nada en él y su worker guardaba
> un puntero colgante a un elemento de vector. La generación corre ahora en 2
> hilos de trabajo; el hilo principal solo integra resultados y construye VBOs.
>
> ### Lo que queda pendiente
>
> Por orden de retorno esperado:
>
> 1. **Guardado asíncrono.** El autosave escribe todos los chunks, no solo los
>    modificados, y de forma síncrona en el hilo de render: con la generación
>    ya en hilos, es el mayor tirón de frame que queda (microparón periódico
>    cada 120 s).
> 2. **Reactivar la iluminación**, desactivada en un intento previo de subir
>    FPS (`getLightLevel()` devuelve constante). Ahora el hilo principal tiene
>    margen de sobra.
> 3. **Integrar greedy meshing** (elegir una de las implementaciones existentes
>    y borrar las demás).
> 4. **Eliminar el almacenamiento triplicado por chunk** (paleta + array crudo
>    de 256 KB + `lightData` de 128 KB inútil mientras la luz sea constante).
> 5. **Desmonte del monolito.** `main.cpp` sigue por encima de las 14.000
>    líneas; de aquí solo se han extraído `BlockType.h` y `WorldName.h`.
> 6. **Test del inventario.** El de la auditoría original no aplica: este motor
>    tiene stacks infinitos en creativo, `add()` que devuelve `void` y
>    `MAX_STACK_SIZE` de 187. Hay que escribir el equivalente para esta
>    semántica.
> 7. **Limpieza menor:** borrar `src/TerrainGeneration/` (código muerto de un
>    generador anterior) y los warnings `/W4` que el build de tests saca de
>    `SaveSystem.cpp` (C4100) y `PalettedStorage.h` (C4267).
>
> ---
>
> Lo que sigue son los hallazgos y puntuaciones **en el momento de la
> auditoría** sobre el árbol de Voxel-Genesis. Se conservan como línea base
> para medir el progreso, no como descripción del estado actual.

**Fecha:** 2026-08-08
**Alcance:** proyecto completo en `C:\VoxelGenesis` (motor de vóxeles tipo Minecraft, C++20, OpenGL fixed-function + VBO, GLFW 3.4, solo Windows, ~26.000 líneas).
**Método:** auditoría de solo lectura con 4 análisis paralelos (arquitectura/SOLID, rendimiento/escalabilidad, seguridad/errores, testing/observabilidad/CI/docs), checklist de la skill `software-engineering` adaptado al dominio (juego de escritorio offline sin red: auth/HTTPS/CORS/rate-limiting/tracing distribuido = **N/A**, re-ponderados; la superficie de seguridad relevante es deserialización de saves, rutas de archivos y robustez ante datos corruptos).

---

## Resumen Ejecutivo

### Score general: 23/100

| Área | Score | Estado |
|---|---:|---|
| Arquitectura | 2/10 | Crítico |
| Rendimiento | 5/10 | Deficiente |
| Seguridad (adaptada) | 4/10 | Deficiente |
| Manejo de Errores | 4/10 | Deficiente |
| Testing | 0/10 | Inexistente |
| Escalabilidad | 4/10 | Deficiente |
| Observabilidad | 2/10 | Crítico |
| CI/CD | 0/10 | Inexistente |
| Documentación | 1/10 | Crítico (activamente dañina) |
| Accesibilidad/i18n | 1/10 | Crítico |

### Diagnóstico en una frase

El proyecto funciona (compiló y corre), pero **no es mantenible ni confiable**: un monolito de 16.310 líneas sin control de versiones, editado históricamente con 45 scripts de parcheo textual, con un sistema de guardado que puede corromper mundos silenciosamente, errores invisibles para el usuario, y ~7.000 líneas de optimizaciones ya escritas que nunca se conectaron.

### Áreas críticas

1. **Integridad de datos**: el guardado puede persistir aire en lugar de terreno (bug del pool de chunks), el compresor RLE corrompe chunks que contengan el byte `0xFF`, el CRC32 es falso, y no hay escritura atómica. **El jugador puede perder su mundo sin ningún aviso.**
2. **Proceso de desarrollo**: sin git, sin tests, sin CI, build no reproducible, documentación que contradice al código.
3. **Silencio total ante fallos**: ~397 logs van a una consola que no existe (`/SUBSYSTEM:WINDOWS`), cero `MessageBox`. Todo fallo es invisible.
4. **Arquitectura**: God Class `World` (~4.880 líneas, ≥10 responsabilidades), `main()` de ~1.960 líneas, estado global (`g_gameState`, 418 usos), cero interfaces (`virtual` = 0 ocurrencias).

### Corrección al diagnóstico popular del proyecto

Los ≥12 documentos de "arreglar FPS" persiguen al culpable equivocado. El render del mundo **ya usa VBOs** con frustum culling y caching de meshes (el modo inmediato solo vive en HUD/menús). Las causas raíz reales del rendimiento son: **(a)** cero threading — toda la infraestructura de workers existe (`workerThreads`, `chunkGenerationWorker`, `lightingThread`) pero **nunca se arranca** (`src/main.cpp:3203`, `:3386-3449`, `:14559`); **(b)** greedy meshing implementado tres veces y conectado cero veces; **(c)** almacenamiento de bloques triplicado (~380 KB/chunk); **(d)** autoguardado síncrono de ~40 MB en el hilo de render. Y nunca se midió dónde se va el tiempo: el `Profiler` completo se compila pero tiene **cero call sites**.

---

## Resultados por Área

### 1. Arquitectura: 2/10

Monolito de un solo archivo sin capas ni abstracciones. Existe una arquitectura correcta (`include/ChunkSystem.h`: state machine, `ThreadSafeQueue`, `MeshBuilder` desacoplado de OpenGL) que se compila pero de la que solo se invoca **una línea** (`src/main.cpp:14403`).

| Ítem | Veredicto | Evidencia |
|---|---|---|
| Separación en capas | NO CUMPLE | `class World` (`src/main.cpp:3140`) mezcla dominio, datos (fopen/fwrite en 7186-7278) y presentación (VBOs en 6046, render en 6942); `main()` dibuja inventario inline (15914-16240) |
| Inversión de dependencias | NO CUMPLE | 0 `virtual` en todo main.cpp; `World` hace `new NextGenTerrainGenerator` en su constructor (`:3454`) |
| Comunicación via interfaces | NO CUMPLE | Todo por globals y campos públicos: `chunk->blocks[x][y][z]` directo (`:3248`) |
| Patrones de diseño | PARCIAL | Object Pool, LRU cache y worker pool existen pero ad-hoc dentro de la God Class; los patrones bien hechos están en código muerto (`ChunkSystem.h:34,251,305`) |
| SOLID | NO CUMPLE | S: `World` con ≥10 responsabilidades y `generateChunk` de ~1.216 líneas (3665-4880). O: añadir un bloque toca ≥6 sitios con switches. L: no hay jerarquías. I: `GameState` con ~90 campos públicos (8400-8707). D: `g_gameState` (418 usos), `g_textureManager`, `g_soundManager` |
| Sin acoplamiento excesivo | NO CUMPLE | 418 referencias a `g_gameState` desde funciones libres; nada es testeable aislado |

### 2. Rendimiento: 5/10

Lo bueno: VBOs para el mundo con batching por textura (`src/main.cpp:6497-6531`, `:7089-7099`), frustum culling real (`:322-396`, aplicado en `:6986`), caching de meshes con dirty flag (`:6049`), carga de chunks priorizada por distancia y dirección (`:6636-6682`), time budget de 8 ms para meshing (`:6785`).

Lo malo: **todo el trabajo pesado corre en el hilo principal** (generación, meshing, iluminación); la iluminación se **apagó por completo** para llegar a 60 FPS (`getLightLevel()` devuelve constante 18, `:2509`); una cara = un quad (sin greedy meshing); escaneo de 65.536 bloques por rebuild solo para detectar chunks vacíos (`:6060-6068`); `reserve()` de vértices que es un no-op (itera mapas vacíos, `:6135-6143`); allocaciones de vectores/mapas por frame en el hot path; fixed-function puro sin un solo shader (`shaders/` está vacía).

### 3. Seguridad: 4/10 (adaptada a juego offline)

| Ítem | Veredicto | Evidencia |
|---|---|---|
| Validación de datos deserializados | NO CUMPLE | `compressedSize` del archivo se usa sin comparar con el buffer real → over-read de heap (`src/SaveSystem.cpp:410-422`, `:447-450`); `version` se lee y nunca se compara; `blockType` casteado sin rango (`src/main.cpp:14208-14213`) |
| Escritura atómica de saves | NO CUMPLE | Sin temp+rename en ninguna ruta; `save.lock` se borra pero nunca se crea (`src/main.cpp:14039-14042`) |
| Sanitización de rutas/nombres | PARCIAL | El filtro `[alnum]_- espacio` bloquea traversal (`:12421-12427`), pero `isalnum(char)` es UB con bytes >127, no rechaza nombres reservados de Windows, y hay `system("rmdir /S /Q ...")` por concatenación (`:12242-12245`) |
| Sin buffer overflows evidentes | PARCIAL | Buffers fijos no reciben datos de archivo, pero el over-read de CRC es real |
| Sin funciones inseguras | NO CUMPLE | 12 `sprintf`, `strcpy/strcat` (`:13281-13283`), `system()` (`:12245`); `_CRT_SECURE_NO_WARNINGS` silencia justo esos avisos |
| Datos de usuario en ubicación única | NO CUMPLE | `"saves"` relativo al CWD en 5 sitios → dos copias divergentes (raíz y `build/bin/Release/`) |
| auth/HTTPS/CORS/rate-limit | N/A | Juego offline sin sockets — correctamente fuera de alcance |

### 4. Manejo de Errores: 4/10

Hay 43 bloques try/catch y fallbacks buenos (textura ausente → id 0, sonido → `Beep`, reintentos con backoff en `deleteWorld`). Pero: **cero estructura de error** (todo es `bool`/`void` + `cerr`), 4 `catch(...)` literalmente vacíos (`src/main.cpp:12063, 12066, 12149, 12229`), parsers `std::stoi/stoul/stof` sin capturar — **un `level.dat` malformado tira la aplicación al abrir el selector de mundos** (`:13914-13930`, `:11009-11032`) —, y "mundo corrupto" es indistinguible de "mundo nuevo" para el llamador (`:14178` vs `:14288`). Todo mensaje de error va a una consola inexistente; 0 `MessageBox` en el proyecto.

### 5. Testing: 0/10

Cero tests de cualquier tipo. Sin framework, sin `enable_testing()`, sin un solo archivo de test. La verificación histórica fue 100 % manual, y el mecanismo de cambio fueron 45 scripts Python de búsqueda-y-reemplazo textual sobre `main.cpp` — re-ejecutar cualquiera sobre el archivo ya parcheado lo corrompe.

### 6. Escalabilidad: 4/10

| Ítem | Veredicto | Evidencia |
|---|---|---|
| Graceful shutdown | PARCIAL | Camino normal correcto (`:16275-16305`); pero el signal handler de emergencia hace I/O, filesystem y toma mutexes desde SIGSEGV — no async-signal-safe (`:14295-14343`) |
| Guardado no bloqueante | NO CUMPLE | Autosave cada 120 s guarda TODOS los chunks (no solo modificados), 256 KB crudos c/u, síncrono en el hilo de render (`:14820-14823` → `:7346-7357`) |
| Idempotencia de guardado | PARCIAL | Sobrescritura completa converge, pero sin atomicidad ni checksum real; además el `.vxr` nunca reutiliza sectores → crece sin límite (`src/SaveSystem.cpp:274-307`) |
| Límites de memoria/chunks | PARCIAL | Hay límites (RENDER_DISTANCE=6, MAX_CACHED_CHUNKS=512, pool=50) pero ~400 KB/chunk por triplicación → ~200 MB solo de caché |
| Métricas/health | PARCIAL | FPS en el título de ventana con el literal fijo engañoso `[60FPS]` (`:14625`); sin HUD; Profiler muerto |

### 7. Observabilidad: 2/10

~397 sentencias de log (263 `cout`, 109 `cerr`) sin niveles, timestamps ni archivo — y **el 100 % se descarta**: binario `WIN32` + `/SUBSYSTEM:WINDOWS` sin `AllocConsole`/`freopen`. El `Profiler` (307+161 líneas: FPS, CPU/GPU ms, draw calls, memoria, overlay con gráficas, macros `PROFILE_SCOPE`) se compila y enlaza pero tiene **cero usos** — el toggle F3 que la documentación promete no existe (`grep F3` = 0). Data race adicional: `getStatistics()` lee estructuras compartidas sin mutex (`src/SaveSystem.cpp:852-861`).

### 8. CI/CD: 0/10

No es un repositorio git. Sin pipeline, sin gates, sin releases versionadas (el "v1.0.0 - Alpha" de la UI está hardcodeado, `:11793`). El build ni siquiera es reproducible: no existe `CMakeLists.txt` en la raíz (el único está en `documentación/`), y `build/CMakeCache.txt` apunta a `D:/Respaldo/Voxel World` (otra máquina) — `cmake -B build` falla hoy de inmediato.

### 9. Documentación: 1/10

75 archivos en `documentación/`, contradictorios entre sí y contra el código: `OPTIMIZACION_120_FPS.md` afirma VSync desactivado cuando el código tiene `glfwSwapInterval(1)` (`:14370`); `FINAL_SUMMARY.md` afirma que F3 abre el profiler (falso); `60FPS_READY.md` declara integradas 5 cabeceras con 0 referencias; el `README.md` describe un proyecto de "8 bloques" con mundos en `projects/` (hay >30 bloques y los mundos están en `saves/`). La carpeta además contiene artefactos que no son docs: el `CMakeLists.txt` real, `run.bat`, `create_portable.bat`, un `.cpp` suelto y el `.gitignore`. **Seguir esta documentación rompe el proyecto.**

### 10. Accesibilidad/i18n: 1/10

Cero infraestructura: ~80 literales de UI hardcodeados dentro de `renderText(...)`, mezclando español ("CREAR NUEVO MUNDO", `:12992`) e inglés ("SURVIVAL"/"CREATIVE", `:13120`/`:13150`) en la misma pantalla; el font bitmap propio no soporta acentos ("Coloca items aqui", `:16165`); sin escalado de texto ni opciones de accesibilidad.

---

## Hallazgos Críticos (arreglar inmediatamente)

**C1. Pérdida de datos al reciclar chunks del pool** — `src/main.cpp:3245-3251`
`allocateChunk()` limpia `blocks[][][]` pero no reinicializa `subchunks` (la paleta que `getBlock()` realmente lee). Con el early-return de `setBlock()` (`:2491`), un chunk reciclado puede guardarse como **aire** en lugar del terreno real. Corrupción silenciosa de mundos.

**C2. El compresor RLE no es round-trip safe** — `src/SaveSystem.cpp:36-69`
Un byte literal `0xFF` en los datos se emite crudo y el descompresor lo reinterpreta como marcador RLE, desplazando el resto del chunk. Cualquier `BlockType` = 255 destruye el chunk al recargar. Además `decompress` no acota la salida (expansión 255× posible).

**C3. Deserialización sin validación de límites** — `src/SaveSystem.cpp:410-422`, `:315-321`, `:447-450`
`compressedSize` y `chunkSizes[index]` vienen del archivo y se usan sin comparar con el tamaño real: over-read de heap de hasta 4 GiB y allocación no acotada (`MAX_CHUNK_SIZE` declarado y jamás usado). El `bad_alloc` resultante no se captura en el hilo de guardado → `std::terminate`.

**C4. La verificación de integridad es decorativa** — `src/SaveSystem.cpp:100-110`, `src/main.cpp:14001/:14219`
El "CRC32" tiene la tabla vacía y un algoritmo inventado; el "checksum" de `player.dat` es la constante `0xDEADBEEF` y su fallo solo emite un warning invisible. No existe detección real de saves corruptos, y tampoco escritura atómica (sin temp+rename; `save.lock` se borra pero nunca se crea, `:14039-14042`).

**C5. Signal handler de emergencia que puede corromper un save bueno** — `src/main.cpp:14295-14343`
`emergencySaveHandler` (SIGSEGV/SIGABRT/SIGILL/SIGFPE) llama a `saveWorld()` completo: `filesystem`, `ofstream`, `cerr`, mutexes — nada async-signal-safe, desde un proceso ya corrupto. Peor caso: sobrescribe un save válido con estado basura.

**C6. Sin control de versiones + 45 scripts de parcheo textual** — raíz del proyecto
No hay `.git/`. El historial de cambios son scripts Python contradictorios (`fix_raycast.py` × 5 variantes, `implement_minecraft_lighting.py` vs `disable_lighting_completely.py`) que corrompen el código si se re-ejecutan. Todo cambio es irreversible y no auditable.

**C7. Build no reproducible** — `documentación/CMakeLists.txt`, `build/CMakeCache.txt:381`
El único `CMakeLists.txt` está fuera de la raíz con rutas que asumen la raíz; la caché apunta a `D:/Respaldo/Voxel World`. `cmake -B build` falla hoy. Binarios, PDB e ILK versionables dentro del árbol.

**C8. Todos los fallos son invisibles** — `documentación/CMakeLists.txt:38,50` + 0 `MessageBox`
~397 logs van a una consola que no existe; los banners de error y los `cin.get()` de "Presiona Enter" retornan por EOF. Un mundo corrupto simplemente "no carga" sin explicación. Además las rutas de texturas y sonidos apuntan a `D:/Respaldo/Voxel World/...` (`src/main.cpp:416, :1928, :2068, :2295-2307`) — **en esta máquina el juego arranca sin texturas ni sonidos y no lo dice**.

**C9. Cero threading: la causa raíz del problema de FPS** — `src/main.cpp:3203, :3386-3449, :14559`
Generación de terreno, meshing e iluminación corren en el hilo principal; los workers existen y nunca se lanzan; la iluminación se amputó entera como workaround. Doce documentos de optimización sin haber medido nada (Profiler con 0 call sites).

**C10. Cero tests** — todo el árbol
16.310 líneas sin una sola verificación automatizada. Combinado con C6, cualquier cambio es ruleta rusa.

---

## Hallazgos Importantes (arreglar pronto)

1. **God Class `World`** (~4.880 líneas, ≥10 responsabilidades) y `generateChunk` de ~1.216 líneas (`src/main.cpp:3665-4880`); `main()` de ~1.960 líneas; `GameState` de ~90 campos públicos.
2. **Almacenamiento triplicado por chunk** (`:2407-2413`): paleta + array crudo de 256 KB + `lightData` de 128 KB que es inútil (la luz es constante). Eliminar los dos últimos ahorra ~95 % de memoria por chunk.
3. **Autoguardado síncrono de ~40 MB en el hilo de render** cada 120 s, guardando todos los chunks aunque no estén modificados (`:14820` → `:7346-7357`).
4. **Greedy meshing escrito 3 veces, conectado 0** (`src/GreedyMesher.h`, `buildChunkMeshGreedy` en `:5810-6045`, `RenderOptimizations.cpp` excluido del build). ~97 % menos vértices esperables — la mejora de mayor ROI pendiente.
5. **~7.000 líneas de código muerto**: todo `src/TerrainGeneration/` (8 headers), `GreedyMesher.h`, `AdaptiveQuality.h`, `PerformanceGuarantee.h`, `ImprovedSaveSystem.h`, `RenderOptimizations.cpp/h`, `buildChunkMeshGreedy`, `ObjectPool.h` (plantilla jamás instanciada), ~980 líneas de `ChunkSystem.cpp` enlazadas y sin usar, `main.cpp.backup`.
6. **Backup que se copia a sí mismo recursivamente** (crecimiento exponencial) y **restore que borra el mundo antes de copiar** (`src/SaveSystem.cpp:506-511`, `:571-581`).
7. **Excepciones de parsing sin capturar**: un `level.dat` malformado = crash en el menú (`src/main.cpp:13914-13930`, `:11009-11032`); `player.dat` se acepta sin validar streams ni rangos (`:14191-14218`).
8. **Data races en `SaveSystem`**: `getStatistics()` y `hasChunk()` leen sin mutex lo que 2 hilos mutan (`src/SaveSystem.cpp:852-861`, `:328-331`); el `.vxr` nunca reutiliza sectores → crece indefinidamente.
9. **Profiler completo compilado con cero instrumentación** — cablearlo (beginFrame/endFrame + F3) es ~1 hora de trabajo y desbloquea toda optimización informada.
10. **Documentación activamente dañina** — archivar los 10 docs de FPS y 12 "resúmenes finales" en `docs/archive/`; reescribir el README (describe otro proyecto).
11. **Título de ventana engañoso**: el literal fijo `[60FPS]` se muestra aunque corra a 15 FPS (`:14625`), y las métricas Phys/Chunks/Render se imprimen antes de calcularse en el frame.
12. **`#include "PalettedStorage.h"` en la línea 2401** de main.cpp con dependencia de orden invisible (necesita macros definidas antes).

---

## Recomendaciones (mejora continua)

- **Compilador como red de seguridad**: `/W4 /permissive-`, quitar `_CRT_SECURE_NO_WARNINGS`, migrar `sprintf/strcpy` → `snprintf/std::format`, ASAN en Debug (`/fsanitize=address`).
- **Datos de usuario en ubicación única**: resolver `saves/` y recursos relativos al ejecutable (o `%APPDATA%\VoxelGenesis`), eliminando las dos copias divergentes y las rutas `D:/...`.
- **Tabla de datos para bloques**: reemplazar los ≥6 switches por bloque con una tabla `BlockInfo[]` (textura, drops, dureza, transparencia) — convierte "añadir un bloque" en añadir una fila.
- **Extraer strings de UI** a una tabla indexada por enum (aunque solo haya un idioma) y unificar idioma en pantalla.
- **Sustituir el font bitmap** (renderChar de 547 líneas) por un atlas de fuente — elimina ~1.000 líneas y habilita acentos.
- **Ordenar el árbol**: `docs/` (con `archive/`), borrar `graphify-out/` (10 MB regenerable), `main.cpp.backup`, y los binarios de `build/`.
- Considerar a futuro migrar el HUD/menús de modo inmediato a VBOs y añadir shaders básicos — pero solo después de threading + greedy meshing, que son el 90 % de la ganancia.

---

## Plan de Acción Sugerido

Ordenado por dependencias: nada es seguro de tocar sin las fases 0-1.

### Fase 0 — Red de seguridad (medio día) 🔴
1. `git init` + mover `documentación/.gitignore` a la raíz (ampliado: `build/`, `graphify-out/`, `*.pdb`, `*.ilk`, `saves/`) + commit inicial.
2. Mover `documentación/CMakeLists.txt` → raíz; borrar `build/`; verificar `cmake -B build && cmake --build build --config Release`.
3. Congelar los 45 scripts Python (moverlos a `tools/legacy-patches/` o borrarlos — ya están en git).

### Fase 1 — Visibilidad (1 día) 🔴
4. Redirigir `cout/cerr` a `%LOCALAPPDATA%\VoxelGenesis\log.txt` (`freopen` al arrancar) y `MessageBox` en los ~5 errores fatales (GPU sin VBO, mundo no carga, save corrupto).
5. Arreglar rutas de recursos: relativas al ejecutable, no al CWD ni a `D:/Respaldo/...`. Verificar que texturas y sonidos cargan en esta máquina.
6. Cablear el Profiler existente: `beginFrame/endFrame`, `PROFILE_SCOPE` en `updateChunks`/`buildChunkMesh`/`render`, toggle F3. Quitar el literal `[60FPS]`.

### Fase 2 — Integridad de datos (2-3 días) 🔴
7. Bug del pool: reinicializar `subchunks` en `allocateChunk()` (C1).
8. RLE round-trip safe (escapar `0xFF`) + acotar salida de `decompress` (C2). Requiere versión de formato + migración de saves viejos.
9. Validar límites al deserializar (`compressedSize`, `chunkSizes`, versión, rangos de `blockType`/`count`) (C3).
10. CRC32 real (tabla estándar) y escritura atómica temp+rename en todas las rutas de guardado (C4).
11. Reemplazar el signal handler por un flag `volatile sig_atomic_t` o eliminarlo (C5). Capturar excepciones de parsing en `scanSavedWorlds`/`loadLevelDat`.
12. Arreglar backup recursivo y restore destructivo.

### Fase 3 — Tests de caracterización (2-3 días) 🟡
13. Catch2 vía CMake `FetchContent` + `enable_testing()`.
14. Tests sobre lógica pura extraíble sin refactor: round-trip de `compress/decompress`, `PalettedStorage`, `PerlinNoise` (determinismo por semilla), `Inventory`/`CraftingSystem`, sanitización de nombres de mundo. Meta inicial: los caminos que protegen datos del jugador.

### Fase 4 — Limpieza (1 día) 🟡
15. Borrar ~7.000 líneas muertas (lista en Hallazgo Importante #5) — con git y tests ya es seguro.
16. Reorganizar docs: `docs/archive/` con fecha para los 22 docs obsoletos; README nuevo y veraz en la raíz.

### Fase 5 — Rendimiento real (1-2 semanas) 🟡
17. Activar los workers existentes: generación de chunks y meshing fuera del hilo principal (la infraestructura ya está escrita; `MeshBuilder`/`GPUUploader` de `ChunkSystem.h` son el modelo correcto: construir en worker, subir a GPU en main).
18. Guardado asíncrono: cola + hilo dedicado, solo chunks `isModified`, compresión sobre la paleta.
19. Integrar greedy meshing (elegir UNA de las 3 implementaciones, borrar las otras).
20. Eliminar `blocks[][][]` y `lightData[][][]` duplicados (cerrar el TODO de `:2410`).
21. Reactivar iluminación (ahora sí viable en worker thread).

### Fase 6 — Desmonte del monolito (continuo, tras Fase 5)
22. Extracción incremental por clase desde `main.cpp` a archivos propios (una clase por PR, el test de caracterización protege cada paso): orden sugerido — `SoundManager`, `TextureManager`, ruido/terreno, `Inventory`/`Crafting`, UI/menús, `World` al final dividido en `ChunkStore` + `TerrainGen` + `Mesher` + `WorldRenderer` + `Persistence`.
23. Eliminar globals progresivamente (inyectar por parámetro); partir `GameState` en modelo vs UI.

**Regla de oro para todo el plan:** ningún cambio de las fases 2+ sin haber completado la Fase 0. El proyecto ha sobrevivido hasta ahora de milagro — sin git, un mal parche más y no hay vuelta atrás.
