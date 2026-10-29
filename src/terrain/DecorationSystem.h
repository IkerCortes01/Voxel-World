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
    TREE_OYAMEL,     // Oyamel: conifera alta y conica
    TREE_OCOTE,      // Ocote (Pinus montezumae): fuste largo, copa redondeada
    TREE_OCOTE_CHINO,// Ocote chino (Pinus leiophylla): copa mas tupida
    // --- Los cuatro pinos mexicanos nuevos ---
    TREE_PATULA,     // Pinus patula: alto y esbelto, acicula pendula
    TREE_PINONERO,   // Pinus cembroides: pequeño, de zona seca
    TREE_ALTURAS,    // Pinus hartwegii: achaparrado, limite arboreo
    TREE_PSEUDOSTROBUS // Pinus pseudostrobus: fuste recto y limpio
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
                // Bosque templado: el oyamel sigue mandando, pero comparte con
                // el OCOTE, que es justo el pino de esta franja de clima --
                // los bosques de pino-encino del centro de Mexico. Le cede una
                // cuarta parte de su tramo.
                //
                // Los DOS OCOTES conviven aqui, que es lo que pasa de verdad:
                // en los bosques de pino-encino del centro de Mexico el
                // montezumae y el leiophylla crecen mezclados. El tramo del
                // ocote se reparte entre los dos, asi que la cantidad total de
                // pino no cambia -- lo que cambia es que ahora hay dos.
                //
                // ⭐ Y CON ELLOS EL PATULA Y EL PSEUDOSTROBUS, que son los
                // otros dos pinos de esta misma franja: los bosques de
                // pino-encino del centro de Mexico son mezcla de varias
                // especies de Pinus, no de una sola. El tramo del pino se
                // reparte entre los cuatro, asi que la cantidad TOTAL de pino
                // no cambia -- lo que cambia es que ahora hay cuatro.
                if (r < 0.46f) return TREE_OYAMEL;
                if (r < 0.54f) return TREE_OCOTE;
                if (r < 0.62f) return TREE_OCOTE_CHINO;
                if (r < 0.69f) return TREE_PATULA;
                if (r < 0.75f) return TREE_PSEUDOSTROBUS;
                if (r < 0.85f) return TREE_ENCINO;
                if (r < 0.91f) return TREE_OAK;
                if (r < 0.96f) return TREE_BIRCH;
                return TREE_SMALL_OAK;

            case BIOME_PLAINS:
                // En llanura el ocote es raro: es arbol de ladera y altura.
                //
                // El PIÑONERO si encaja en llanura: es arbol de zona abierta
                // y seca, de porte pequeño, y en el Altiplano crece
                // precisamente en terreno llano y arido. Se lleva un tramo
                // pequeño, como corresponde a un arbol que salpica el paisaje
                // sin formar masa.
                if (r < 0.56f) return TREE_OYAMEL;
                if (r < 0.63f) return TREE_OCOTE;
                if (r < 0.70f) return TREE_PINONERO;
                if (r < 0.84f) return TREE_SMALL_OAK;
                if (r < 0.93f) return TREE_OAK;
                return TREE_ENCINO;

            // ================================================================
            // LOS TRES OCOTALES
            // ================================================================
            // La regla de los tres: la especie que da nombre al bioma se lleva
            // ~75%, y el resto se reparte entre acompanantes.
            //
            // POR QUE NO 100%. Un pinar monoespecifico existe, pero es una
            // PLANTACION: hileras de un solo arbol, y se ve artificial. Un
            // ocotal real tiene la masa dominante de una especie con oyameles y
            // encinos intercalados, y son justo esos arboles distintos los que
            // dejan ver que la masa es de otra cosa. Sin ellos no hay contraste
            // y el bosque se lee plano.
            //
            // El acompanante principal de cada ocotal puro es EL OTRO OCOTE, en
            // proporcion pequena: es lo que hace que al cruzar de un ocotal al
            // siguiente la transicion ya venga anunciada.
            case BIOME_OCOTAL_BLANCO:
                // Pinus montezumae manda. Es el pino de cota alta, asi que su
                // acompanante de altura es el oyamel.
                if (r < 0.75f) return TREE_OCOTE;
                if (r < 0.87f) return TREE_OCOTE_CHINO;
                if (r < 0.95f) return TREE_OYAMEL;
                return TREE_ENCINO;

            case BIOME_OCOTAL_CHINO:
                // Pinus leiophylla manda. Es de cota media y algo mas seca, asi
                // que aqui el acompanante es el ENCINO y no el oyamel: son los
                // bosques de pino-encino de media montana.
                //
                // ⭐ Y AQUI ENTRA EL PIÑONERO.
                //
                // Pinus cembroides es el pino de la zona SECA: matorral arido
                // y ladera caliza. De los biomas que generan arboles, este es
                // el mas seco --pino-encino de media montana-- asi que es su
                // sitio natural. En el desierto no puede estar: ese bioma
                // tiene treeDensity 0 a proposito y no genera ni un arbol.
                if (r < 0.70f) return TREE_OCOTE_CHINO;
                if (r < 0.80f) return TREE_OCOTE;
                if (r < 0.88f) return TREE_PINONERO;
                if (r < 0.95f) return TREE_ENCINO;
                return TREE_OAK;

            case BIOME_OCOTAL_MIXTO:
                // Aqui NO hay especie dominante: es el punto donde los dos
                // rangos se solapan, asi que van a partes iguales (43/43). Es
                // lo que distingue a este bioma de los otros dos y la razon de
                // que exista como bioma propio en vez de como frontera.
                //
                // El reparto simetrico importa: si uno se llevara el 60% se
                // veria como un ocotal puro con intrusos, no como un bosque
                // mezclado.
                if (r < 0.43f) return TREE_OCOTE;
                if (r < 0.86f) return TREE_OCOTE_CHINO;
                if (r < 0.94f) return TREE_OYAMEL;
                return TREE_ENCINO;

            case BIOME_MOUNTAINS:
                // Alta montana: el oyamel domina la cota alta, pero el ocote
                // es EL pino de la montana mexicana y aqui tiene su sitio.
                //
                // El chino sube menos que el blanco: Pinus leiophylla es de
                // cota media, asi que aqui se lleva la parte pequena.
                //
                // ⭐ EL PINO DE LAS ALTURAS TIENE AQUI SU SITIO, Y ES GRANDE.
                //
                // Pinus hartwegii es el arbol que crece MAS ALTO del mundo:
                // forma el limite arboreo de los volcanes mexicanos, por
                // encima de los 4.000 m, donde ya no sube nada mas. Si hay un
                // bioma donde debe ser abundante es este -- en la montana
                // mexicana de verdad es la masa dominante de la cota alta.
                //
                // Se le da una quinta parte larga, a costa del tramo del
                // oyamel y del ocote: los tres comparten la cota alta.
                if (r < 0.44f) return TREE_OYAMEL;
                if (r < 0.66f) return TREE_ALTURAS;
                if (r < 0.80f) return TREE_OCOTE;
                if (r < 0.86f) return TREE_OCOTE_CHINO;
                if (r < 0.91f) return TREE_PSEUDOSTROBUS;
                if (r < 0.96f) return TREE_MOUNTAIN;
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
    // MINERALES: RETIRADOS DEL JUEGO
    // ========================================================================
    // Aqui vivia GetOre() y su tabla de siete vetas (carbon, pirita, desecho,
    // hierro, oro, plata y diamante), que decidia por ruido 3D que voxeles de
    // roca profunda se convertian en mineral.
    //
    // Los bloques de mineral se eliminaron, asi que la funcion entera se va con
    // ellos: ChunkGenerator ya no la llama y el subsuelo se genera como roca
    // limpia. Los IDs siguen reservados en el enum y los mundos guardados
    // convierten sus minerales en piedra al cargar
    // (BlockCompat::mineralAPiedra).
    //
    // Lo que SI sigue vivo son los guijarros sueltos del suelo (pedernal,
    // cobre, goethita, hematite, limonita): son items que se recogen
    // agachandose, no bloques de veta, y no pasaban por aqui.

};

} // namespace TerrainGen

#endif // DECORATION_SYSTEM_H
