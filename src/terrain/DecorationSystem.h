#ifndef DECORATION_SYSTEM_H
#define DECORATION_SYSTEM_H

#include "NoiseSystem.h"
#include "BiomeTypes.h"

// ============================================================================
// DECORATION SYSTEM - ETAPA 7 (Bosques) + ETAPA 13 (Escalabilidad)
// ============================================================================
// RESPONSABILIDAD UNICA: decidir DONDE va cada elemento decorativo
// (arboles, hierba, flores, minerales). NO dibuja nada: solo responde
// preguntas booleanas. El dibujado lo hace el motor.
//
// ----------------------------------------------------------------------------
// POISSON DISK SAMPLING DETERMINISTA (para arboles)
// ----------------------------------------------------------------------------
// PROBLEMA: colocar un arbol cuando `random() < densidad` produce grumos y
// huecos; los arboles se solapan y el bosque se ve sucio.
//
// El Poisson Disk clasico (algoritmo de Bridson) es SECUENCIAL: mantiene una
// lista activa y va rechazando candidatos. Eso es inservible aqui porque
// romperia el determinismo por chunk: el resultado dependeria del orden en
// que se generan los chunks, y dos chunks vecinos generados por hilos
// distintos produciran arboles distintos en la frontera.
//
// SOLUCION: Poisson Disk basado en REJILLA con jitter determinista.
//   - El mundo se divide en celdas de tamano CELL (>= radio minimo deseado).
//   - Cada celda contiene COMO MUCHO un arbol.
//   - La posicion del arbol dentro de la celda se obtiene por hash de las
//     coordenadas de la celda: es una funcion pura, independiente del orden.
//   - Como cada celda tiene un solo punto y las celdas miden CELL bloques,
//     dos arboles nunca estan a menos de ~CELL/2 bloques. Se garantiza la
//     separacion minima sin necesidad de estado global.
//
// Esto da la distribucion "azul" caracteristica del Poisson (sin grumos)
// manteniendo determinismo total y evaluacion O(1) por columna.
// ============================================================================

namespace TerrainGen {

// Especies de arbol. Coinciden conceptualmente con World::TreeSpecies.
enum TreeType : uint8_t {
    TREE_NONE = 0,
    TREE_OAK,
    TREE_PINE,
    TREE_BIRCH,
    TREE_SMALL_OAK,
    TREE_MOUNTAIN,
    TREE_ENCINO,     // Encino: copa ancha y redondeada
    TREE_OYAMEL      // Oyamel: conifera alta y conica
};

class DecorationSystem {
private:
    int seed;

    int seedTree()    const { return seed + 2303929; }
    int seedClearing()const { return seed + 2417687; }
    int seedGrass()   const { return seed + 2531443; }
    int seedFlower()  const { return seed + 2645201; }
    int seedOre()     const { return seed + 2758957; }
    int seedDune()    const { return seed + 2872713; }

public:
    // Tamano de celda del Poisson grid, en bloques.
    // Separacion minima efectiva entre arboles ~ CELL/2.
    static constexpr int TREE_CELL = 5;

    explicit DecorationSystem(int s) : seed(s) {}

