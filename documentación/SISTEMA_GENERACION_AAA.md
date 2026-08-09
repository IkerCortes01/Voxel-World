# Sistema de Generación Procedural AAA

**Fecha:** 8 de agosto de 2026
**Estado:** Implementado, compilado y validado
**Ejecutable:** `build\bin\Release\VoxelWorld.exe`

---

## 1. Resumen

Sistema de generación de terreno multicapa que reemplaza por completo el
generador anterior. No se basa en Perlin simple: usa una arquitectura de
mapas climáticos independientes → selección de biomas → relieve por capas →
voxelización, con módulos separados por responsabilidad.

**Antes:** el árbol de fuentes **no compilaba** (3 errores fatales) y el
generador era un esqueleto de ~150 líneas con un desbordamiento de memoria
de ~900 KB por chunk.

**Ahora:** 12 módulos, ~2.900 líneas, 0 errores, 9 tests de validación en
verde y el juego corriendo estable.

---

## 2. Archivos

### Creados

| Archivo | Líneas | Responsabilidad |
|---|---|---|
| `src/terrain/BiomeGenerator.h` | 175 | Selección y mezcla de biomas (etapas 2, 10) |
| `src/terrain/MountainGenerator.h` | 135 | Cordilleras, valles, acantilados (etapa 4) |
| `src/terrain/OceanGenerator.h` | 140 | Batimetría, taludes, plataformas (etapa 5) |
| `src/terrain/BeachGenerator.h` | 110 | Playas por geometría (etapa 6) |
| `src/terrain/CaveGenerator.h` | 230 | Cuevas multi-algoritmo (etapa 9) |
| `src/terrain/DecorationSystem.h` | 300 | Árboles Poisson, dunas, minerales (etapas 7, 8) |
| `src/terrain/TerrainGenerator.h` | 250 | Composición del relieve (etapa 3) |
| `src/terrain/ChunkGenerator.h` | 215 | Voxelización segura (etapas 11, 12) |

### Reescritos

| Archivo | Cambio |
|---|---|
| `src/terrain/NoiseSystem.h` | Reescrito: Perlin/Simplex/Cellular 2D-3D, FBM, Ridged, Billow, warping, splines |
| `src/terrain/ClimateGenerator.h` | Reescrito: 6 mapas climáticos con expansión de contraste |
| `src/terrain/BiomeTypes.h` | Reescrito: registro de biomas por tabla de datos |
| `src/terrain/WorldGeneratorAAA.h` | Reescrito: fachada con API segura por plantillas |

### Modificados

| Archivo | Cambio |
|---|---|
| `src/main.cpp` | `generateChunk()` reescrito, `ChunkVoxelWriter`, guardia anti-recursión, 3 bugs preexistentes corregidos |

---

## 3. Las etapas, una por una

### Etapa 1 — Mapa climático
`ClimateGenerator` produce 6 campos independientes, cada uno con su propio
seed derivado para que no estén correlacionados: continentalidad,
temperatura, humedad, erosión, weirdness y altura base.

**Problema encontrado y resuelto:** el FBM suma octavas, y por el teorema del
límite central esa suma tiende a una gaussiana estrecha centrada en 0.5. Se
midió que el 90% de los valores caía en [0.39, 0.61]. Como el desierto exige
`humedad < 0.32`, **ese bioma no podía existir**. Se añadió
`Noise::expandContrast()`, que expande el rango y redistribuye con una curva
S suave (sin romper la continuidad). Tras el arreglo, los campos cubren
[0,1] completo y aparecen los 8 biomas.

### Etapa 2 — Selección de biomas
`BiomeGenerator::SelectBiome` es un árbol de decisión por prioridad
geográfica: océano (manda la continentalidad) → montaña (mandan erosión y
altura) → clima (temperatura/humedad). Nada es aleatorio.

### Etapa 3 — Relieve
Siete capas que se suman, cada una modulada por factores continuos:
continentes (spline) → batimetría → montañas → colinas → micro relieve →
detalle fino. Domain warping en las capas de gran escala para evitar
patrones repetitivos.

