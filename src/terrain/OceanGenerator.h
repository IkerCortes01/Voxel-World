#ifndef OCEAN_GENERATOR_H
#define OCEAN_GENERATOR_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"

// ============================================================================
// OCEAN GENERATOR - ETAPA 5
// ============================================================================
// RESPONSABILIDAD UNICA: modelar el relieve SUBMARINO.
//
// El perfil batimetrico grueso (abismo -> talud -> plataforma) ya lo produce
// la spline de continentalidad en ClimateGenerator::GetBaseHeight. Este
// modulo anade el DETALLE submarino que la spline no puede dar:
//
//   - Dorsales y fosas oceanicas en el abismo
//   - Rugosidad del talud continental
//   - Ondulacion suave de la plataforma continental
//   - Bancos de arena cerca de la costa
//
// Se mantiene separado del terreno emergido porque los procesos geologicos
// submarinos son distintos: el agua amortigua la erosion, por lo que el
// fondo marino es mas suave y de mayor longitud de onda que la tierra firme.
// ============================================================================

namespace TerrainGen {

class OceanGenerator {
private:
    int seed;

    int seedAbyss()  const { return seed + 1277813; }
    int seedShelf()  const { return seed + 1391477; }
    int seedRidge()  const { return seed + 1505231; }

public:
    static constexpr int SEA_LEVEL = ClimateGenerator::SEA_LEVEL;

    explicit OceanGenerator(int s) : seed(s) {}

    // ========================================================================
    // DETALLE SUBMARINO
    // ========================================================================
    // Devuelve los bloques a SUMAR a la altura base en zonas oceanicas.
    // Devuelve 0 en tierra firme (transicion continua).
    float GetOceanDetail(float x, float z, const ClimateData& c) const {
        // Factor oceanico: 1 en mar abierto, 0 en tierra.
        // El solape con la zona costera hace que el detalle submarino se
        // desvanezca gradualmente al llegar a la playa, sin escalon.
        const float oceanFactor = 1.0f - Noise::smoothstep(0.42f, 0.56f, c.continentalness);
        if (oceanFactor <= 0.001f) return 0.0f;

        float detail = 0.0f;

        // --------------------------------------------------------------------
        // 1. PLATAFORMA CONTINENTAL (continentalness 0.36 - 0.50)
        // --------------------------------------------------------------------
        // Muy plana, con ondulacion larga y suave. Es donde vive la vida
        // marina y donde el agua es poco profunda.
        const float shelfFactor = Noise::smoothstep(0.28f, 0.42f, c.continentalness);
        const float shelfNoise = Noise::fbmSimplex2D(seedShelf(),
                                                     x * 0.0022f,
                                                     z * 0.0022f, 3, 2.0f, 0.5f);
        detail += shelfNoise * 3.5f * shelfFactor;

        // --------------------------------------------------------------------
        // 2. TALUD CONTINENTAL
        // --------------------------------------------------------------------
        // La pendiente abrupta entre plataforma y abismo. Se le anade
        // rugosidad porque los taludes reales estan surcados por canones
        // submarinos.
        const float slopeZone = Noise::smoothstep(0.14f, 0.30f, c.continentalness)
                              * (1.0f - Noise::smoothstep(0.30f, 0.40f, c.continentalness));
        const float slopeNoise = Noise::ridged2D(seedRidge(),
                                                 x * 0.0035f,
                                                 z * 0.0035f, 3, 2.0f, 0.5f);
        // Resta: los canones excavan el talud hacia abajo.
        detail -= slopeNoise * 7.0f * slopeZone;

        // --------------------------------------------------------------------
        // 3. LLANURA ABISAL + DORSALES
        // --------------------------------------------------------------------
        // Fondo profundo: llanura muy plana interrumpida por dorsales
        // (cordilleras submarinas) generadas con ridged noise.
        const float abyssFactor = 1.0f - Noise::smoothstep(0.06f, 0.26f, c.continentalness);
        if (abyssFactor > 0.001f) {
            // Dorsal oceanica: cadena elevada en medio del abismo.
            const float ridge = Noise::ridged2D(seedAbyss(),
                                                x * 0.00075f,
                                                z * 0.00075f, 4, 2.0f, 0.5f);
            const float ridgeMask = Noise::smoothstep(0.55f, 0.90f, ridge);
            detail += ridgeMask * 16.0f * abyssFactor;

            // Fosas: depresiones estrechas y profundas.
            const float trench = Noise::fbmSimplex2D(seedAbyss() + 41,
                                                     x * 0.0009f,
                                                     z * 0.0009f, 3);
            if (trench < -0.55f) {
                // OJO CON EL ORDEN DE LOS EDGES: smoothstep exige edge0<edge1.
                // Escrito como smoothstep(-0.55, -0.95, ...) caia en la rama
                // degenerada y devolvia siempre 0, de modo que las fosas
                // nunca se excavaban. Se invierte el intervalo y el sentido.
                const float trenchDepth = 1.0f - Noise::smoothstep(-0.95f, -0.55f, trench);
                detail -= trenchDepth * 12.0f * abyssFactor;
            }

            // Rugosidad general de la llanura abisal (muy suave).
            const float abyssalNoise = Noise::fbmSimplex2D(seedAbyss() + 83,
                                                           x * 0.004f,
                                                           z * 0.004f, 2);
            detail += abyssalNoise * 2.0f * abyssFactor;
        }

        // --------------------------------------------------------------------
        // 4. BANCOS DE ARENA COSTEROS
        // --------------------------------------------------------------------
        // Justo antes de la playa: pequenos monticulos sumergidos que hacen
        // que la costa no sea una rampa uniforme.
        const float nearShore = Noise::smoothstep(0.40f, 0.48f, c.continentalness)
                              * (1.0f - Noise::smoothstep(0.48f, 0.55f, c.continentalness));
        if (nearShore > 0.001f) {
            const float bars = Noise::billow2D(seedShelf() + 29,
                                               x * 0.012f,
                                               z * 0.012f, 2);
            detail += bars * 2.2f * nearShore;
        }

        return detail * oceanFactor;
    }

    // ========================================================================
    // PROFUNDIDAD DEL AGUA en un punto (utilidad)
    // ========================================================================
    static int GetWaterDepth(int terrainHeight) {
        const int d = SEA_LEVEL - terrainHeight;
        return d > 0 ? d : 0;
    }
};

} // namespace TerrainGen

#endif // OCEAN_GENERATOR_H
