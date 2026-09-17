# Auditoría de ingeniería — Voxel-World, 2026-09-17

Sucede a `AUDITORIA-INGENIERIA-2026-08-08.md`, que se hizo sobre otro
repositorio (Voxel-Genesis). Esta se hace sobre **este** código, y cada
hallazgo lleva `archivo:línea` verificada.

## Nota sobre el alcance

Voxel-World es **un juego nativo de escritorio, un jugador, sin red ni
servidor**. Buena parte del checklist estándar de ingeniería (JWT, CORS,
HTTPS, rate limiting, colas de mensajes, contenedores, OpenAPI, migraciones
de base de datos, health checks) **no aplica y no se puntúa**: no hay
superficie donde aplicarlo. Puntuar 0 en "autenticación" a un juego offline
no es una auditoría, es ruido.

Lo que sí es el equivalente real en un motor nativo, y sí se audita:

| Concepto del checklist web | Su equivalente aquí |
|---|---|
| Seguridad de API / autenticación | **Seguridad de memoria** y carreras entre hilos |
| Validación de entradas del usuario | **Deserialización del guardado** y nombres de mundo |
| Integridad transaccional en BD | **Atomicidad del guardado** y journal |
| Escalabilidad horizontal | **Streaming de chunks** y presupuesto de frame |
| Observabilidad (logs/métricas) | Sí aplica tal cual |
| CI/CD, linters, cobertura | Sí aplica tal cual |

> **El MCP `swe_audit_project` dio un resultado inservible y no se usa aquí.**
> Reportó lenguaje «C#/.NET» (es C++20), 4.761 líneas (`main.cpp` solo tiene
> 42.528), y un 91/100 con 10/10 en seguridad y arquitectura mientras su
> propio checklist tenía todas las casillas sin marcar. Además se contradice:
> marca CICD-001 «pipeline automatizado» como hecho y a la vez lo lista como
> hallazgo. No analizó este proyecto.

---

## Resumen

**Score global: 5,5/10.** Son dos proyectos en uno.

El **razonamiento** de ingeniería es de los mejores que se ven en un proyecto
personal: cada decisión difícil está escrita con su porqué y el bug que la
motivó, hay un registro de decisiones asumidas, la deserialización del
guardado es defensiva de principio a fin, y el trabajo de rendimiento se hace
**midiendo**, no adivinando.

La **higiene** de ingeniería no existe: sin CI, sin cobertura, sin linter, sin
sanitizers, el juego compila con menos avisos que sus tests, y el monolito
creció un 60 % mientras el documento lo declaraba «en desmonte».

| Área | Nota | Una línea |
|---|---:|---|
| Rendimiento | **9/10** | Medido, no supuesto. Punto más fuerte. |
| Observabilidad | **9/10** | Instrumentación de nivel profesional. |
| Documentación de decisiones | **8/10** | Registro de decisiones = ADRs de facto. |
| Manejo de errores | **7/10** | 4 vías de cierre cubiertas; sin catálogo de códigos. |
| Integridad de datos | **7/10** | Chunks excelentes; `player.dat` desprotegido. |
| Testing | **5/10** | 878 casos reales, pero 71 fuera del build y 0 tocan `main.cpp`. |
| Seguridad de memoria / hilos | **5/10** | 2 carreras confirmadas con camino concreto. |
| Arquitectura | **3/10** | `main.cpp` = 61 % del código; plan de desmonte 0/5. |
| Calidad de build | **2/10** | Juego a `/W1`, tests a `/W4`. Sin sanitizers ni linter. |
| CI/CD | **0/10** | No existe `.github/`. |

---

## Estado: los cuatro críticos quedaron arreglados el mismo día

| | Arreglo | Dónde |
|---|---|---|
| C1 | Los 7 tests huérfanos entran al build | `tests/CMakeLists.txt` |
| C2 | `recargarMallas` salta los chunks con trabajo en vuelo | `main.cpp:9870` |
| C3 | `shared_mutex` sobre el mapa `textures` | `main.cpp:4592` |
| C4 | El diálogo deja de prometer copias inexistentes | `main.cpp:37770` |
| I2 (parte) | ASAN en la configuración Debug | `CMakeLists.txt:22` |