    // ========================================================================
    // ETAPA 7: COLOCACION DE ARBOLES (Poisson Disk por rejilla)
    // ========================================================================
    // Devuelve true si en la columna (worldX, worldZ) nace un arbol.
    //
    // Es una funcion PURA: no depende del chunk que la llame ni del orden.
    // Dos chunks vecinos que evaluan la misma columna frontera obtienen el
    // mismo resultado, por lo que no hay arboles cortados ni duplicados.
    bool HasTree(int worldX, int worldZ, BiomeType biome, float slope) const {
        const BiomeDefinition& def = GetBiome(biome);
        if (def.treeDensity <= 0.0f) return false;

        // --- Sin arboles en pendientes fuertes ---
        // Un arbol en una pared vertical se ve flotando.
        if (slope > 0.85f) return false;

        // --- 1. Celda de Poisson ---
        // Division con floor correcto para coordenadas negativas.
        const int cellX = (worldX >= 0) ? (worldX / TREE_CELL)
                                        : ((worldX - TREE_CELL + 1) / TREE_CELL);
        const int cellZ = (worldZ >= 0) ? (worldZ / TREE_CELL)
                                        : ((worldZ - TREE_CELL + 1) / TREE_CELL);

        // --- 2. Punto candidato dentro de la celda (jitter determinista) ---
        // El jitter se restringe a la SUBREGION CENTRAL de la celda, no a la
        // celda completa.
        //
        // POR QUE: si el candidato pudiera caer en cualquier posicion de la
        // celda, dos celdas adyacentes podrian colocar sus arboles en bordes
        // opuestos y quedar a 1 bloque de distancia, perdiendo la propiedad
        // de disco de Poisson (se midio exactamente ese caso: separacion
        // minima 1.0 bloques).
        //
        // Restringiendo el jitter a [MARGIN, TREE_CELL-1-MARGIN], la
        // separacion minima garantizada entre dos arboles de celdas vecinas
        // es 2*MARGIN+1 bloques.
        constexpr int JITTER_MARGIN = 1;
        constexpr int JITTER_RANGE  = TREE_CELL - 2 * JITTER_MARGIN;
        static_assert(JITTER_RANGE >= 1, "TREE_CELL demasiado pequeno para el margen");

        // Dos hashes INDEPENDIENTES para X y Z. Derivar el segundo del
        // primero (p.ej. h/7 % 3) no da independencia estadistica: h y h/7
        // estan correlacionados modulo 3, lo que sesga ciertas combinaciones
        // (offX,offZ) y degrada la distribucion de Poisson que buscamos.
        const uint32_t hX = Noise::rawHash2D(seedTree(),      cellX, cellZ);
        const uint32_t hZ = Noise::rawHash2D(seedTree() + 31, cellX, cellZ);
        const int offX = JITTER_MARGIN + (int)(hX % (uint32_t)JITTER_RANGE);
        const int offZ = JITTER_MARGIN + (int)(hZ % (uint32_t)JITTER_RANGE);

        const int treeX = cellX * TREE_CELL + offX;
        const int treeZ = cellZ * TREE_CELL + offZ;

        // Solo la columna exacta del candidato puede tener arbol.
        if (worldX != treeX || worldZ != treeZ) return false;

        // --- 3. Test de densidad ---
        // El candidato existe; ahora se decide si prospera, segun la
        // densidad del bioma modulada por los claros.
        const float roll = Noise::valueAt2D(seedTree() + 13, cellX, cellZ);

        // --- 4. CLAROS NATURALES ---
        // Ruido de frecuencia media que reduce la densidad en manchas.
        // Produce prados abiertos dentro del bosque en lugar de una masa
        // uniforme de arboles.
        //
        // NOTA: se expande el contraste antes del smoothstep. Sin expandir,
        // el FBM se concentra en [0.4,0.6] y el smoothstep(0.32,0.68) lo
        // aplastaba a valores medios-bajos, dejando clearingFactor tan
        // pequeno que NINGUN arbol superaba el test de densidad.
        const float clearing = Noise::expandContrast(
            Noise::toUnit(Noise::fbmSimplex2D(seedClearing(),
                                              (float)worldX * 0.011f,
                                              (float)worldZ * 0.011f, 3)),
            2.5f);

        // clearing bajo = claro (poca densidad); alto = espesura.
        // El suelo de 0.25 garantiza que incluso en los claros haya algun
        // arbol disperso, en vez de zonas completamente peladas.
        const float clearingFactor = 0.25f + 0.75f * Noise::smoothstep(0.25f, 0.75f, clearing);

        // --- 5. VARIACION DE DENSIDAD a gran escala ---
        // Unas zonas del bosque son mas densas que otras.
        const float densityVar = Noise::toUnit(
            Noise::fbmSimplex2D(seedTree() + 401,
                                (float)worldX * 0.0025f,
                                (float)worldZ * 0.0025f, 2));

        // La densidad del bioma se interpreta como probabilidad POR CELDA
        // de Poisson (no por bloque), por lo que se usa directamente.
        const float effectiveDensity =
            def.treeDensity * clearingFactor * (0.60f + densityVar * 0.8f);

        return roll < effectiveDensity;
    }