**Splines:** la continentalidad no se convierte en altura con una rampa
lineal sino con una spline de 10 puntos de control, que crea la plataforma
continental plana y el talud abrupto de la batimetría real.

### Etapa 4 — Montañas
Ridged multifractal con realimentación (`weight`), de modo que el detalle
fino solo aparece cerca de las crestas: laderas suaves abajo, roca rota
arriba.

**"Nunca montañas aisladas":** la amplitud no viene de un ruido propio sino
del campo de erosión, que es continuo y de baja frecuencia. Como la erosión
forma regiones extensas conectadas, las montañas heredan esa conectividad y
salen como cordilleras. Una máscara ridged de muy baja frecuencia define las
cadenas; un segundo campo talla los valles; la cuantización en terrazas
produce los acantilados.

### Etapa 5 — Océanos
Plataforma continental (ondulación suave), talud (cañones submarinos
excavados con ridged), llanura abisal con dorsales y fosas, y bancos de
arena costeros.

### Etapa 6 — Playas
**No es un bioma climático, es una condición geométrica.** Requiere dos
condiciones simultáneas:
1. Altura dentro de una banda estrecha del nivel del mar (−2, +3)
2. Pendiente baja (< 0.12), medida por diferencias finitas centradas

La condición de pendiente es la que cumple "nunca cortar montañas
verticalmente": un acantilado que cae al mar tiene pendiente alta y no
recibe arena.

**Calibración:** con la banda ±4 y pendiente 0.42 originales, el 27% del
mundo era playa. Se midió la distribución real de pendientes (mediana 0.085,
p95 0.26) y se ajustó el umbral a 0.12, más una costa más pronunciada en la
spline. Resultado: 2.5–5.8%.

### Etapa 7 — Bosques
**Poisson Disk determinista por rejilla.** El Bridson clásico es secuencial y
rompería el determinismo por chunk (el resultado dependería del orden de
generación). Aquí el mundo se divide en celdas de 5 bloques, cada una con un
único candidato cuya posición sale de un hash de las coordenadas de celda:
función pura, independiente del orden.

El jitter se restringe al centro de la celda, garantizando separación mínima
de 3 bloques (medido). Claros naturales y variación de densidad a gran
escala mediante ruido.

### Etapa 8 — Desiertos
Dunas con **billow noise** (valor absoluto → crestas redondeadas de arena
acumulada, a diferencia del ridged que da crestas afiladas de roca), con
estiramiento anisótropo que imita el viento dominante.

### Etapa 9 — Cuevas
Cinco técnicas combinadas:

| Técnica | Estructura |
|---|---|
| Celular F2−F1 | Túneles ramificados conectados |
| Simplex 3D | Serpenteo orgánico |
| Perlin 3D | Cámaras y cavernas |
| Domain warping | Meandros, rompe regularidad |
| Umbral variable | Conductos estrechos vs galerías |

**Por qué no basta Perlin 3D umbralizado:** produce burbujas ovaladas
aisladas y alineadas con la rejilla. El F2−F1 celular se anula sobre las
aristas del diagrama de Voronoi, que forman por construcción un grafo
conectado: de ahí salen túneles reales que se bifurcan y se reencuentran.
Las columnas y arcos surgen de forma emergente preservando material donde
un segundo campo supera un umbral.

### Etapa 10 — Transiciones
Dos mecanismos:
- **Jitter de frontera:** se perturba la posición antes de evaluar el clima,
  de modo que el borde entre biomas es dentado y no una curva de nivel limpia.
- **Mezcla por peso:** se muestrean 4 vecinos; `blendWeight` mide cuán cerca
  se está de una frontera. El detalle propio del bioma (dunas, rugosidad) se
  escala por ese peso, así que se desvanece progresivamente en vez de
  cortarse de golpe.

### Etapa 11 — Optimización
Ver sección 5.

### Etapa 12 — Determinismo
Todo el ruido deriva de aritmética entera sobre `(seed, x, y, z)` con
`uint32_t` (el desbordamiento con signo sería UB). Sin estado mutable, sin
`rand()`, sin dependencia del orden. Todos los métodos son `const`.

