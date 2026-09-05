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

### Llevárselo a otro sitio

```batch
tools\empaquetar.bat            el PROYECTO fuente   (~15 MB)
tools\empaquetar.bat juego      solo el JUEGO        (~12 MB)
tools\empaquetar.bat todo       los dos
```

Los ZIP salen en `dist\`. Copiar la carpeta a mano mueve **250 MB**, y el 93 %
de eso se regenera solo: `build\` (105 MB), `.git\` (60), `graphify-out\` (35)
y `saves\` (18). El código fuente entero son 3,5 MB.

El paquete de fuente lleva las dependencias vendorizadas, así que en la máquina
de destino se descomprime y se compila sin red. Está verificado de punta a
punta: se descomprime en limpio, compila y pasa los tests.

---

## Estructura

```
src/
  main.cpp            Motor: render, mundo, chunks, UI, guardado
  terrain/            Generación procedural de terreno (modular)
  player/             Character controller (modular)
  audio/              Mezclador de sonido y síntesis de pasos
  ui/                 Texturas de item, renderizado 2D e iconos
  SaveSystem.cpp      Persistencia de mundos
  PalettedStorage.h   Almacenamiento comprimido por paletas
  BlockType.h         Enum de bloques (compartido con tests y paletas)
  WorldName.h         Validación de nombres de mundo
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
| `CaveGenerator.h` | Cuevas por niveles, con bocas naturales en superficie |
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

**Agua con volumen.** Cada celda de agua tiene un nivel de 1 a 8 octavos, y se
dibuja a la altura que le toca: el agua derramada se ve *bajar* por escalones.
El nivel viaja dentro del ID del bloque (`Compuesto::Agua`), así que se guarda
y se carga sin tocar el formato ni gastar memoria aparte.

El agua **no se crea ni se destruye, solo se reparte**: mover agua es restar
de una celda y sumar lo mismo a otra. Por eso es finita — colocar un bloque
dentro del agua la desplaza a los lados en vez de borrarla, y llenar un tazón
saca agua de verdad.

Que el agua sea finita y que los mares sigan siendo mares se sostiene sobre
tres reglas: el agua solo fluye *cuesta abajo* (una superficie plana está en
equilibrio y no se simula), la tierra y la arena **se saturan** al beber (una
orilla se moja una vez y deja de robarle agua al mar; el agua absorbida se
queda en el bloque, no desaparece), y la cola de simulación solo se llena
cuando el jugador toca algo, así que un océano intacto no gasta un solo ciclo
de CPU. La piedra es impermeable, que es lo que permite construir un estanque.

Los mundos guardados antes de esto siguen funcionando: su `BLOCK_WATER` cuenta
como celda llena y se convierte en agua con nivel sola, celda a celda, según
el flujo la toca. No hay migración.

**Las pencas, en la mano.** Cortar un agave con hacha entrega sus hojas: el
pulquero da de 1 a 4 según su etapa, el tequilana 1 o 2 y con **item propio**,
porque su penca es azul plateada mate y la del pulquero verde. Sueltas por el
suelo no se dibujan como cubos ni como calcomanías planas: son un modelo 3D de
**4 px de grosor** sobre los 16 de lado, porque la hoja de agave es carnosa y
como lámina fina no se lee como lo que es.

**Ocote (*Pinus montezumae*).** La cuarta especie de árbol, y la única que no
usa el generador de ramas genérico: su copa y su ramaje son la misma
estructura. El tronco es recto y monopódico —un solo eje que llega hasta la
punta— con un fuste limpio que se lleva la mitad de abajo, porque la copa va
"principalmente en la parte superior". Arriba las ramas salen horizontales en
pisos, alternando cruz y diagonal, y de cada una **cuelgan las acículas**:
mechones de hasta 3 bloques que caen más cerca del tronco y menos hacia la
punta. Eso es lo que da los niveles de la copa —capas de hoja a distintas
alturas en vez de discos planos— y deja ver el esqueleto de ramas entre ellas.
El perfil del radio es una curva que engorda en el vientre y se cierra arriba,
así que la copa sale redondeada y no cónica como la del oyamel. Crece en bosque
templado y montaña; su madera resinosa pesa más que la del pino y menos que la
del encino.