    // ========================================================================
    // SELECCION DE ESPECIE
    // ========================================================================
    // La especie depende del bioma y de un hash local, de modo que un bosque
    // tiene mezcla de especies en lugar de ser monoespecifico.
    TreeType GetTreeType(int worldX, int worldZ, BiomeType biome, float temperature) const {
        const float r = Noise::valueAt2D(seedTree() + 777, worldX, worldZ);

        // ⭐ EL OYAMEL ES LA ESPECIE DOMINANTE DEL MUNDO.
        // Antes estaba restringido al bosque frio y a la montana, asi que en
        // bosque templado y llanura —donde esta la mayoria de los arboles— no
        // salia ni uno. Ahora domina en TODOS los biomas con arbolado; las
        // demas especies siguen apareciendo para que el bosque no sea
        // monoespecifico, pero en minoria.
        switch (biome) {
            case BIOME_FOREST:
                if (temperature < 0.35f) {
                    // Bosque frio: practicamente un oyametal.
                    if (r < 0.85f) return TREE_OYAMEL;
                    if (r < 0.94f) return TREE_PINE;
                    return TREE_BIRCH;
                }
                // Bosque templado: el oyamel pasa a ser la especie principal.
                if (r < 0.72f) return TREE_OYAMEL;
                if (r < 0.83f) return TREE_ENCINO;
                if (r < 0.90f) return TREE_OAK;
                if (r < 0.96f) return TREE_BIRCH;
                return TREE_SMALL_OAK;

            case BIOME_PLAINS:
                // Incluso en llanura el oyamel es ya el arbol mas frecuente.
                if (r < 0.65f) return TREE_OYAMEL;
                if (r < 0.82f) return TREE_SMALL_OAK;
                if (r < 0.93f) return TREE_OAK;
                return TREE_ENCINO;

            case BIOME_MOUNTAINS:
                // Alta montana: su habitat por excelencia, casi exclusivo.
                if (r < 0.88f) return TREE_OYAMEL;
                if (r < 0.95f) return TREE_MOUNTAIN;
                return TREE_PINE;

            default:
                return TREE_NONE;
        }
    }

    // ========================================================================
    // HIERBA ALTA
    // ========================================================================
    bool HasTallGrass(int worldX, int worldZ, BiomeType biome) const {
        const BiomeDefinition& def = GetBiome(biome);
        if (def.grassDensity <= 0.0f) return false;

        // Manchas de hierba en vez de distribucion uniforme.
        const float patch = Noise::toUnit(
            Noise::fbmSimplex2D(seedGrass(),
                                (float)worldX * 0.035f,
                                (float)worldZ * 0.035f, 2));
        const float patchFactor = Noise::smoothstep(0.30f, 0.75f, patch);

        const float r = Noise::valueAt2D(seedGrass() + 3, worldX, worldZ);
        return r < def.grassDensity * patchFactor;
    }

    // ========================================================================
    // FLORES
    // ========================================================================
    bool HasFlower(int worldX, int worldZ, BiomeType biome) const {
        const BiomeDefinition& def = GetBiome(biome);
        if (def.flowerDensity <= 0.0f) return false;

        // Las flores crecen en grupos pequenos y bien definidos.
        const float patch = Noise::toUnit(
            Noise::fbmSimplex2D(seedFlower(),
                                (float)worldX * 0.08f,
                                (float)worldZ * 0.08f, 2));
        if (patch < 0.62f) return false;

        const float r = Noise::valueAt2D(seedFlower() + 5, worldX, worldZ);
        return r < def.flowerDensity * 3.0f;
    }

    // ========================================================================
    // ETAPA 8: DUNAS DE DESIERTO
    // ========================================================================
    // Contribucion de altura de las dunas. Se usa BILLOW noise porque su
    // valor absoluto produce crestas redondeadas caracteristicas de la arena
    // acumulada por el viento, a diferencia del FBM (colinas simetricas) o
    // el ridged (crestas afiladas de roca).
    //
    // El domain warping estira las dunas en una direccion preferente,
    // imitando el efecto del viento dominante.
    float GetDuneHeight(float x, float z) const {
        // Estiramiento anisotropo: las dunas son alargadas, no circulares.
        const float sx = x * 0.020f;
        const float sz = z * 0.009f; // menor frecuencia en Z = dunas alargadas

        float wx, wz;
        Noise::warp2D(seedDune(), sx, sz, 0.35f, wx, wz);

        const float dunes = Noise::billow2D(seedDune() + 11, wx, wz, 3, 2.0f, 0.5f);

        // Amplitud modulada: campos de dunas altas alternando con hamadas
        // (desierto rocoso plano).
        const float fieldMask = Noise::toUnit(
            Noise::fbmSimplex2D(seedDune() + 29, x * 0.0015f, z * 0.0015f, 2));
        const float amplitude = Noise::lerp(2.0f, 9.0f,
                                            Noise::smoothstep(0.35f, 0.80f, fieldMask));

        return dunes * amplitude;
    }