**Al desenterrar los 71 tests, uno falló** — y no era un fallo del juego: era
el propio test, podrido *por no ejecutarse*. `test_maguey_punta.cpp:173`
afirmaba `BLOCK_TYPE_MAX == BLOCK_TAZON_OYAMEL_AGUAMIEL`, cierto el día que se
escribió, pero después se añadieron la tierra mojada, el agave azul y el
huevo de pecarí detrás del tazón. El invariante sigue vigilado donde le
corresponde (`test_agave_azul.cpp:581`, que lo ata al final del enum sin
nombrar un bloque concreto); la copia vieja se retiró.

**Resultado: 949 casos y 631.147 aserciones en verde** (antes 878 / 591.651).

Lo demás de este informe sigue abierto.

---

## Hallazgos críticos

### C1 — 7 ficheros de test, 71 casos, fuera del build sin que nadie lo sepa

`tests/CMakeLists.txt:6-64` lista los fuentes **uno a uno**. Estos existen en
disco, están versionados y **no se compilan**:

```
test_aguamiel.cpp       10 casos   test_maguey_modelo.cpp  11 casos
test_maguey_bugs.cpp    11 casos   test_maguey_punta.cpp   13 casos
test_maguey_ciclo.cpp   11 casos   test_maguey_hitbox.cpp   7 casos
test_maguey_escala.cpp   8 casos
```

949 casos en disco − 71 huérfanos = **878**, que es justo la cifra que
reporta `ctest`. Son 1.380 líneas que **parecen cobertura y no ejecutan
nada**. Nadie lo detectó porque no hay CI y porque `add_executable` con lista
explícita no avisa de ficheros sueltos.

**Arreglo:** añadirlos a `tests/CMakeLists.txt` y ver cuáles siguen pasando.
Considerar `file(GLOB)` para que no vuelva a ocurrir.

### C2 — La tecla R puede provocar un *use-after-free*

`src/main.cpp:9870-9884` (`recargarMallas`, atada a la tecla R en `33452`)
suelta el candado de **todos** los chunks:

```cpp
c->isUpdatingMesh.store(false, std::memory_order_release);
```

**sin comprobar `mallaInFlight`** — justo lo que sí comprueban las otras dos
rutas equivalentes (`9630` y `25052`). El candado es el único invariante que
impide descargar un chunk mientras un worker lo está mallando. Secuencia:

1. Un worker está dentro de `buildChunkMesh` leyendo `chunk->subchunks`.
2. El jugador pulsa R → el candado se suelta.
3. `updateChunks` ya no ve el candado → `chunks.erase` (`24170`) →
   `deallocateChunk` (`24203`) → el chunk vuelve al pool.
4. `allocateChunk` lo recicla y reasigna sus subchunks (`11221-11223`),
   liberando el buffer que el worker sigue leyendo.

**Arreglo (una línea)**, dentro del bucle:

```cpp
if (mallaInFlight.count(par.first) || genInFlight.count(par.first)) continue;
```

### C3 — Carrera sobre el mapa `textures` entre el mesher y el hilo principal

`src/main.cpp:4592` declara `std::map<std::string, GLuint> textures`.

- **Los workers leen**: `textures.find()` en `4884`, alcanzado desde el mesher
  vía `texSegura → getBlockTextureCache → getBlockTexture → getTexture`.
- **El hilo principal inserta**: `4878`, `5807`, `6826`, `6850`, `6976`.

Insertar en un `std::map` reequilibra el árbol rojo-negro; recorrerlo a la vez
es comportamiento indefinido. El camino concreto: los items **no se
precargan** (comentario en `5499-5502`), así que la primera vez que el jugador
sostiene un item nuevo, `loadTextureFromPath` (`6850`) inserta mientras los
tres workers de mallado hacen `find()`.

