#ifndef CLIMATE_GENERATOR_H
#define CLIMATE_GENERATOR_H

#include "NoiseSystem.h"

// ============================================================================
// CLIMATE GENERATOR - ETAPA 1: Mapas Climaticos Independientes
// ============================================================================
// Genera SEIS campos escalares independientes sobre el plano XZ. Cada campo
// usa su propio seed derivado, por lo que no estan correlacionados entre si:
// esto es lo que hace que el mundo tenga variedad real en vez de que todos
// los biomas sigan el mismo patron.
//
//   1. Continentalidad -> oceano vs tierra (la mas importante)
//   2. Temperatura     -> caliente vs frio (con gradiente de latitud)
//   3. Humedad         -> seco vs humedo
//   4. Erosion         -> terreno erosionado/plano vs escarpado
//   5. Weirdness       -> rareza, controla variantes y picos inusuales
//   6. Altura base     -> derivada de continentalidad + erosion via splines
//
// TODOS son de BAJA FRECUENCIA: varian en escalas de cientos o miles de
// bloques, de forma que los biomas resultantes son grandes y coherentes en
// lugar de un mosaico ruidoso.
//
// DETERMINISMO (ETAPA 12): funciones puras de (seed, x, z). Sin estado.
// SIN COSTURAS: se evalua en coordenadas de mundo absolutas.
// ============================================================================

namespace TerrainGen {

// ----------------------------------------------------------------------------
// Datos climaticos de un punto del mundo
// ----------------------------------------------------------------------------
struct ClimateData {
    float continentalness; // [0,1] 0 = abismo oceanico, 1 = interior continental
    float temperature;     // [0,1] 0 = helado, 1 = torrido
    float humidity;        // [0,1] 0 = arido, 1 = selvatico
    float erosion;         // [0,1] 0 = escarpado/montanoso, 1 = erosionado/plano
    float weirdness;       // [0,1] rareza, para variantes
    float baseHeight;      // Altura base del terreno en bloques (antes de relieve)
};

class ClimateGenerator {
private:
    int seed;

    // ------------------------------------------------------------------------
    // SEEDS DERIVADOS
    // ------------------------------------------------------------------------
    // Cada mapa recibe un offset grande y primo para descorrelacionarlos por
    // completo. Si compartieran seed, la temperatura y la humedad tendrian
    // exactamente la misma forma y solo existirian dos biomas.
    int seedContinental() const { return seed + 0;      }
    int seedTemperature() const { return seed + 104729; }
    int seedHumidity()    const { return seed + 224737; }
    int seedErosion()     const { return seed + 350377; }
    int seedWeirdness()   const { return seed + 486187; }
    int seedDetail()      const { return seed + 611953; }

    // ------------------------------------------------------------------------
    // ESCALAS (frecuencias). Menor = estructuras mas grandes.
    // ------------------------------------------------------------------------
    // Continentalidad es la mas baja: los continentes miden ~4000+ bloques.
    static constexpr float CONTINENTAL_SCALE = 0.00035f;
    static constexpr float TEMPERATURE_SCALE = 0.00060f;
    static constexpr float HUMIDITY_SCALE    = 0.00075f;
    static constexpr float EROSION_SCALE     = 0.00090f;
    static constexpr float WEIRDNESS_SCALE   = 0.00110f;

    // Gradiente de latitud: periodo de ~16000 bloques en Z.
    static constexpr float LATITUDE_SCALE    = 0.00006f;

public:
    // Nivel del mar. Referencia global de todo el generador.
    static constexpr int SEA_LEVEL = 62;

    explicit ClimateGenerator(int s) : seed(s) {}

    // ========================================================================
    // 1. CONTINENTALIDAD -> [0,1]
    // ========================================================================
    // Define la forma de los continentes. Es el campo maestro: determina
    // oceano vs tierra y alimenta la altura base.
    //
    // ALGORITMO: FBM con domain warping.
    //   - El warp deforma las masas de tierra para que no parezcan manchas
    //     circulares de ruido; crea peninsulas, golfos y bahias.
    //   - Se usa simplex en vez de perlin para evitar que las costas se
    //     alineen con los ejes X/Z.
    float GetContinentalness(float x, float z) const {
        float wx, wz;
        Noise::warp2D(seedContinental() + 555,
                      x * CONTINENTAL_SCALE,
                      z * CONTINENTAL_SCALE,
                      0.45f, wx, wz);

        // 5 octavas: forma global + detalle de costa.
        const float n = Noise::fbmSimplex2D(seedContinental(), wx, wz, 5, 2.0f, 0.5f);

        // Redistribucion: se expande el rango para que existan tanto oceanos
        // profundos como interiores continentales reales. Sin esta expansion
        // el FBM se concentra en torno a 0.5 y el mundo entero seria costa.
        return Noise::expandContrast(Noise::toUnit(n), 2.6f);
    }

