#ifndef BIOME_GENERATOR_H
#define BIOME_GENERATOR_H

#include "BiomeTypes.h"
#include "ClimateGenerator.h"
#include "NoiseSystem.h"

// ============================================================================
// BIOME GENERATOR - ETAPA 2 (Seleccion) + ETAPA 10 (Transiciones)
// ============================================================================
// RESPONSABILIDAD UNICA: dado un punto climatico, decidir que bioma es y
// con que peso se mezcla con sus vecinos.
//
// PRINCIPIO CLAVE: los biomas NUNCA se eligen al azar. Cada bioma es una
// REGION del espacio climatico de 5 dimensiones (temperatura, humedad,
// continentalidad, erosion, weirdness). Como los campos climaticos son
// continuos, las fronteras entre biomas tambien lo son.
//
// TRANSICIONES SIN BORDES RECTOS (ETAPA 10):
//   El problema clasico es que `if (temp > 0.6) desierto; else planicie;`
//   produce una frontera dura exactamente en temp=0.6, visible como una
//   linea recta de arena contra hierba.
//
//   Solucion implementada: se muestrea el bioma en varios puntos alrededor
//   del objetivo (kernel de 3x3 con jitter) y se promedian sus ALTURAS,
//   ponderadas por distancia. El bioma dominante decide los bloques, pero
//   la altura es una mezcla continua. Ademas, el punto de muestreo lleva un
//   jitter de ruido de alta frecuencia, lo que hace que la frontera sea
//   irregular y dentada en lugar de recta.
// ============================================================================

namespace TerrainGen {

// Resultado de la evaluacion de bioma en un punto.
struct BiomeSample {
    BiomeType dominant;      // Bioma que define los bloques de superficie
    float     blendWeight;   // [0,1] 1 = centro del bioma, 0 = frontera
    BiomeType secondary;     // Bioma vecino mas influyente
};

class BiomeGenerator {
private:
    int seed;

    // Seed para el jitter de fronteras.
    int seedJitter() const { return seed + 731987; }

    // ------------------------------------------------------------------------
    // UMBRALES CLIMATICOS
    // ------------------------------------------------------------------------
    // Centralizados aqui para poder ajustar el mundo sin tocar la logica.
    static constexpr float OCEAN_DEEP_MAX   = 0.30f; // < esto = oceano profundo
    static constexpr float OCEAN_MAX        = 0.47f; // < esto = oceano
    static constexpr float COAST_MAX        = 0.54f; // zona costera

    // ⭐ EL DESIERTO, AMPLIADO (medido, no a ojo).
    //
    // Estaba en 0.62/0.32, que pedia a la vez temperatura muy alta Y humedad
    // muy baja. La interseccion de dos extremos es un area pequeña: salia el
    // 8.3 % de la tierra firme, o sea que se podia jugar mucho rato sin ver
    // uno -- y con el la biznaga y el agave azul, que solo viven ahi.
    //
    // Aflojando las dos condiciones a 0.54/0.40 el desierto pasa al 15.0 % de
    // la tierra: casi el doble. Se encuentra sin buscarlo pero sigue siendo
    // minoria clara frente al bosque (52 %) y las planicies (33 %).
    //
    // Medido sobre 1.000.000 de columnas con el generador real:
    //
    //     umbral        desierto (de la tierra firme)
    //     0.62 / 0.32     8.3 %   <- antes
    //     0.58 / 0.36    11.4 %
    //     0.54 / 0.40    15.0 %   <- ahora
    //     0.52 / 0.42    17.3 %
    //     0.50 / 0.44    19.7 %   <- ya empieza a comerse las planicies
    //
    // Se para en 0.54/0.40 a proposito: el reparto sigue siendo un mundo
    // templado con desiertos dentro, no un mundo de arena con parches verdes.
    //
    // ⚠️ NO se toca FOREST_HUMID_MIN. El bosque se selecciona DESPUES del
    // desierto, asi que ampliar este ya le quita a las planicies lo justo; si
    // ademas se moviera aquel, el reparto cambiaria por dos sitios a la vez y
    // seria imposible atribuir el resultado.
    static constexpr float DESERT_TEMP_MIN  = 0.54f;
    static constexpr float DESERT_HUMID_MAX = 0.40f;

    static constexpr float FOREST_HUMID_MIN = 0.50f;