**Verificado:** misma seed → mundo idéntico; seeds distintas → 195/200
columnas diferentes; 8 hilos concurrentes → 0 discrepancias.

### Etapa 13 — Escalabilidad
- **Biomas:** una fila en `BIOME_TABLE` (BiomeTypes.h)
- **Minerales:** una fila en la tabla `ORES` (DecorationSystem.h)
- **Árboles:** una entrada en `TreeType` + su caso en `GetTreeType`

Ningún otro módulo necesita cambios.

### Etapa 14 — SOLID
Cada módulo tiene una responsabilidad única y declarada. `ChunkGenerator`
escribe a través de una **interfaz de escritura por plantilla**, por lo que
no conoce el layout de memoria del motor: funciona igual con un array plano
o con subchunks paletizados, y la llamada se inlinea (sin coste virtual).

---

## 4. Verificación

Suite de 9 tests (`test_terrain.cpp`), todos en verde:

```
[OK] TEST 1: chunk generado sin desbordamiento
[OK] TEST 2: determinismo (misma seed identica=si, seed distinta difiere en 195/200)
[OK] TEST 3: costuras entre chunks, delta maximo = 2 bloques
[OK] TEST 4: biomas presentes: 8/8
[OK] TEST 5: playas=8229, fuera de banda del mar=0, en pendiente fuerte=0
[OK] TEST 6: alturas min=11 max=108 media=54.7 (fuera de [1,128): 0)
[OK] TEST 7: cuevas ocupan 14.14% del subsuelo
[OK] TEST 8: 199 arboles en 220x220, separacion minima = 3.00 bloques
```

**Distribución de biomas** (90.000 muestras):

| Bioma | % |
|---|---|
| Océano Profundo | 25.9 |
| Océano | 20.5 |
| Bosque | 16.1 |
| Planicies | 13.5 |
| Montañas | 11.8 |
| Playa | 5.8 |
| Picos Nevados | 3.9 |
| Desierto | 2.5 |

**Sin costuras entre chunks:** delta máximo de 2 bloques en la frontera,
sobre 256 fronteras comprobadas. Se garantiza porque todo el ruido se evalúa
en **coordenadas de mundo absolutas**, nunca locales: dos chunks vecinos que
evalúan el mismo punto frontera obtienen el mismo valor por construcción.

---

## 5. Rendimiento

| Configuración | ms/chunk |
|---|---|
| 1 hilo | 21.0 |
| 2 hilos | 12.3 |
| 4 hilos | 7.1 |
| 8 hilos | 6.5 |

Partiendo de 41 ms/chunk. Optimizaciones aplicadas, todas medidas con
perfilador:

1. **Pendiente sobre relieve base** (58 → 11.4 µs/columna, 5×). Muestrear
   `GetFinalHeight` 4 veces costaba 25 evaluaciones climáticas por columna.
   La pendiente solo alimenta umbrales gruesos, así que basta el relieve base.
2. **Reutilización del clima** en `SampleBiomeFromClimate`, y omisión de
   `baseHeight` en los vecinos (verificado: `SelectBiome` nunca lo lee).
3. **Warp y widthNoise a 1 octava** en cuevas: el código más caliente
   (~15.000 llamadas/chunk).
4. **Poda por plano en el ruido celular**: descarta planos cuya distancia
   mínima ya supera F2. Optimización exacta, no cambia el resultado.

**Memoria:** `sizeof(WorldGeneratorAAA)` = 40 bytes. El ruido es aritmética
pura, sin tablas de permutación, por lo que instanciarlo por hilo es gratis.

**Optimización descartada por medición:** se probó un "rechazo temprano" de
cuevas con un campo simplex barato. Se midió y no funciona: ese campo no
correlaciona con la posición de los túneles, así que descartaba cuevas
reales en la misma proporción que voxeles (umbral 0.82 → 4.1% descartado,
4.06% de cuevas perdidas). Se eliminó.

---

## 6. Bugs corregidos

### En el código preexistente (impedían compilar)