**Cuevas por niveles.** El subsuelo ya no es una nube uniforme de huecos: la
red se organiza en **pisos horizontales** separados por bancos de roca, con
pozos y chimeneas que los comunican. Es como se estructura un sistema kárstico
real, donde cada nivel es el rastro de una antigua posición del nivel freático.
Los pisos se ondulan decenas de bloques a lo largo del mundo, así que ninguno
es una loncha plana. El subsuelo queda hueco al **33 %**, y ninguna altura se
queda sin paso — bajar de un nivel al siguiente siempre es posible.

Se entra por **bocas naturales**: una cada ~14 columnas, con forma de embudo
—ancha arriba donde el techo se desplomó, estrecha abajo donde engancha con la
galería— y borde dentado. El sitio de la boca lo decide un campo 2D, así que
es una propiedad del mapa y se ve desde lejos, no una grieta que aparece según
la altura desde la que se mire.

**Sin lava generada.** El fondo de las cuevas ya no se inunda: dejaba la parte
baja intransitable y convertía el descenso en una carrera de obstáculos. El
bloque y su simulación siguen existiendo —los mundos guardados conservan la
suya y se puede colocar—, lo que desaparece es la lava generada de cero.

**Reparto del mundo.** El desierto ocupa el **15 % de la tierra firme** (antes
el 8 %): se encuentra sin buscarlo, pero sigue siendo minoría clara frente al
bosque (52 %) y las planicies (33 %). Es un mundo templado con desiertos
dentro, no un mundo de arena con parches verdes. Los guijarros sueltos —piedra,
cobre, pedernal— caen a razón de **4 por chunk** en terreno normal y 12 donde
el agua y la gravedad los juntan (riscos, orillas y cauces); bajo tierra siguen
siendo bastante más comunes, porque la cueva es la fuente buena y bajar tiene
que compensar.

**Agave tequilana azul.** La planta del tequila crece en el desierto y en las
laderas secas de montaña, separada del maguey pulquero del Altiplano para que
cada especie tenga su territorio. Se reproduce por rizoma, así que no brota
suelta: se siembra un foco y de él sale una colonia, más dispersa que la del
pulquero. Tiene las tres siluetas de su ciclo de vida — la **roseta** de hojas
rígidas y erectas, azul plateado mate y más ancha que alta (3 m × 2 m); la
**piña** que queda al jimarla; y el **quiote**, el eje floral de 5 m rematado
en candelabro que levanta la planta que nadie cosechó. Unas pocas maduras
nacen ya espigadas: son el hito que se ve desde lejos en el llano.

Una planta que ocupa más de una celda **se dibuja entera desde la de abajo**.
Repartir la geometría entre celdas y recortar cada franja funciona para una
columna, pero no para una roseta: sus pencas cruzan la frontera en diagonal, y
un quad que se sale por arriba en una celda y por abajo en la siguiente no lo
dibujaba ninguna de las dos — se perdía el 40 % de las hojas en las plantas
grandes. Es la misma regla que ya seguía el maguey pulquero.

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

El inventario completo de deuda técnica y evolución pendiente está en
[`docs/PENDIENTES.md`](docs/PENDIENTES.md).

**Formato de guardado.** Versión 3. Los mundos creados con versiones
anteriores se siguen leyendo (hay un decodificador legacy y un test que lo
cubre, y los IDs de bloque viejos se traducen al cargar cada chunk); los
guardados nuevos usan el RLE con escape y CRC32 real.

Al escribir, primero van los datos del chunk y solo después la tabla que
apunta a ellos, de modo que un corte a mitad deja la tabla anterior
señalando el guardado íntegro. Al cerrar la región y en los guardados
explícitos se espera a que el disco confirme la escritura
(`FlushFileBuffers`): sin eso, un corte de luz dejaba archivos del tamaño
correcto llenos de ceros.

Solo se guarda lo que el jugador tocó. El mundo es determinista, así que un
chunk intacto se regenera idéntico y escribirlo no conserva nada.

---

## Créditos

Trabajo de reestructuración y correcciones de rendimiento integradas desde
[Voxel-Genesis](https://github.com/Scram-Consulting/Voxel-Genesis).
