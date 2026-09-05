#ifndef CHUNK_GENERATOR_H
#define CHUNK_GENERATOR_H

#include "TerrainGenerator.h"
#include "CaveGenerator.h"
#include "BiomeTypes.h"
#include "../BloqueCompuesto.h"   // Agua::conOrilla / conCorriente
#include <cmath>                  // fabsf

// ============================================================================
// CHUNK GENERATOR - ETAPA 11 (Optimizacion) + ETAPA 12 (Determinismo)
// ============================================================================
// RESPONSABILIDAD UNICA: convertir datos de terreno en VOXELES concretos
// dentro de un buffer de chunk.
//
// ----------------------------------------------------------------------------
// SEGURIDAD DE MEMORIA
// ----------------------------------------------------------------------------
// La version anterior de este sistema tenia un desbordamiento grave:
// GenerateColumn declaraba `int chunkBlocks[][256][16]` mientras que el
// buffer real del motor es `BlockType[16][128][16]`. El generador escribia
// hasta y=255 con un stride de 256, desbordando ~900 KB fuera del array y
// corrompiendo el resto del struct Chunk.
//
// Aqui se elimina esa clase de bug por construccion:
//   - La altura del mundo es un parametro (worldHeight), no una constante
//     duplicada en dos sitios.
//   - Se escribe a traves de un CALLBACK/interfaz de escritura tipada, de
//     modo que el generador NUNCA toca la memoria del chunk directamente ni
//     necesita conocer su layout.
//   - Todos los bucles verticales estan acotados por worldHeight.
//
// ----------------------------------------------------------------------------
// DETERMINISMO Y THREADING (ETAPAS 11 y 12)
// ----------------------------------------------------------------------------
// Todos los metodos son const y solo dependen de (seed, coordenadas de
// mundo). No hay estado mutable compartido, no hay RNG con estado, no hay
// dependencia del orden de generacion. Esto significa que:
//   - Se puede generar cualquier numero de chunks en paralelo sin locks.
//   - La misma seed produce siempre el mismo mundo.
//   - Un chunk generado ahora es identico al mismo chunk generado despues.
// ============================================================================

namespace TerrainGen {

// ----------------------------------------------------------------------------
// INTERFAZ DE ESCRITURA DE VOXELES
// ----------------------------------------------------------------------------
// Desacopla el generador del layout de memoria del motor. El motor
// implementa este interfaz sobre su propia estructura de chunk (array plano,
// subchunks paletizados, etc.) sin que el generador tenga que saberlo.
//
// Es un template para que la llamada se inline por completo: no hay coste de
// llamada virtual por voxel.
// ----------------------------------------------------------------------------

// Ejemplo de writer sobre un array 3D crudo [X][Y][Z].
// El motor puede usar este o proveer el suyo.
template <typename BlockT, int SX, int SY, int SZ>
struct ArrayChunkWriter {
    BlockT (*blocks)[SY][SZ];

    explicit ArrayChunkWriter(BlockT (*b)[SY][SZ]) : blocks(b) {}

    inline void Set(int lx, int y, int lz, Blocks::Id block) {
        // Guardas defensivas: nunca escribir fuera de rango.
        if (lx < 0 || lx >= SX) return;
        if (y  < 0 || y  >= SY) return;
        if (lz < 0 || lz >= SZ) return;
        blocks[lx][y][lz] = (BlockT)block;
    }

    static constexpr int Height() { return SY; }
};


class ChunkGenerator {
private:
    int seed;
    TerrainGenerator terrain;
    CaveGenerator    caves;

    int seedBedrock() const { return seed + 3327739; }
    int seedSurface() const { return seed + 3441495; }

public:
    static constexpr int SEA_LEVEL = TerrainGenerator::SEA_LEVEL;

    explicit ChunkGenerator(int s)
        : seed(s), terrain(s), caves(s) {}

    const TerrainGenerator& Terrain() const { return terrain; }
    const CaveGenerator&    Caves()   const { return caves;   }