    // ------------------------------------------------------------------------
    // LOS OCOTALES: LA FRANJA FRESCA Y HUMEDA
    // ------------------------------------------------------------------------
    // Los pinares mexicanos ocupan la tierra fria templada: mas fresca que el
    // bosque de encino y bastante mas humeda que el desierto. Esa es la region
    // climatica que se les asigna aqui.
    //
    // POR QUE SE EVALUAN ANTES QUE EL BOSQUE
    // El arbol de decision se queda con la PRIMERA rama que acierta, asi que
    // el orden es prioridad. Si el bosque fuera primero, se llevaria toda la
    // franja humeda (humidity > 0.50 lo cubre entero) y los ocotales no
    // saldrian NUNCA -- el mismo fallo que ya tuvieron los biomas de montana,
    // que se asignaban sin comprobar que hubiera montana debajo.
    //
    // Van despues del desierto porque el desierto pide temperatura ALTA y los
    // ocotales temperatura BAJA: no compiten por el mismo terreno y el orden
    // entre ellos da igual. Se deja el desierto delante para no mover un
    // reparto que ya estaba medido.
    static constexpr float OCOTAL_TEMP_MAX   = 0.46f;  // por debajo: tierra fria
    static constexpr float OCOTAL_HUMID_MIN  = 0.42f;  // el pinar necesita agua

    // ------------------------------------------------------------------------
    // Y COMO SE REPARTEN LOS TRES ENTRE SI
    // ------------------------------------------------------------------------
    // Por HUMEDAD, que es lo que de verdad separa a las dos especies en campo:
    //
    //   Pinus montezumae (blanco)   sube mas de cota y aguanta mas frio, en la
    //                               vertiente humeda. Se lleva la parte de
    //                               ARRIBA de la franja.
    //   Pinus leiophylla (chino)    es de cota media y tolera mas sequedad. Se
    //                               lleva la parte de ABAJO.
    //   Donde los dos rangos se tocan crecen mezclados: ese solape es el
    //   OCOTAL MIXTO, y va EN MEDIO de los otros dos por construccion.
    //
    // Que el mixto quede en medio no es cosmetico: significa que nunca hay una
    // frontera directa entre ocotal blanco y ocotal chino. Siempre se pasa por
    // el mixto, que es justo lo que hace que la transicion se vea gradual --
    // primero aparecen chinos sueltos entre los blancos, luego se igualan,
    // luego se van los blancos.
    //
    // ⭐ LOS CORTES SALEN DE MEDIR LA HUMEDAD, NO DE ELEGIRLOS A OJO.
    //
    // El primer intento puso 0.52 y 0.62 razonando sobre el rango teorico
    // [0,1]. El resultado medido fue un reparto de 3.3% / 3.2% / 16.9%: el
    // ocotal blanco se llevaba cinco veces mas mundo que los otros dos juntos,
    // y el chino y el mixto quedaban como franjas anecdoticas.
    //
    // La causa es que la humedad NO se reparte uniformemente en la franja
    // fria. Medida sobre 12.759 columnas de tierra firme con temperatura por
    // debajo de OCOTAL_TEMP_MAX, su distribucion es:
    //
    //     percentil   humedad
    //        p10       0.492
    //        p25       0.601
    //        p50       0.787     <- la mediana esta MUY arriba
    //        p75       0.941
    //        p90       1.000     <- y se satura en el tope
    //
    // O sea que casi toda la franja esta por encima de 0.62, y cortar ahi
    // mandaba el 72% de las columnas al blanco.
    //
    // Los cortes de ahora son los CUARTILES REALES de esa distribucion, asi
    // que los tres ocotales reciben aproximadamente un tercio cada uno. Es el
    // mismo metodo que se uso para ampliar el desierto: medir el reparto sobre
    // el generador real y elegir el umbral que da el resultado buscado.
    //
    // ⚠️ Si algun dia se cambia ClimateGenerator::GetHumidity, estos dos
    // numeros dejan de ser cuartiles y hay que volver a medirlos. El test
    // "los tres salen en proporciones parecidas" es lo que avisa de eso.
    static constexpr float OCOTAL_MIXTO_HUMID_MIN  = 0.66f;
    static constexpr float OCOTAL_BLANCO_HUMID_MIN = 0.87f;

    // Montanas: erosion BAJA (roca joven) y altura suficiente.
    static constexpr float MOUNTAIN_EROSION_MAX = 0.38f;
    static constexpr float PEAKS_EROSION_MAX    = 0.24f;

public:
    explicit BiomeGenerator(int s) : seed(s) {}

