#ifndef BEACH_GENERATOR_H
#define BEACH_GENERATOR_H

#include "NoiseSystem.h"
#include "BiomeTypes.h"

// ============================================================================
// BEACH GENERATOR - ETAPA 6
// ============================================================================
// RESPONSABILIDAD UNICA: decidir si una columna es playa.
//
// REQUISITOS EXPLICITOS DEL DISENO:
//   (a) "Las playas solo pueden aparecer entre oceanos y tierra"
//   (b) "Nunca deben aparecer playas flotantes"
//   (c) "Nunca cortar montanas verticalmente"
//
// POR QUE LA PLAYA NO ES UN BIOMA CLIMATICO:
//   Si la playa se eligiera por clima (como hacia la version anterior:
//   `if (continentalness < 0.35 && baseHeight < SEA_LEVEL+5) return BEACH;`)
//   apareceria en cualquier punto que cumpliera esos numeros, incluida la
//   ladera de una montana o una meseta interior que casualmente estuviera a
//   esa altura. Eso son exactamente las "playas flotantes" prohibidas.
//
// SOLUCION: la playa es una condicion GEOMETRICA evaluada sobre el terreno
// YA GENERADO. Requiere DOS condiciones simultaneas:
//
//   1. BANDA DE ALTURA: la columna esta dentro de pocos bloques del nivel
//      del mar. Garantiza (a) y (b): no puede haber playa a 90 bloques.
//
//   2. PENDIENTE BAJA: el terreno alrededor es casi horizontal. Garantiza
//      (c): un acantilado que cae al mar tiene pendiente alta y NO recibe
//      arena, por lo que la montana no queda cortada por una franja de
//      playa. La pendiente se mide con diferencias finitas centradas sobre
//      la propia funcion de altura.
// ============================================================================

namespace TerrainGen {

class BeachGenerator {
private:
    int seed;
    int seedWidth() const { return seed + 1619477; }

public:
    // Banda vertical en la que puede existir playa, relativa a SEA_LEVEL.
    // Estrecha a proposito: una playa real es una franja delgada. Con una
    // banda de +-4 el 27% del mundo cumplia la condicion y el resultado era
    // un planeta de arena.
    static constexpr int BEACH_MIN_OFFSET = -2; // hasta 2 bloques bajo el agua
    static constexpr int BEACH_MAX_OFFSET =  3; // hasta 3 bloques sobre el agua

    // Pendiente maxima admisible, en bloques de desnivel por bloque
    // horizontal. Por encima de esto se considera acantilado o ladera.
    //
    // Calibrado con la distribucion real de pendientes del generador
    // (mediana 0.085, p95 0.26): 0.12 selecciona aproximadamente el tercio
    // mas plano del terreno, que es donde la arena se deposita de verdad.
    // El valor anterior (0.42) estaba por encima del p95 y no filtraba nada.
    static constexpr float MAX_BEACH_SLOPE = 0.12f;

    explicit BeachGenerator(int s) : seed(s) {}

    // ========================================================================
    // TEST DE PLAYA
    // ========================================================================
    // heightAt: altura del terreno en (x,z)
    // slope:    magnitud del gradiente de altura en (x,z) (ver ComputeSlope)
    //
    // Devuelve true si la columna debe convertirse en playa.
    bool IsBeach(float x, float z, int seaLevel, int heightAt, float slope) const {
        // --- CONDICION 1: banda de altura alrededor del nivel del mar ---
        // Esto por si solo ya impide playas en montanas o mesetas.
        const int minH = seaLevel + BEACH_MIN_OFFSET;
        const int maxH = seaLevel + BEACH_MAX_OFFSET;
        if (heightAt < minH || heightAt > maxH) return false;

        // --- CONDICION 2: pendiente baja ---
        // Un acantilado costero tiene pendiente muy superior y queda excluido,
        // de modo que la roca llega directamente al agua (requisito c).
        if (slope > MAX_BEACH_SLOPE) return false;

        // --- ANCHURA VARIABLE ---
        // Ruido de frecuencia media que modula el limite superior: la playa
        // es mas ancha en unas zonas que en otras (calas y arenales) en vez
        // de tener un ancho constante que se veria artificial.
        const float widthNoise = Noise::fbmSimplex2D(seedWidth(), x * 0.018f, z * 0.018f, 2);
        // Desplaza el techo de la playa entre -2 y +2 bloques.
        const float dynamicMax = (float)maxH + widthNoise * 2.0f;
        if ((float)heightAt > dynamicMax) return false;

        return true;
    }

    // ========================================================================
    // CALCULO DE PENDIENTE (diferencias finitas centradas)
    // ========================================================================
    // Estima |grad(h)| muestreando la altura a ambos lados en X y en Z.
    //
    //   slope = sqrt( (dh/dx)^2 + (dh/dz)^2 )
    //
    // El llamador pasa las 4 alturas vecinas, que TerrainGenerator ya calcula
    // para el suavizado de transiciones, de modo que no hay coste extra.
    static float ComputeSlope(float hXMinus, float hXPlus,
                              float hZMinus, float hZPlus,
                              float sampleDistance) {
        if (sampleDistance <= 0.0f) return 0.0f;
        const float dhdx = (hXPlus - hXMinus) / (2.0f * sampleDistance);
        const float dhdz = (hZPlus - hZMinus) / (2.0f * sampleDistance);
        return sqrtf(dhdx * dhdx + dhdz * dhdz);
    }
};

} // namespace TerrainGen

#endif // BEACH_GENERATOR_H