    // ========================================================================
    // MINERALES (ETAPA 13: facilmente extensible)
    // ========================================================================
    // Definicion declarativa de una veta mineral.
    struct OreDefinition {
        Blocks::Id block;
        int     minY;
        int     maxY;
        float   frequency;  // escala del ruido: mayor = vetas mas pequenas
        float   threshold;  // mayor = mas raro
    };

    // TABLA DE MINERALES: anadir una fila basta para tener un mineral nuevo.
    //
    // ⚠️ EL ORDEN IMPORTA. El bucle de abajo recorre la tabla DE ATRAS HACIA
    // ADELANTE y se queda con el primero que acierte, de modo que lo que esta
    // al FINAL gana cuando dos vetas se solapan. Por eso los comunes van
    // arriba y los raros abajo: sin eso, una veta de carbon (que ahora es
    // enorme) se tragaria el diamante que hubiera dentro.
    static constexpr int ORE_COUNT = 7;

    // Devuelve el bloque de mineral que corresponde a este voxel, o
    // Blocks::AIR (0) si es piedra normal.
    //
    // Se usa ruido 3D en vez de "vetas" procedurales explicitas porque el
    // ruido produce grupos conectados de forma natural y es O(1) por voxel,
    // sin necesidad de estado ni de generar la veta completa de golpe.
    Blocks::Id GetOre(int worldX, int y, int worldZ) const {
        static const OreDefinition ORES[ORE_COUNT] = {
            //  bloque                minY maxY  freq     thr
            //
            // ⭐ LOS TRES DE ARRIBA SON SUPER COMUNES.
            //
            // El umbral (thr) es lo que manda: cuanto MAS BAJO, mas voxeles
            // pasan el corte y mas grande es la veta. Bajar de 0.62 a 0.38
            // no hace "un poco mas" de carbon, multiplica la superficie de
            // roca que se convierte en mineral -- cavando en cualquier
            // direccion se topa uno con ellos constantemente.
            //
            // Ademas cubren casi todo el rango vertical, asi que no hay que
            // bajar a una profundidad concreta para encontrarlos.
            { Blocks::COAL_ORE,         8, 118, 0.070f, 0.38f },
            { Blocks::PYRITE_ORE,       6, 100, 0.076f, 0.40f },
            { Blocks::SCRAP_METAL,     10, 115, 0.082f, 0.42f },

            // Y estos siguen siendo un hallazgo: umbral alto y franja
            // estrecha. Si fueran igual de comunes, encontrar diamante
            // dejaria de significar nada.
            { Blocks::IRON_ORE,        10,  70, 0.085f, 0.70f },
            { Blocks::GOLD_ORE,         5,  35, 0.100f, 0.80f },
            { Blocks::SILVER_ORE,       5,  45, 0.098f, 0.79f },
            { Blocks::DIAMOND_ORE,      3,  18, 0.115f, 0.86f }
        };

        // ⭐ SE RECORRE DE ABAJO ARRIBA: GANA EL MAS RARO.
        //
        // La tabla va de comun a raro, y este bucle la lee al reves, asi que
        // el PRIMERO en probarse es el diamante y el ultimo el carbon. El
        // primero que acierta se lleva el voxel.
        //
        // Esto importa mucho mas ahora que los comunes son enormes: con el
        // orden al reves, la veta de carbon -- que ocupa casi el 5% de toda
        // la roca -- se tragaba la pirita, el desecho Y el diamante que
        // hubiera dentro. Medido: pirita 0,00% y ni un diamante en 80x80
        // columnas.
        for (int i = ORE_COUNT - 1; i >= 0; --i) {
            const OreDefinition& ore = ORES[i];
            if (y < ore.minY || y > ore.maxY) continue;

            const float n = Noise::fbm3D(seedOre() + i * 3167,
                                         (float)worldX * ore.frequency,
                                         (float)y      * ore.frequency,
                                         (float)worldZ * ore.frequency, 2);
            if (n > ore.threshold) {
                return ore.block;
            }
        }
        return Blocks::AIR;
    }
};

} // namespace TerrainGen

#endif // DECORATION_SYSTEM_H