    // ========================================================================
    // 2. TEMPERATURA -> [0,1]
    // ========================================================================
    // Combina un gradiente de latitud (bandas climaticas como en la Tierra)
    // con ruido regional. Sin la latitud, los desiertos y las tundras
    // apareceran mezclados sin logica geografica.
    float GetTemperature(float x, float z) const {
        // Bandas de latitud suaves. Se usa una sinusoide de periodo largo.
        const float latitude = sinf(z * LATITUDE_SCALE) * 0.5f + 0.5f; // [0,1]

        // Variacion regional (masas de aire, corrientes).
        // Se expande el contraste para que existan regiones genuinamente
        // calidas y genuinamente frias, no solo templadas.
        const float regional = Noise::expandContrast(
            Noise::toUnit(Noise::fbmSimplex2D(seedTemperature(),
                                              x * TEMPERATURE_SCALE,
                                              z * TEMPERATURE_SCALE, 4, 2.0f, 0.5f)),
            2.8f);

        // 45% latitud + 55% regional: la latitud da coherencia geografica
        // pero el ruido regional aporta el rango extremo necesario para
        // desiertos y tundras.
        float t = latitude * 0.45f + regional * 0.55f;
        return Noise::clamp(t, 0.0f, 1.0f);
    }

    // ========================================================================
    // 3. HUMEDAD -> [0,1]
    // ========================================================================
    // Ruido propio + efecto costero: cerca del mar llueve mas. Esto crea el
    // patron realista de desiertos en el interior continental y bosques
    // humedos cerca de la costa.
    float GetHumidity(float x, float z, float continentalness) const {
        // Expansion de contraste: sin ella la humedad nunca baja de ~0.42 y
        // el desierto (que exige humedad < 0.32) no puede existir.
        const float base = Noise::expandContrast(
            Noise::toUnit(Noise::fbmSimplex2D(seedHumidity(),
                                              x * HUMIDITY_SCALE,
                                              z * HUMIDITY_SCALE, 4, 2.0f, 0.5f)),
            3.0f);

        // Efecto costero: humedece la costa y RESECA el interior profundo.
        // Se centra en 0 (rango [-0.5, +0.5]) en lugar de solo sumar, para
        // no sesgar la media global hacia arriba: antes este termino hacia
        // que el mundo entero fuese humedo y todo acabase siendo bosque.
        const float inland = Noise::clamp((continentalness - 0.45f) * 1.8f, 0.0f, 1.0f);
        const float coastalBias = (1.0f - inland * inland) - 0.5f; // [-0.5, +0.5]

        float h = base + coastalBias * 0.30f;
        return Noise::clamp(h, 0.0f, 1.0f);
    }

    // ========================================================================
    // 4. EROSION -> [0,1]
    // ========================================================================
    // 0 = roca joven sin erosionar (montanas escarpadas)
    // 1 = terreno muy erosionado (llanuras, valles amplios)
    //
    // Es el campo que decide DONDE puede haber montanas. Al ser un campo
    // continuo y de baja frecuencia, las montanas forman CORDILLERAS
    // conectadas (ETAPA 4) en lugar de picos aislados al azar.
    float GetErosion(float x, float z) const {
        float wx, wz;
        Noise::warp2D(seedErosion() + 313,
                      x * EROSION_SCALE,
                      z * EROSION_SCALE,
                      0.30f, wx, wz);

        // Expansion: hacen falta zonas de erosion genuinamente baja para que
        // se formen cordilleras, y genuinamente alta para llanuras amplias.
        const float n = Noise::fbmSimplex2D(seedErosion(), wx, wz, 4, 2.0f, 0.5f);
        return Noise::expandContrast(Noise::toUnit(n), 2.4f);
    }

    // ========================================================================
    // 5. WEIRDNESS / RAREZA -> [0,1]
    // ========================================================================
    // Campo de variacion que rompe la monotonia: selecciona variantes de
    // bioma y modula los picos. Usa warp fractal para producir estructuras
    // filamentosas impredecibles.
    float GetWeirdness(float x, float z) const {
        float wx, wz;
        Noise::warpFractal2D(seedWeirdness(),
                             x * WEIRDNESS_SCALE,
                             z * WEIRDNESS_SCALE,
                             0.60f, wx, wz);

        const float n = Noise::fbmSimplex2D(seedWeirdness() + 77, wx, wz, 3, 2.0f, 0.5f);
        return Noise::expandContrast(Noise::toUnit(n), 2.2f);
    }