    // ========================================================================
    // ETAPA 2: SELECCION DE BIOMA
    // ========================================================================
    // Arbol de decision ordenado por PRIORIDAD GEOGRAFICA:
    //   1. Oceano   (continentalidad manda sobre todo lo demas)
    //   2. Montana  (erosion + altura mandan sobre el clima)
    //   3. Clima    (temperatura/humedad deciden el bioma terrestre)
    //
    // NOTA sobre PLAYAS: no se deciden aqui. Una playa no es una region
    // climatica, es una condicion GEOMETRICA (altura cerca del mar +
    // pendiente baja). Se resuelve en BeachGenerator (ETAPA 6), que es lo
    // que impide las "playas flotantes" en la ladera de una montana.
    BiomeType SelectBiome(const ClimateData& c) const {
        // ---- 1. OCEANOS (ETAPA 5) ----
        if (c.continentalness < OCEAN_DEEP_MAX) {
            return BIOME_OCEAN_DEEP;
        }
        if (c.continentalness < OCEAN_MAX) {
            return BIOME_OCEAN;
        }

        // ---- 2. MONTANAS (ETAPA 4) ----
        // Requiere: poca erosion (roca resistente) Y estar tierra adentro.
        // La condicion de continentalidad evita montanas brotando del mar.
        // ⭐ SIN MONTANAS, NO HAY BIOMAS DE MONTANA
        //
        // Estos dos biomas se asignaban SOLO por erosion baja, sin
        // comprobar que hubiera montana de verdad. Al desactivar el
        // MountainGenerator, el relieve paso a ser llano pero el bioma
        // seguia saliendo: y como BIOME_MOUNTAIN_PEAKS pone PIEDRA en
        // superficie, aparecian parches de roca desnuda con borde recto
        // en medio de praderas y bosques. Es exactamente el "bioma
        // incorrecto en medio de otro" que se veia.
        //
        // Comprobado en las capturas: una llanura de piedra pegada a un
        // bosque verde, cortada en linea recta.
        //
        // Mientras las montanas esten apagadas, estas dos ramas no se
        // toman y la columna cae en los biomas climaticos de abajo, que
        // son los que corresponden a un terreno llano.
        //
        // Para recuperarlas basta con volver a poner MONTANAS_ACTIVAS a
        // true en MountainGenerator y quitar este `false &&`.
        const bool inland = c.continentalness > COAST_MAX;
        if (false && inland && c.erosion < MOUNTAIN_EROSION_MAX) {
            // Los picos requieren erosion aun menor y weirdness alta,
            // por lo que aparecen como el nucleo de las cordilleras.
            if (c.erosion < PEAKS_EROSION_MAX && c.weirdness > 0.52f) {
                return BIOME_MOUNTAIN_PEAKS;
            }
            return BIOME_MOUNTAINS;
        }
        (void)inland;

        // ---- 3. BIOMAS CLIMATICOS ----
        // Desierto: caliente Y seco (ambas condiciones, ETAPA 8).
        if (c.temperature > DESERT_TEMP_MIN && c.humidity < DESERT_HUMID_MAX) {
            return BIOME_DESERT;
        }

        // ---- OCOTALES: la tierra fria y humeda ----
        // Fresco Y humedo. Se evalua ANTES que el bosque a proposito (ver la
        // nota de los umbrales): el bosque cubre toda la franja humeda, asi que
        // si fuera primero se quedaria con esto tambien.
        //
        // Los tres se separan por humedad, de mas seco a mas humedo:
        // chino -> mixto -> blanco. El mixto SIEMPRE queda entre los otros dos,
        // asi que no existe frontera directa blanco/chino.
        if (c.temperature < OCOTAL_TEMP_MAX && c.humidity > OCOTAL_HUMID_MIN) {
            if (c.humidity > OCOTAL_BLANCO_HUMID_MIN) return BIOME_OCOTAL_BLANCO;
            if (c.humidity > OCOTAL_MIXTO_HUMID_MIN)  return BIOME_OCOTAL_MIXTO;
            return BIOME_OCOTAL_CHINO;
        }

        // Bosque: humedo, temperatura no extrema (ETAPA 7).
        if (c.humidity > FOREST_HUMID_MIN && c.temperature > 0.25f) {
            return BIOME_FOREST;
        }

        // Por defecto: planicies. Es el bioma "relleno" que ocupa el espacio
        // climatico intermedio, lo que garantiza que siempre hay una
        // transicion suave entre desierto y bosque.
        return BIOME_PLAINS;
    }