El código **ya admite el problema** en `6830-6834` — se cerró la puerta de
OpenGL (`puedeCargar()`) pero no la del mapa.

**Arreglo:** `std::shared_mutex` en `TextureManager`, o replicar lo que ya se
hizo bien para `capaPorHandle` (`4662-4672`): array plano indexado por handle,
elegido explícitamente *por los hilos*.

### C4 — El diálogo tras un cierre inesperado promete copias que no existen

`src/main.cpp:37750` le dice al jugador que acaba de perder progreso:

> «Hay copias de seguridad en `saves\<mundo>\backups\`.»

**`createBackup()` no se llama desde ningún sitio.** Solo está declarado
(`SaveSystem.h:270`, `:359`) e implementado (`SaveSystem.cpp:720-747`), sin un
solo llamante en `src/`. Verificado en disco: ningún mundo tiene carpeta
`backups/`.

**Arreglo:** o se llama a `createBackup()` de verdad (el código está hecho y
es bueno: excluye `backups/` al copiar, rota 3 copias), o se quita esa línea
del diálogo. Mentir al jugador sobre sus datos es lo peor de las dos.

---

## Hallazgos importantes

### I1 — `player.dat` es el fichero más doloroso de perder y el único sin defensa

- **Sin escritura atómica**: `src/main.cpp:37141` abre con `std::ofstream` en
  modo truncar → sobrescribe en sitio. Igual `world.cfg` (`37217`) y
  `level.dat` (`36943`). Un corte de luz ahí deja posición, inventario y
  durabilidad corruptos, sin fichero anterior.
- **Checksum falso**: `src/main.cpp:37202` → `uint32_t checksum = 0xDEADBEEF;`
  Es una **constante literal**, no una función de los datos. Validado contra
  sí misma en `37511`. Detecta truncamiento, no corrupción.

Contrasta con los chunks, que sí tienen CRC32 real (`SaveSystem.cpp:184-203`),
versionado v1→v2→v3 con migraciones, journal y `FlushFileBuffers`.

**Arreglo:** CRC32 real (la función ya existe) + temp/rename (`fs::rename` ya
se usa en `SaveSystem.cpp:791-830`).

### I2 — El código de producción compila con menos escrutinio que los tests

| | Juego | Tests |
|---|---|---|
| Avisos | `/W1` (por defecto, ninguna opción en `CMakeLists.txt`) | `/W4` (`tests/CMakeLists.txt:77`) |
| `/WX`, `/permissive-` | No | No |
| Sanitizers | Ninguno | Ninguno |

42.528 líneas de C++ con punteros crudos compiladas a `/W1`. Está reconocido
en `docs/PENDIENTES.md:398` desde hace tiempo. ASAN en Debug es **una línea**
de CMake (`PENDIENTES.md:403`) y lleva sin escribirse.

### I3 — 23 de 58 tests validan una reimplementación, no el motor

Como `main.cpp` no se enlaza con los tests, 23 ficheros **copian** el
algoritmo y comprueban la copia. Está declarado con honestidad en cada uno
(p. ej. `tests/test_agua_flujo.cpp:8-20`, `tests/test_luz_costura.cpp:20-22`),
pero el riesgo ya se materializó: `tests/test_derrumbe.cpp:30-32` documenta
que **el ocote se quedó fuera de la lista del test** al añadirse al motor.

Y lo de fondo: **ningún test enlaza las 42.528 líneas de `main.cpp`** —
`class World`, `TextureManager`, `CraftingSystem`, menús, raycast e
iluminación están fuera de la red. 34 de los 65 headers de `src/` no aparecen
en ningún test: render, UI (679 líneas), audio (665), y 10 de los 12 headers
de `src/player/` (1.939 líneas).

**No hay medición de cobertura** configurada en ningún sitio.

### I4 — El plan de desmonte del monolito: 0 de 5 pasos, y el monolito creció

`docs/PENDIENTES.md:22-38` declara el plan como «EN CURSO» y cifra `main.cpp`
en **26.653 líneas** (medido 2026-08-22). Hoy son **42.528**: creció un 60 %.

| Paso declarado | Estado |
|---|---|
| Extraer `CraftingSystem` | ❌ sigue en `main.cpp:8489` |
| Extraer `SoundManager` y `TextureManager` | ❌ `main.cpp:3008` y `:4590` |
| Sacar el ruido/terreno restante | ⚠️ parcial: `PerlinNoise`, `WorleyNoise`, `NextGenTerrainGenerator` siguen dentro (`:3269`, `:3370`, `:3503`) |
| Extraer UI y menús | ❌ los `render*Screen` siguen dentro |
| Partir `World` | ❌ `main.cpp:9054` |

Sí se ha extraído mucho código **nuevo** a `src/fauna/` (8.147 líneas),
`src/player/` (2.815), `src/render/` (2.283) y `src/audio/` (665) — trabajo
real y bien hecho. Pero eso es *escribir fuera*, no *desmontar lo de dentro*.

Dato que lo resume: **una sola función ocupa ~6.667 líneas** (`buildChunkMesh`,
desde `main.cpp:16541`).

### I5 — Dos mecanismos de cancelación documentados que no existen

- `Chunk::version` (`main.cpp:7346-7356`) promete que «los trabajos en vuelo
  llevan copia de la versión; al volver, si no coincide, el resultado se
  tira». **No hay ni un campo de versión en `MallaPendiente` (`9204`) ni en
  `GenResult` (`9127`), ni una sola comparación.**
- `epocaStreaming` (`9272-9282`) promete lo mismo. Se incrementa y se imprime;
  **nunca se compara**.

Hoy no rompen nada porque nadie confía en ellos, pero son una trampa: quien
lea esos comentarios asumirá que puede cancelar trabajo con seguridad.

### I6 — Banderas de `Chunk` escritas desde el worker sin atomicidad

`needsRebuild`, `waitingForNeighbors`, `buildRetries`, `esperasVecinos` y
`tieneAciculas` se escriben desde el mesher (`16655`, `16856`, `16883`,
`16887`, `16943`, `19680`, `21151`, `21877`) y se leen/escriben en el
principal (`9592`, `9643`, `9807`, `10236`, `25062`). Son `bool`/`int` planos.

La regla correcta **ya está escrita** en `main.cpp:16896-16900` para `estado` y
`tiempoEstado`; falta aplicarla a estos cinco. Síntoma práctico: actualizaciones
perdidas de `needsRebuild` → chunk invisible hasta que el vigilante lo rescata
a los 6 s, que es justo el fallo perseguido en `25023-25044`.

### I7 — `documentación/` son 90 ficheros obsoletos que estorban

Carpeta aparte con ~90 `.md`/`.txt` fechados 2026-08-09: seis documentos
distintos sobre «60 FPS», cinco sobre inversión de ratón, cada uno declarándose
«final». Son bitácoras de sesión, no documentación, y entierran lo poco que sí
está actualizado en `docs/`. Contiene además código suelto
(`modern_voxel_engine.cpp`) y su propio `CMakeLists.txt`.

---

## Lo que está bien y no hay que tocar

- **Deserialización del guardado** (`SaveSystem.cpp:313-675`): defensiva de
  principio a fin — magic, versión acotada, `compressedSize` contrastado
  contra el buffer real, `uncompressedSize` exacto, CRC32 **antes** de
  descomprimir, y tope de salida en las tres ramas del descompresor. Un
  fichero manipulado no produce lectura fuera de rango. Lo mejor del proyecto.
- **Versionado y migraciones** (`SaveSystem.h:38-44`): v1→v2→v3 con migración
  real, y se conserva `legacyCRC32v1` — una réplica del CRC defectuoso de la
  v1 — para poder seguir validando mundos viejos. Eso es compatibilidad hacia
  atrás hecha en serio.
- **Manejo de cierres inesperados**: cuatro vías cubiertas (excepción en
  `main` con guardado de emergencia `42415`, señal `37602`, SEH de Windows
  `37776`, `set_terminate` para hilos `37792`). El manejador de señal
  **deliberadamente no guarda**, y el porqué está escrito (`37590-37595`):
  guardar desde un proceso corrupto podía «sobrescribir un save bueno con
  memoria basura».
- **Confinamiento de hilos donde importa**: `chunks` es exclusivo del hilo
  principal y el mesher **no hace ni un acceso al mapa** (verificado); la foto
  del borde (`BordeVecinos`) existe justo para eso; `pendingBlocks` y el pool
  están bajo mutex; los contadores que tocan workers son `std::atomic` y los
  que no, planos — la clasificación es correcta.
- **Bounds checking**: todos los accesores de bloque y luz lo tienen. El
  empaquetado del flood-fill deriva sus bits de `CHUNK_HEIGHT` con
  `static_assert`, tras un desbordamiento silencioso ya documentado.
- **Observabilidad**: `[FPS]` con reparto por fase y peor frame, `[GPU]` con
  batches/caras/reparto de render, `[STREAM]` con censo de estados y
  percentiles, `[LENTO]`, `[CARGA-LENTA]`, `[CALIDAD]`, balizas por worker y
  vigilante de atascos. En la sesión del 16-sep esto permitió localizar tres
  cuellos reales y **descartar dos hipótesis equivocadas** con datos.
- **Rendimiento**: en una sola sesión, 129→200 FPS en la misma vista y carga
  inicial de 13-20→64-75 FPS, todo con A/B medido y distancia de render
  clavada para que las comparaciones fueran válidas. Ver `PENDIENTES.md §3-ter`.
- **Comentarios de decisión**: densidad del 37 % en `main.cpp`, y explican el
  *porqué* y el bug que lo motivó, no el *qué*. Es un activo poco común.
- **Registro de decisiones asumidas** (`PENDIENTES.md:442-475`): seis entradas
  con decisión, consecuencia aceptada y contrapartida obligatoria. Son ADRs de
  facto, y buenos.

---

## Plan de acción

Ordenado por daño evitado ÷ esfuerzo.

### Ahora (menos de una hora en total)

1. **Añadir los 7 tests huérfanos** a `tests/CMakeLists.txt` y ver cuáles
   pasan. *(C1 — 10 min)*
2. **Una línea en `recargarMallas`** para no soltar candados en vuelo.
   *(C2 — 5 min)*
3. **Quitar o cumplir la promesa de los backups** del diálogo de cierre.
   *(C4 — 15 min)*
4. **ASAN en la configuración Debug**: una línea de CMake. *(I2 — 5 min)*

### Esta semana

5. **CI mínimo** (`.github/workflows`): configurar, compilar Release, `ctest`.
   Es lo que habría detectado C1 y lo que protege todo lo demás. *(2 h)*
6. **`player.dat`: CRC32 real + temp/rename.** Las dos piezas ya existen en el
   repo. *(I1 — 1 h)*
7. **Mutex o array plano para `textures`.** *(C3 — 30 min)*
8. **Atomizar las cinco banderas de `Chunk`.** *(I6 — 30 min)*

### Cuando haya hueco

9. **`/W4` + `/permissive-` en el juego**, y limpiar la tanda de avisos.
10. **Borrar `Chunk::version` y `epocaStreaming`** o implementarlos de verdad.
    Un mecanismo de seguridad que no existe es peor que no tenerlo. *(I5)*
11. **Archivar `documentación/`** fuera del repo o en una carpeta `historico/`.
12. **Retomar el desmonte de `main.cpp`** por donde el plan ya decía:
    `CraftingSystem` primero (es lógica pura y se puede testear al sacarla).
    Y **actualizar la cifra de `PENDIENTES.md`**, que va 60 % desviada.
13. **Medir cobertura** (OpenCppCoverage en Windows) para saber de qué se
    habla cuando se dice «878 tests».