    // ========================================================================
    // GENERAR UNA COLUMNA
    // ========================================================================
    // writer      : destino de escritura (ver ArrayChunkWriter)
    // worldX/Z    : coordenadas de MUNDO (garantiza continuidad entre chunks)
    // lx, lz      : coordenadas LOCALES dentro del chunk
    // worldHeight : altura del mundo en bloques (NO hardcodeada)
    //
    // Devuelve los datos de la columna, que el motor puede usar para la
    // fase de decoracion (arboles, etc.).
    template <typename Writer>
    ColumnData GenerateColumn(Writer& writer,
                              int worldX, int worldZ,
                              int lx, int lz,
                              int worldHeight) const {

        const ColumnData col = terrain.GetColumnData((float)worldX, (float)worldZ);
        const BiomeDefinition& def = GetBiome(col.biome);

        // Altura de superficie acotada al mundo. Esta es la guarda que
        // impide cualquier escritura fuera de rango.
        int surfaceY = col.surfaceHeight;
        if (surfaceY < 1) surfaceY = 1;
        if (surfaceY > worldHeight - 1) surfaceY = worldHeight - 1;

        // --------------------------------------------------------------------
        // 1. BEDROCK (base indestructible, con superficie irregular)
        // --------------------------------------------------------------------
        // Altura variable 1-4 para que no sea una losa plana perfecta.
        const float bedrockNoise = Noise::valueAt2D(seedBedrock(), worldX, worldZ);
        const int bedrockTop = 1 + (int)(bedrockNoise * 3.0f);

        writer.Set(lx, 0, lz, Blocks::BEDROCK);
        for (int y = 1; y <= bedrockTop && y < worldHeight; ++y) {
            writer.Set(lx, y, lz, Blocks::BEDROCK);
        }

        // --------------------------------------------------------------------
        // 2. PIEDRA hasta la superficie
        // --------------------------------------------------------------------
        const int subsurfaceStart = surfaceY - def.subsurfaceDepth;

        for (int y = bedrockTop + 1; y <= surfaceY && y < worldHeight; ++y) {
            // --- ETAPA 9: CUEVAS ---
            // Se consultan ANTES de colocar el bloque: si hay cueva, se deja
            // aire.
            //
            // ⭐ SIN LAGOS DE LAVA.
            //
            // Antes el fondo del mundo (y <= LAVA_LEVEL) se inundaba de lava.
            // Se retiro a peticion de diseño: la lava dejaba el fondo de las
            // cuevas intransitable y convertia el descenso en una carrera de
            // obstaculos en vez de una exploracion.
            //
            // ⚠️ LO QUE **NO** SE HA TOCADO, Y ES DELIBERADO:
            //   - El bloque BLOCK_LAVA sigue existiendo en el enum.
            //   - Su simulacion (updateLavaFlow) sigue viva.
            //   - Los mundos ya guardados conservan la lava que tuvieran.
            // Asi el cambio es REVERSIBLE: si algun dia se quiere lava en el
            // fondo, se vuelve a poner aqui y todo lo demas sigue en su sitio.
            // Lo unico que desaparece es la lava GENERADA de cero.
            if (caves.IsCave((float)worldX, y, (float)worldZ, surfaceY)) {
                writer.Set(lx, y, lz, Blocks::AIR);
                continue;
            }

            Blocks::Id block;

            if (y < subsurfaceStart) {
                // Roca profunda: aqui viven los minerales.
                block = Blocks::STONE;
                const Blocks::Id ore = terrain.Decoration().GetOre(worldX, y, worldZ);
                if (ore != Blocks::AIR) block = ore;
            }
            else if (y < surfaceY) {
                // Capa de subsuelo (tierra bajo la hierba, arena bajo la
                // superficie del desierto...).
                block = col.isOcean ? def.underwaterBlock : def.subsurfaceBlock;
            }
            else {
                // --- SUPERFICIE ---
                if (col.isRiverBed) {
                    // ---- LECHO DE RIO ----
                    // El lecho es de ARENA. La arcilla aparece solo en
                    // vetas escasas dentro del cauce mas profundo, en vez
                    // de tapizar el rio entero: asi conserva valor como
                    // recurso que hay que buscar.
                    //
                    // Dos condiciones simultaneas la hacen rara:
                    //   - estar muy cerca del centro del cauce (>0.80)
                    //   - caer dentro de una veta de ruido de alta frecuencia
                    block = Blocks::SAND;

                    if (col.riverStrength > 0.80f) {
                        const float clayVein = Noise::valueAt2D(
                            seed + 4177291, worldX, worldZ);
                        // ~22% de las columnas del centro -> arcilla.
                        if (clayVein > 0.78f) block = Blocks::CLAY;
                    }
                } else if (col.isOcean) {
                    block = def.underwaterBlock;
                } else {
                    // ⚠️ AQUI HUBO UN ANILLO DE PIEDRA ALREDEDOR DE LAS BOCAS.
                    //
                    // Se retiro a peticion de diseño: se veia como parches de
                    // roca sueltos en mitad del pasto, no como el borde de una
                    // sima. La boca ya se ve sin el -- el agujero es lo que
                    // llama la atencion, no su marco.
                    //
                    // Lo que SI se conserva es que la boca llegue hasta la
                    // superficie sin techo encima, que es lo que de verdad
                    // hacia falta (ver IsCaveEntrance).
                    block = def.surfaceBlock;
                }
            }

            writer.Set(lx, y, lz, block);
        }

        // --------------------------------------------------------------------
        // 3. ENTRADAS A CUEVAS
        // --------------------------------------------------------------------
        // Perforan el techo solido para conectar la red con el exterior.
        //
        // ⭐ EL RANGO LO MANDA LA BOCA, NO EL MARGEN DE LA CUEVA.
        //
        // Antes este bucle recorria solo SURFACE_MARGIN (5) bloques, mientras
        // que IsCaveEntrance estaba escrita para trabajar sobre 22. O sea: se
        // calculaba un embudo de 22 y se perforaban 5. El pozo se quedaba en
        // un mordisco en la superficie que no llegaba a ninguna parte.
        //
        // Ahora los dos hablan del mismo rango, y es la propia funcion la que
        // decide donde para: baja hasta encontrar la galeria (ver
        // ENTRADA_MAX_HONDURA).
        //
        for (int y = surfaceY;
             y >= surfaceY - CaveGenerator::ENTRADA_MAX_HONDURA; --y) {
            if (y < 1 || y >= worldHeight) continue;
            if (caves.IsCaveEntrance((float)worldX, y, (float)worldZ, surfaceY)) {
                writer.Set(lx, y, lz, Blocks::AIR);
            }
        }

        // --------------------------------------------------------------------
        // 4. AGUA (ETAPA 5)
        // --------------------------------------------------------------------
        // Cota hasta la que llega el agua del rio. Se usa mas abajo para que
        // el bucle de aire NO la sobrescriba.
        int riverWaterTop = -1;
        // Rellena desde la superficie del terreno hasta el nivel del mar.
        if (surfaceY < SEA_LEVEL) {
            const int waterTop = (SEA_LEVEL < worldHeight - 1) ? SEA_LEVEL : worldHeight - 1;

            // ⭐ LA FRANJA DE ORILLA: DONDE ROMPEN LAS OLAS
            //
            // Es el agua somera pegada a la playa, no todo el mar. Se
            // reconoce porque el fondo esta a uno o dos bloques bajo la
            // superficie del agua: mas hondo que eso ya es mar abierto y ahi
            // el oleaje no llega al fondo.
            //
            // Solo la celda de ARRIBA de esa franja oscila. Las de debajo se
            // quedan llenas: una ola mueve la lamina superficial, no vacia la
            // columna entera hasta la arena.
            const int hondura = waterTop - surfaceY;   // celdas de agua
            const bool esOrilla = (hondura >= 1 && hondura <= 2);

            for (int y = surfaceY + 1; y <= waterTop; ++y) {
                if (esOrilla && y == waterTop) {
                    // La marca viaja dentro del ID, asi que se guarda con el
                    // chunk y no necesita estructura aparte.
                    writer.Set(lx, y, lz, (Blocks::Id)
                        Compuesto::Agua::conOrilla(
                            Compuesto::Agua::nuevo(Compuesto::Agua::LLENA), true));
                } else {
                    writer.Set(lx, y, lz, Blocks::WATER);
                }
            }
        }
        // ---- AGUA DEL RIO (por encima del nivel del mar) ----
        // El cauce se excavo restando altura, asi que hay que rellenarlo.
        // Se rellena hasta la cota original del terreno (antes de excavar),
        // que es lo que hace que el rio siga la pendiente del valle en vez
        // de quedar como una zanja seca.
        else if (col.isRiverBed) {
            // ---- AGUA DEL RIO (GARANTIZADA) ----
            // Un cauce SIEMPRE lleva agua: como minimo un bloque. Antes la
            // profundidad salia de truncar strength*3.75 a entero, asi que
            // el borde del cauce podia quedar en 0 y el rio se veia seco.
            int depth = (int)(col.riverStrength
                            * TerrainGen::RiverGenerator::RIVER_DEPTH * 0.75f);
            if (depth < 1) depth = 1;          // <- garantia de agua

            const int riverTop = surfaceY + depth;
            const int top = (riverTop < worldHeight - 1) ? riverTop : worldHeight - 1;

            // ⭐ EL RIO NO VA LLENO, Y CORRE
            //
            // Un rio de montaña es agua SOMERA sobre un lecho que se ve. Antes
            // se rellenaba con celdas llenas, asi que un arroyo se veia igual
            // que un trozo de mar: opaco y quieto.
            //
            // Ahora la lamina de arriba lleva nivel bajo -- 2 o 3 octavos
            // segun la fuerza del cauce -- de modo que se transparenta el
            // fondo y se distingue de un vistazo un rio de un lago.
            //
            // El nivel sale de `riverStrength`: el centro del cauce lleva mas
            // agua que la orilla, que es lo que da el perfil de un rio real.
            const int nivelRio = (col.riverStrength > 0.55f) ? 3 : 2;

            // ---- LA DIRECCION DE LA CORRIENTE ----
            //
            // No hay un campo "hacia donde va el rio", pero se puede deducir:
            // el agua baja por el cauce, y el cauce es mas profundo donde
            // `riverStrength` es mayor. Muestreando la fuerza a los cuatro
            // lados se sabe hacia donde desciende el valle.
            //
            // Es una aproximacion --el gradiente de la fuerza, no de la
            // altura-- pero da una corriente COHERENTE a lo largo del cauce,
            // que es lo que importa: dos celdas vecinas del mismo rio apuntan
            // al mismo sitio en vez de contradecirse.
            uint16_t dirCorriente = Compuesto::Agua::SIN_CORRIENTE;
            {
                const float sN = terrain.Rivers().GetRiverStrength((float)worldX, (float)(worldZ - 2), col.climate);
                const float sS = terrain.Rivers().GetRiverStrength((float)worldX, (float)(worldZ + 2), col.climate);
                const float sE = terrain.Rivers().GetRiverStrength((float)(worldX + 2), (float)worldZ, col.climate);
                const float sO = terrain.Rivers().GetRiverStrength((float)(worldX - 2), (float)worldZ, col.climate);

                const float gradZ = sS - sN;   // >0: el cauce crece hacia +Z
                const float gradX = sE - sO;

                if (fabsf(gradX) > fabsf(gradZ)) {
                    dirCorriente = (gradX > 0.0f) ? Compuesto::Agua::HACIA_ESTE
                                                  : Compuesto::Agua::HACIA_OESTE;
                } else if (fabsf(gradZ) > 0.0f) {
                    dirCorriente = (gradZ > 0.0f) ? Compuesto::Agua::HACIA_SUR
                                                  : Compuesto::Agua::HACIA_NORTE;
                }
            }

            for (int y = surfaceY + 1; y <= top; ++y) {
                if (y == top) {
                    // La lamina de arriba: nivel bajo y con corriente.
                    BlockType agua = Compuesto::Agua::nuevo((uint16_t)nivelRio);
                    agua = Compuesto::Agua::conCorriente(agua, dirCorriente);
                    writer.Set(lx, y, lz, (Blocks::Id)agua);
                } else {
                    // Debajo, agua llena: es el cuerpo del cauce.
                    writer.Set(lx, y, lz, Blocks::WATER);
                }
            }
            riverWaterTop = top;   // el bucle de aire debe empezar por encima
        }

        // --------------------------------------------------------------------
        // 5. AIRE por encima
        // --------------------------------------------------------------------
        // BUG CORREGIDO: cuando el rio esta POR ENCIMA del nivel del mar,
        // airStart valia surfaceY+1, que es justo donde se acababa de
        // escribir el agua del cauce. El bucle la borraba entera y el rio
        // aparecia SECO. Ahora el aire empieza por encima del agua.
        int airStart = (surfaceY > SEA_LEVEL) ? surfaceY + 1 : SEA_LEVEL + 1;
        if (riverWaterTop >= 0 && airStart <= riverWaterTop) {
            airStart = riverWaterTop + 1;
        }
        for (int y = airStart; y < worldHeight; ++y) {
            writer.Set(lx, y, lz, Blocks::AIR);
        }

        // --------------------------------------------------------------------
        // 6. NIEVE en picos
        // --------------------------------------------------------------------
        if (def.isSnowy && !col.isOcean && surfaceY + 1 < worldHeight) {
            // Linea de nieve difusa: no todos los picos se cubren a la misma
            // altura exacta.
            const float snowNoise = Noise::fbmSimplex2D(seedSurface(),
                                                        (float)worldX * 0.01f,
                                                        (float)worldZ * 0.01f, 2);
            const float snowLine = 96.0f + snowNoise * 8.0f;
            if ((float)surfaceY > snowLine) {
                writer.Set(lx, surfaceY, lz, Blocks::SNOW);
            }
        }

        return col;
    }
};

} // namespace TerrainGen

#endif // CHUNK_GENERATOR_H