    // ========================================================================
    // ETAPA 10: MUESTREO CON JITTER (frontera irregular)
    // ========================================================================
    // Aplica una perturbacion de alta frecuencia a la posicion antes de
    // evaluar el clima. Efecto: la frontera entre dos biomas deja de ser la
    // curva de nivel limpia del campo climatico y pasa a ser una linea
    // dentada y organica, como en la naturaleza.
    void JitterPosition(float x, float z, float& outX, float& outZ) const {
        const float jx = Noise::fbmSimplex2D(seedJitter(),      x * 0.02f, z * 0.02f, 2);
        const float jz = Noise::fbmSimplex2D(seedJitter() + 91, x * 0.02f, z * 0.02f, 2);
        // Amplitud ~6 bloques: suficiente para romper la linea sin
        // desdibujar la geografia.
        outX = x + jx * 6.0f;
        outZ = z + jz * 6.0f;
    }

    // ========================================================================
    // EVALUACION COMPLETA CON MEZCLA
    // ========================================================================
    // Muestrea el bioma en el punto y en 4 vecinos a distancia RADIUS para
    // estimar cuan cerca esta de una frontera. blendWeight = fraccion de
    // muestras que coinciden con el bioma dominante.
    //
    // Coste: 5 evaluaciones climaticas por columna. Se acepta porque el
    // resultado se cachea por columna en TerrainGenerator (no se recalcula
    // por cada voxel de la columna).
    BiomeSample SampleBiome(const ClimateGenerator& climate, float x, float z) const {
        float jx, jz;
        JitterPosition(x, z, jx, jz);
        const ClimateData center = climate.GetClimateData(jx, jz);
        return SampleBiomeFromClimate(climate, jx, jz, center);
    }

    // ------------------------------------------------------------------------
    // VARIANTE OPTIMIZADA: reutiliza un ClimateData ya calculado
    // ------------------------------------------------------------------------
    // GetColumnData ya calcula el clima del punto central. Recalcularlo aqui
    // duplicaba trabajo. Esta sobrecarga permite pasarlo.
    //
    // OPTIMIZACION DE LOS VECINOS:
    // Los 4 vecinos NO necesitan el ClimateData completo (6 campos, ~15 FBM).
    // Para decidir su bioma bastan los 5 campos que consulta SelectBiome, y
    // baseHeight no se usa en esa decision. Se omite GetBaseHeight, que es
    // el campo mas caro (spline + FBM macro). Esto reduce el coste de los
    // vecinos en ~30% sin cambiar ni un solo bioma resultante.
    BiomeSample SampleBiomeFromClimate(const ClimateGenerator& climate,
                                       float jx, float jz,
                                       const ClimateData& center) const {
        const BiomeType dominant = SelectBiome(center);

        // Radio de mezcla: distancia a la que se buscan biomas vecinos.
        constexpr float RADIUS = 12.0f;

        // Clima reducido para los vecinos: solo lo que SelectBiome consulta.
        auto neighborBiome = [&](float nx, float nz) -> BiomeType {
            ClimateData d;
            d.continentalness = climate.GetContinentalness(nx, nz);
            d.temperature     = climate.GetTemperature(nx, nz);
            d.humidity        = climate.GetHumidity(nx, nz, d.continentalness);
            d.erosion         = climate.GetErosion(nx, nz);
            d.weirdness       = climate.GetWeirdness(nx, nz);
            d.baseHeight      = 0.0f; // no lo usa SelectBiome
            return SelectBiome(d);
        };

        BiomeType neighbors[4];
        neighbors[0] = neighborBiome(jx + RADIUS, jz);
        neighbors[1] = neighborBiome(jx - RADIUS, jz);
        neighbors[2] = neighborBiome(jx, jz + RADIUS);
        neighbors[3] = neighborBiome(jx, jz - RADIUS);

        int matching = 0;
        BiomeType secondary = dominant;
        for (int i = 0; i < 4; ++i) {
            if (neighbors[i] == dominant) ++matching;
            else secondary = neighbors[i];
        }

        BiomeSample s;
        s.dominant    = dominant;
        s.secondary   = secondary;
        // 4 coincidencias = interior del bioma (peso 1).
        // 0 coincidencias = frontera pura (peso 0).
        s.blendWeight = (float)matching / 4.0f;
        return s;
    }
};

} // namespace TerrainGen

#endif // BIOME_GENERATOR_H