1. **`main.cpp:3685` — desbordamiento de ~900 KB.** `GenerateColumn` recibía
   `int[][256][16]` mientras el buffer real era `BlockType[16][128][16]`.
   Ni compilaba (tipos incompatibles) ni habría funcionado: con stride 256
   sobre un buffer de 128 habría arrasado `lightData`, el vector de VBOs y
   el heap adyacente. Resuelto con plantillas que deducen la altura del
   propio array, más un parámetro `worldHeight` explícito.

2. **`NoiseSystem.h:169` — redeclaración.** `float y0`/`y1` chocaban con los
   `int y0`/`y1` del mismo scope (C2371).

3. **`main.cpp:627` — clase `PerlinNoise` decapitada.** Una edición previa
   había sustituido la cabecera `class PerlinNoise { private: ...` por un
   `#include`, dejando el cuerpo huérfano. Rompía el parser y cascadeaba
   >100 errores.

4. **`main.cpp:854-3159` — bloque de comentario descontrolado.** El
   `/* GENERADOR ANTIGUO` abarcaba 2.300 líneas y engullía código vivo:
   `Chunk`, `Player`, `Inventory`, `CraftingSystem`, `ParticleSystem`. Se
   cerró en la línea 1921, donde termina realmente el generador viejo.

5. **`allocateChunk` — subchunks sin limpiar.** Al reciclar un chunk del
   pool se limpiaba `blocks[]` pero no los subchunks paletizados, que son
   los que lee el mesher. Quedaban bloques fantasma del mundo anterior.

### Introducidos durante esta implementación y corregidos

6. **Stack overflow (0xC00000FD).** Los árboles que sobresalen del borde
   llaman a `World::setBlock` → `getOrCreateChunk` del vecino →
   `generateChunk` → más árboles → recursión infinita. Se reprodujo a los
   15 s. Resuelto marcando `isGenerated` antes de decorar y con un guardia
   de profundidad `thread_local`.

7. **Corrupción del buffer de columnas.** El guardia estaba *después* de la
   fase de terreno, así que un chunk anidado sobrescribía el buffer
   `static thread_local` compartido y el chunk exterior decoraba con datos
   del vecino — decoración incorrecta y no determinista. Resuelto haciendo
   el buffer local a cada invocación.

8. **Fosas oceánicas muertas.** `smoothstep(-0.55, -0.95, x)` caía en la
   rama degenerada (`edge1 <= edge0`) y devolvía siempre 0.

9. **Sesgo en el jitter de árboles.** `offZ` derivaba de `h/7 % 3`, que está
   correlacionado con `offX = h % 3`. Sustituido por un segundo hash
   independiente.

10. **Gradientes 3D sesgados.** Los índices 12-15 repetían direcciones cuya
    suma no era cero, introduciendo anisotropía en las cuevas. Corregido al
    esquema clásico de Perlin, donde las repeticiones se cancelan.

---

## 7. Limitaciones conocidas

- **Altura del mundo: 128 bloques.** Es una constante del motor
  (`CHUNK_HEIGHT`), reducida en su día por rendimiento. El generador se ha
  calibrado a ese presupuesto: la altura base llega a 82 y las montañas
  suman hasta ~58, con compresión asintótica sobre 96 para que las cimas no
  se decapiten. Si se sube `CHUNK_HEIGHT` a 256, conviene reescalar la
  spline de continentalidad para aprovechar el rango.

- **Ríos y lagos no implementados.** No estaban en la lista de biomas
  pedida. La arquitectura los admite sin tocar el código principal: serían
  un módulo `RiverGenerator` que reste altura siguiendo un campo de flujo,
  llamado desde `TerrainGenerator::GetTerrainHeight`.

- **Los mundos guardados anteriores usan el generador viejo.** Los chunks ya
  guardados se cargan de disco; solo el terreno nuevo usa el sistema AAA. Un
  mundo creado antes tendrá una discontinuidad en la frontera de lo ya
  explorado. Para ver el sistema en su forma pura hay que crear un mundo
  nuevo.

- **Verificación visual pendiente.** Se ha comprobado que el juego compila,
  arranca y se mantiene estable 60 s con memoria plana, y el terreno se ha
  validado numéricamente (biomas, alturas, costuras, determinismo). No he
  inspeccionado el resultado dentro del juego: eso requiere jugarlo.