    // ========================================================================
    // 6. ALTURA BASE -> bloques
    // ========================================================================
    // ETAPA 3 / 5: convierte continentalidad en un PERFIL DE ELEVACION
    // realista usando splines. Este es el corazon del sistema de oceanos.
    //
    // El perfil modela, de mar adentro hacia tierra:
    //   abismo -> talud -> plataforma continental -> costa -> tierra baja ->
    //   interior elevado
    //
    // Una rampa lineal (lo que hacia la version anterior) produciria un fondo
    // marino en forma de embudo uniforme y costas identicas en todas partes.
    // La spline crea la plataforma plana y el talud abrupto que se ven en la
    // batimetria real.
    float GetBaseHeight(float x, float z, float continentalness, float erosion) const {
        // --- Perfil de continentalidad -> altura ---
        // PRESUPUESTO VERTICAL: el mundo mide WORLD_HEIGHT=128 bloques.
        // La altura base debe dejar cabecera suficiente para que las
        // montanas (hasta ~58 bloques) no toquen el techo. Por eso el
        // interior continental se queda en ~78 y no en 104: 78 + 58 = 136
        // seguiria pasandose, de modo que MountainGenerator ademas aplica
        // una compresion suave cerca del techo (ver MAX_TERRAIN_HEIGHT).
        static const Spline::Point CONTINENT_SPLINE[] = {
            { 0.00f, 14.0f },  // Abismo oceanico
            { 0.15f, 26.0f },  // Fondo profundo
            { 0.28f, 34.0f },  // Base del talud continental
            { 0.36f, 52.0f },  // Talud: ascenso rapido
            { 0.44f, 56.0f },  // Plataforma continental (poco profunda)
            // Transicion costera RAPIDA: se cruza el nivel del mar en un
            // intervalo estrecho de continentalidad. Si la costa es plana,
            // una fraccion enorme del mundo queda a +-3 bloques del mar y
            // todo se convierte en playa. Esta pendiente concentra la linea
            // de costa en una franja delgada, como en la realidad.
            { 0.50f, 64.0f },  // Linea de costa (SEA_LEVEL=62)
            { 0.58f, 69.0f },  // Llanura costera
            { 0.72f, 74.0f },  // Tierra interior
            { 0.88f, 78.0f },  // Altiplano
            { 1.00f, 82.0f }   // Interior elevado
        };
        constexpr int CONTINENT_POINTS =
            (int)(sizeof(CONTINENT_SPLINE) / sizeof(CONTINENT_SPLINE[0]));

        float height = Spline::eval(CONTINENT_SPLINE, CONTINENT_POINTS, continentalness);

        // --- Modulacion por erosion ---
        // Terreno erosionado (erosion alta) se aplana hacia una altura media.
        // Terreno no erosionado conserva su elevacion y podra recibir montanas.
        // Solo aplica en tierra: no queremos aplanar el relieve submarino.
        if (continentalness > 0.5f) {
            const float landFactor = Noise::smoothstep(0.50f, 0.62f, continentalness);
            const float flattenTarget = 70.0f;
            // erosion=1 -> se acerca a flattenTarget; erosion=0 -> sin cambio.
            const float flattenAmount = erosion * 0.45f * landFactor;
            height = Noise::lerp(height, flattenTarget, flattenAmount);
        }

        // --- Variacion de gran escala ---
        // Rompe la dependencia exclusiva de la continentalidad para que dos
        // zonas con la misma continentalidad no tengan identica altura.
        const float macro = Noise::fbmSimplex2D(seedDetail(),
                                                x * 0.00022f,
                                                z * 0.00022f, 3, 2.0f, 0.5f);
        height += macro * 9.0f;

        return height;
    }

    // ========================================================================
    // AGREGADOR: obtiene todos los campos de una vez
    // ========================================================================
    // Se calculan en el orden correcto de dependencias:
    // continentalidad -> humedad y altura base.
    ClimateData GetClimateData(float x, float z) const {
        ClimateData d;
        d.continentalness = GetContinentalness(x, z);
        d.temperature     = GetTemperature(x, z);
        d.humidity        = GetHumidity(x, z, d.continentalness);
        d.erosion         = GetErosion(x, z);
        d.weirdness       = GetWeirdness(x, z);
        d.baseHeight      = GetBaseHeight(x, z, d.continentalness, d.erosion);
        return d;
    }
};

} // namespace TerrainGen

#endif // CLIMATE_GENERATOR_H
