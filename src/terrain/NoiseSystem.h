#ifndef NOISE_SYSTEM_H
#define NOISE_SYSTEM_H

#include <cmath>
#include <cstdint>
#include <algorithm>

// ============================================================================
// SISTEMA DE RUIDO AAA
// ============================================================================
// ETAPA 3 / 9 del diseno: primitivas de ruido para relieve y cuevas.
//
// Contenido:
//   - Hash entero determinista (sin tablas de permutacion, sin estado)
//   - Perlin 2D/3D con gradientes normalizados
//   - OpenSimplex2-style 2D/3D (simplex sin patentes, kernel esferico)
//   - Ruido celular / Worley (F1 y F2, para cuevas y continentes)
//   - Fractales: FBM, Ridged multifractal, Billow
//   - Domain warping (simple y fractal)
//
// GARANTIA DE DETERMINISMO (ETAPA 12):
//   Todo el ruido deriva de aritmetica entera sobre (seed, x, y, z).
//   No hay estado mutable, no hay rand(), no hay dependencia de orden de
//   evaluacion ni de threads. La misma seed produce el mismo mundo siempre.
//   Todos los metodos son const y por tanto seguros desde multiples hilos.
//
// GARANTIA DE CONTINUIDAD (sin costuras entre chunks):
//   Las funciones se evaluan en COORDENADAS DE MUNDO absolutas, nunca en
//   coordenadas locales del chunk. El ruido es una funcion matematica pura
//   del punto del mundo, por lo que dos chunks vecinos que evaluan el mismo
//   punto frontera obtienen exactamente el mismo valor.
// ============================================================================

namespace TerrainGen {

// ============================================================================
// HASH ENTERO DETERMINISTA
// ============================================================================
// Basado en el esquema de FastNoiseLite: multiplicacion por primos grandes
// seguida de un avalanche final. Elegido sobre las tablas de permutacion
// clasicas de Perlin porque:
//   1. No requiere memoria ni inicializacion (importante con muchos hilos).
//   2. No tiene el periodo de 256 de la tabla clasica, que causa repeticion
//      visible en mundos grandes.
//   3. Es reproducible bit a bit en cualquier plataforma usando enteros
//      sin signo de 32 bits (el desbordamiento con signo seria UB).
// ============================================================================

namespace detail {

    constexpr uint32_t PRIME_X = 0x1DE5B9EFu; // 501125321
    constexpr uint32_t PRIME_Y = 0x43C7A26Du; // 1136930381
    constexpr uint32_t PRIME_Z = 0x668B6E2Fu; // 1720413743

    // Avalanche final: mezcla todos los bits de entrada en todos los de salida.
    inline uint32_t avalanche(uint32_t h) {
        h ^= h >> 16;
        h *= 0x7FEB352Du;
        h ^= h >> 15;
        h *= 0x846CA68Bu;
        h ^= h >> 16;
        return h;
    }

    inline uint32_t hash2D(int seed, int x, int y) {
        uint32_t h = (uint32_t)seed;
        h ^= (uint32_t)x * PRIME_X;
        h ^= (uint32_t)y * PRIME_Y;
        return avalanche(h);
    }

    inline uint32_t hash3D(int seed, int x, int y, int z) {
        uint32_t h = (uint32_t)seed;
        h ^= (uint32_t)x * PRIME_X;
        h ^= (uint32_t)y * PRIME_Y;
        h ^= (uint32_t)z * PRIME_Z;
        return avalanche(h);
    }

    // Float determinista en [0,1) a partir de un hash.
    inline float hashToUnit(uint32_t h) {
        return (float)(h & 0x00FFFFFFu) / 16777216.0f;
    }

    // Float determinista en [-1,1).
    inline float hashToSigned(uint32_t h) {
        return hashToUnit(h) * 2.0f - 1.0f;
    }

    // Gradientes 2D: 8 direcciones uniformemente distribuidas en el circulo.
    // Se usan vectores unitarios reales (no la variante sesgada de Perlin
    // original) para que el ruido no tenga direcciones privilegiadas.
    inline float grad2D(uint32_t h, float x, float y) {
        // 8 direcciones: ejes + diagonales normalizadas
        static const float GRADS[16] = {
             1.0f,        0.0f,
            -1.0f,        0.0f,
             0.0f,        1.0f,
             0.0f,       -1.0f,
             0.70710678f, 0.70710678f,
            -0.70710678f, 0.70710678f,
             0.70710678f,-0.70710678f,
            -0.70710678f,-0.70710678f
        };
        const int idx = (int)(h & 7u) << 1;
        return GRADS[idx] * x + GRADS[idx + 1] * y;
    }

    // Gradientes 3D: las 12 aristas del cubo.
    //
    // Los indices 12-15 repiten 4 de las 12 aristas porque hay que rellenar
    // una tabla de 16 entradas con 12 direcciones. La eleccion NO es
    // arbitraria: Perlin demostro que repitiendo concretamente
    // (x+y), (-x+y), (y+z), (-y-z) el sesgo resultante se cancela, porque
    // esos cuatro vectores forman dos pares opuestos y su suma es cero.
    //
    // Una version anterior repetia (x+y), (-y+z), (-x+y), (-y-z), cuya suma
    // NO es cero, introduciendo una direccion privilegiada y por tanto
    // anisotropia visible en las cuevas.
    inline float grad3D(uint32_t h, float x, float y, float z) {
        switch (h & 15u) {
            case  0: return  x + y;
            case  1: return -x + y;
            case  2: return  x - y;
            case  3: return -x - y;
            case  4: return  x + z;
            case  5: return -x + z;
            case  6: return  x - z;
            case  7: return -x - z;
            case  8: return  y + z;
            case  9: return -y + z;
            case 10: return  y - z;
            case 11: return -y - z;
            // Repeticiones que se cancelan entre si:
            case 12: return  x + y;
            case 13: return -x + y;
            case 14: return  y + z;
            default: return -y - z;
        }
    }

    // Quintic fade de Perlin: 6t^5 - 15t^4 + 10t^3.
    // Tiene primera y segunda derivada nulas en 0 y 1, lo que elimina los
    // artefactos de "rejilla" visibles con la interpolacion cubica original.
    inline float fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    inline int fastFloor(float f) {
        const int i = (int)f;
        return f < (float)i ? i - 1 : i;
    }

} // namespace detail


// ============================================================================
// NOISE - Primitivas de ruido puras (funciones estaticas, sin estado)
// ============================================================================
// Todas las funciones devuelven aproximadamente [-1, 1] salvo que se indique.
// La frecuencia NO se aplica internamente: el llamador escala las coordenadas.
// Esto evita el bug clasico de aplicar la frecuencia dos veces al componer
// fractales.
// ============================================================================
class Noise {
public:

    // ------------------------------------------------------------------------
    // PERLIN 2D -> [-1, 1]
    // ------------------------------------------------------------------------
    // Uso: capas suaves y continuas (macro relieve, mapas climaticos).
    // Justificacion: gradiente continuo, derivada continua, barato.
    static float perlin2D(int seed, float x, float y) {
        const int x0 = detail::fastFloor(x);
        const int y0 = detail::fastFloor(y);
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        const float xs = x - (float)x0;
        const float ys = y - (float)y0;

        const float xf = detail::fade(xs);
        const float yf = detail::fade(ys);

        const float n00 = detail::grad2D(detail::hash2D(seed, x0, y0), xs,        ys);
        const float n10 = detail::grad2D(detail::hash2D(seed, x1, y0), xs - 1.0f, ys);
        const float n01 = detail::grad2D(detail::hash2D(seed, x0, y1), xs,        ys - 1.0f);
        const float n11 = detail::grad2D(detail::hash2D(seed, x1, y1), xs - 1.0f, ys - 1.0f);

        const float ix0 = detail::lerp(n00, n10, xf);
        const float ix1 = detail::lerp(n01, n11, xf);

        // Normalizacion: los gradientes unitarios dan |valor| <= sqrt(2)/2.
        return detail::lerp(ix0, ix1, yf) * 1.41421356f;
    }

    // ------------------------------------------------------------------------
    // PERLIN 3D -> [-1, 1]
    // ------------------------------------------------------------------------
    // Uso: cuevas, overhangs, densidad volumetrica.
    // NOTA: esta funcion corrige el bug de redeclaracion de la version previa
    // (donde `float y0` chocaba con `int y0` en el mismo scope). Aqui los
    // acumuladores intermedios se llaman iy0/iy1.
    static float perlin3D(int seed, float x, float y, float z) {
        const int x0 = detail::fastFloor(x);
        const int y0 = detail::fastFloor(y);
        const int z0 = detail::fastFloor(z);
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;
        const int z1 = z0 + 1;

        const float xs = x - (float)x0;
        const float ys = y - (float)y0;
        const float zs = z - (float)z0;

        const float xf = detail::fade(xs);
        const float yf = detail::fade(ys);
        const float zf = detail::fade(zs);

        const float n000 = detail::grad3D(detail::hash3D(seed, x0, y0, z0), xs,        ys,        zs);
        const float n100 = detail::grad3D(detail::hash3D(seed, x1, y0, z0), xs - 1.0f, ys,        zs);
        const float n010 = detail::grad3D(detail::hash3D(seed, x0, y1, z0), xs,        ys - 1.0f, zs);
        const float n110 = detail::grad3D(detail::hash3D(seed, x1, y1, z0), xs - 1.0f, ys - 1.0f, zs);
        const float n001 = detail::grad3D(detail::hash3D(seed, x0, y0, z1), xs,        ys,        zs - 1.0f);
        const float n101 = detail::grad3D(detail::hash3D(seed, x1, y0, z1), xs - 1.0f, ys,        zs - 1.0f);
        const float n011 = detail::grad3D(detail::hash3D(seed, x0, y1, z1), xs,        ys - 1.0f, zs - 1.0f);
        const float n111 = detail::grad3D(detail::hash3D(seed, x1, y1, z1), xs - 1.0f, ys - 1.0f, zs - 1.0f);

        const float ix00 = detail::lerp(n000, n100, xf);
        const float ix10 = detail::lerp(n010, n110, xf);
        const float ix01 = detail::lerp(n001, n101, xf);
        const float ix11 = detail::lerp(n011, n111, xf);

        const float iy0 = detail::lerp(ix00, ix10, yf);
        const float iy1 = detail::lerp(ix01, ix11, yf);

        return detail::lerp(iy0, iy1, zf) * 1.1547005f;
    }

    // ------------------------------------------------------------------------
    // OPENSIMPLEX2-STYLE 2D -> [-1, 1]
    // ------------------------------------------------------------------------
    // Uso: capas donde Perlin mostraria alineacion con los ejes.
    // Justificacion: el simplex usa una rejilla triangular, que no tiene
    // direcciones privilegiadas; produce formas mas organicas. Se usa el
    // kernel esferico (0.5 - r^2) sin patentes.
    static float simplex2D(int seed, float x, float y) {
        const float F2 = 0.36602540378f; // (sqrt(3)-1)/2
        const float G2 = 0.21132486540f; // (3-sqrt(3))/6

        const float s = (x + y) * F2;
        const int i = detail::fastFloor(x + s);
        const int j = detail::fastFloor(y + s);

        const float t = (float)(i + j) * G2;
        const float x0 = x - ((float)i - t);
        const float y0 = y - ((float)j - t);

        const int i1 = x0 > y0 ? 1 : 0;
        const int j1 = x0 > y0 ? 0 : 1;

        const float x1 = x0 - (float)i1 + G2;
        const float y1 = y0 - (float)j1 + G2;
        const float x2 = x0 - 1.0f + 2.0f * G2;
        const float y2 = y0 - 1.0f + 2.0f * G2;

        float n = 0.0f;

        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 > 0.0f) {
            t0 *= t0;
            n += t0 * t0 * detail::grad2D(detail::hash2D(seed, i, j), x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 > 0.0f) {
            t1 *= t1;
            n += t1 * t1 * detail::grad2D(detail::hash2D(seed, i + i1, j + j1), x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 > 0.0f) {
            t2 *= t2;
            n += t2 * t2 * detail::grad2D(detail::hash2D(seed, i + 1, j + 1), x2, y2);
        }

        return n * 45.0f;
    }

    // ------------------------------------------------------------------------
    // OPENSIMPLEX2-STYLE 3D -> [-1, 1]
    // ------------------------------------------------------------------------
    // Uso: cuevas y tuneles. Preferido sobre Perlin 3D para tuneles porque
    // no genera las "cajas" alineadas a ejes tipicas de la rejilla cubica.
    static float simplex3D(int seed, float x, float y, float z) {
        const float F3 = 1.0f / 3.0f;
        const float G3 = 1.0f / 6.0f;

        const float s = (x + y + z) * F3;
        const int i = detail::fastFloor(x + s);
        const int j = detail::fastFloor(y + s);
        const int k = detail::fastFloor(z + s);

        const float t = (float)(i + j + k) * G3;
        const float x0 = x - ((float)i - t);
        const float y0 = y - ((float)j - t);
        const float z0 = z - ((float)k - t);

        // Determinar en cual de los 6 simplices del cubo caemos.
        int i1, j1, k1, i2, j2, k2;
        if (x0 >= y0) {
            if (y0 >= z0)      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; }
            else if (x0 >= z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; }
            else               { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; }
        } else {
            if (y0 < z0)       { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; }
            else if (x0 < z0)  { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; }
            else               { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; }
        }

        const float x1 = x0 - (float)i1 + G3;
        const float y1 = y0 - (float)j1 + G3;
        const float z1 = z0 - (float)k1 + G3;
        const float x2 = x0 - (float)i2 + 2.0f * G3;
        const float y2 = y0 - (float)j2 + 2.0f * G3;
        const float z2 = z0 - (float)k2 + 2.0f * G3;
        const float x3 = x0 - 1.0f + 3.0f * G3;
        const float y3 = y0 - 1.0f + 3.0f * G3;
        const float z3 = z0 - 1.0f + 3.0f * G3;

        float n = 0.0f;

        float t0 = 0.6f - x0*x0 - y0*y0 - z0*z0;
        if (t0 > 0.0f) {
            t0 *= t0;
            n += t0 * t0 * detail::grad3D(detail::hash3D(seed, i, j, k), x0, y0, z0);
        }
        float t1 = 0.6f - x1*x1 - y1*y1 - z1*z1;
        if (t1 > 0.0f) {
            t1 *= t1;
            n += t1 * t1 * detail::grad3D(detail::hash3D(seed, i+i1, j+j1, k+k1), x1, y1, z1);
        }
        float t2 = 0.6f - x2*x2 - y2*y2 - z2*z2;
        if (t2 > 0.0f) {
            t2 *= t2;
            n += t2 * t2 * detail::grad3D(detail::hash3D(seed, i+i2, j+j2, k+k2), x2, y2, z2);
        }
        float t3 = 0.6f - x3*x3 - y3*y3 - z3*z3;
        if (t3 > 0.0f) {
            t3 *= t3;
            n += t3 * t3 * detail::grad3D(detail::hash3D(seed, i+1, j+1, k+1), x3, y3, z3);
        }

        return n * 32.0f;
    }

    // ------------------------------------------------------------------------
    // CELULAR / WORLEY 2D
    // ------------------------------------------------------------------------
    // Devuelve F1 (distancia al punto mas cercano) normalizada a ~[0,1].
    // Uso: mascaras de continentes, distribucion de biomas raros.
    static float cellular2D(int seed, float x, float y) {
        const int xr = detail::fastFloor(x + 0.5f);
        const int yr = detail::fastFloor(y + 0.5f);

        float f1 = 1e9f;

        for (int xi = xr - 1; xi <= xr + 1; ++xi) {
            for (int yi = yr - 1; yi <= yr + 1; ++yi) {
                const uint32_t h = detail::hash2D(seed, xi, yi);
                // Jitter del punto dentro de su celda, determinista.
                const float px = (float)xi + detail::hashToUnit(h);
                const float py = (float)yi + detail::hashToUnit(h * 0x9E3779B1u);

                const float dx = x - px;
                const float dy = y - py;
                const float d = dx * dx + dy * dy;

                if (d < f1) f1 = d;
            }
        }
        return sqrtf(f1 < 0.0f ? 0.0f : f1);
    }

    // ------------------------------------------------------------------------
    // CELULAR 3D: devuelve F2 - F1
    // ------------------------------------------------------------------------
    // Uso: CUEVAS TIPO TUNEL. F2-F1 se aproxima a cero exactamente sobre las
    // aristas del diagrama de Voronoi, formando una red conectada de
    // corredores. Es la tecnica estandar para cuevas ramificadas naturales,
    // muy superior a umbralizar Perlin 3D (que da burbujas inconexas).
    static float cellular3D_F2F1(int seed, float x, float y, float z) {
        const int xr = detail::fastFloor(x + 0.5f);
        const int yr = detail::fastFloor(y + 0.5f);
        const int zr = detail::fastFloor(z + 0.5f);

        float f1 = 1e9f;
        float f2 = 1e9f;

        // Recorrido 3x3x3 con PODA POR PLANO.
        //
        // Esta funcion es el codigo mas caliente del generador (se evalua por
        // voxel de roca, ~15.000 veces por chunk). La poda descarta filas y
        // planos completos cuya distancia minima posible ya supera f2: si el
        // plano X=xi esta a distancia dx del punto, ninguna celda de ese
        // plano puede acercarse mas que dx, asi que se salta entero sin
        // calcular sus 9 hashes.
        //
        // Es una optimizacion EXACTA: no cambia el resultado, solo evita
        // trabajo demostrablemente inutil.
        for (int xi = xr - 1; xi <= xr + 1; ++xi) {
            const float dxMin = (float)xi - 0.5f - x;
            const float dxMax = (float)xi + 0.5f - x;
            // Distancia minima al plano (0 si el punto esta dentro de la banda)
            const float dxc = (dxMin > 0.0f) ? dxMin : ((dxMax < 0.0f) ? -dxMax : 0.0f);
            if (dxc * dxc >= f2) continue;

            for (int yi = yr - 1; yi <= yr + 1; ++yi) {
                const float dyMin = (float)yi - 0.5f - y;
                const float dyMax = (float)yi + 0.5f - y;
                const float dyc = (dyMin > 0.0f) ? dyMin : ((dyMax < 0.0f) ? -dyMax : 0.0f);
                if (dxc * dxc + dyc * dyc >= f2) continue;

                for (int zi = zr - 1; zi <= zr + 1; ++zi) {
                    const uint32_t h = detail::hash3D(seed, xi, yi, zi);
                    const float px = (float)xi + detail::hashToUnit(h);
                    const float py = (float)yi + detail::hashToUnit(h * 0x9E3779B1u);
                    const float pz = (float)zi + detail::hashToUnit(h * 0x85EBCA77u);

                    const float dx = x - px;
                    const float dy = y - py;
                    const float dz = z - pz;
                    const float d = dx * dx + dy * dy + dz * dz;

                    if (d < f1)      { f2 = f1; f1 = d; }
                    else if (d < f2) { f2 = d; }
                }
            }
        }

        if (f1 < 0.0f) f1 = 0.0f;
        if (f2 < 0.0f) f2 = 0.0f;
        return sqrtf(f2) - sqrtf(f1);
    }

    // ========================================================================
    // FRACTALES
    // ========================================================================

    // ------------------------------------------------------------------------
    // FBM 2D (Fractal Brownian Motion) -> ~[-1, 1]
    // ------------------------------------------------------------------------
    // Suma de octavas con frecuencia creciente (lacunarity) y amplitud
    // decreciente (gain). Es la base del relieve natural: mezcla detalle a
    // multiples escalas igual que el terreno real.
    // Cada octava usa un seed derivado distinto para que no se correlacionen.
    static float fbm2D(int seed, float x, float y, int octaves,
                       float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum  += perlin2D(seed + i * 1013, x * freq, y * freq) * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    // FBM 2D con simplex (mas organico, sin alineacion a ejes).
    static float fbmSimplex2D(int seed, float x, float y, int octaves,
                              float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum  += simplex2D(seed + i * 1013, x * freq, y * freq) * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    // ------------------------------------------------------------------------
    // FBM 3D -> ~[-1, 1]
    // ------------------------------------------------------------------------
    static float fbm3D(int seed, float x, float y, float z, int octaves,
                       float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum  += perlin3D(seed + i * 1013, x * freq, y * freq, z * freq) * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    static float fbmSimplex3D(int seed, float x, float y, float z, int octaves,
                              float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum  += simplex3D(seed + i * 1013, x * freq, y * freq, z * freq) * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    // ------------------------------------------------------------------------
    // RIDGED MULTIFRACTAL 2D -> [0, 1]
    // ------------------------------------------------------------------------
    // ETAPA 4: es EL algoritmo para cordilleras.
    // Cada octava aplica 1-|n| y se eleva al cuadrado, creando crestas afiladas
    // en lugar de colinas redondeadas. El termino de realimentacion (weight)
    // hace que el detalle fino solo aparezca cerca de las crestas, igual que
    // en montanas reales: laderas suaves abajo, roca rota arriba.
    static float ridged2D(int seed, float x, float y, int octaves,
                          float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;
        float weight = 1.0f;

        for (int i = 0; i < octaves; ++i) {
            float n = perlin2D(seed + i * 1013, x * freq, y * freq);
            n = 1.0f - fabsf(n);
            n *= n;
            n *= weight;

            // Realimentacion: la octava siguiente se atenua donde esta es baja.
            weight = n * 2.0f;
            if (weight > 1.0f) weight = 1.0f;
            if (weight < 0.0f) weight = 0.0f;

            sum  += n * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    // ------------------------------------------------------------------------
    // BILLOW 2D -> [0, 1]
    // ------------------------------------------------------------------------
    // |n| produce formas hinchadas/redondeadas. Uso: DUNAS de desierto y
    // colinas suaves (ETAPA 8).
    static float billow2D(int seed, float x, float y, int octaves,
                          float lacunarity = 2.0f, float gain = 0.5f) {
        float amp = 1.0f;
        float freq = 1.0f;
        float sum = 0.0f;
        float norm = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            sum  += fabsf(perlin2D(seed + i * 1013, x * freq, y * freq)) * amp;
            norm += amp;
            amp  *= gain;
            freq *= lacunarity;
        }
        return norm > 0.0f ? sum / norm : 0.0f;
    }

    // ------------------------------------------------------------------------
    // DOMAIN WARPING (ETAPA 3)
    // ------------------------------------------------------------------------
    // Desplaza el punto de muestreo usando otro campo de ruido antes de
    // evaluar. Esto rompe la regularidad residual del FBM y produce las
    // formas retorcidas y los meandros del terreno real. Sin esto, el relieve
    // se ve "repetitivo" aunque tenga muchas octavas.
    //
    // La salida se escribe en (outX, outY): el llamador decide que muestrear.
    static void warp2D(int seed, float x, float y, float strength,
                       float& outX, float& outY) {
        // Dos campos independientes -> vector de desplazamiento.
        const float wx = fbm2D(seed + 7717, x, y, 3);
        const float wy = fbm2D(seed + 9931, x, y, 3);
        outX = x + wx * strength;
        outY = y + wy * strength;
    }

    // Warp fractal: dos pasadas de warp. Mas caro pero produce estructuras
    // filamentosas muy naturales. Se usa en el mapa de weirdness.
    static void warpFractal2D(int seed, float x, float y, float strength,
                              float& outX, float& outY) {
        float ax, ay;
        warp2D(seed, x, y, strength, ax, ay);
        warp2D(seed + 4441, ax * 2.0f, ay * 2.0f, strength * 0.5f, outX, outY);
    }

    // ========================================================================
    // UTILIDADES
    // ========================================================================

    // Mapea [-1,1] -> [0,1]
    static float toUnit(float v) {
        v = v * 0.5f + 0.5f;
        return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // ------------------------------------------------------------------------
    // EXPANSION DE CONTRASTE (critico para los mapas climaticos)
    // ------------------------------------------------------------------------
    // PROBLEMA: el FBM suma N octavas independientes. Por el teorema del
    // limite central, esa suma tiende a una distribucion gaussiana ESTRECHA
    // centrada en 0.5, no a una uniforme en [0,1]. Medido empiricamente con
    // 4 octavas, el 90% de los valores cae en [0.39, 0.61].
    //
    // CONSECUENCIA SI NO SE CORRIGE: los umbrales de bioma que exigen
    // valores extremos (p.ej. "humedad < 0.32" para desierto) no se cumplen
    // NUNCA, y esos biomas no aparecen jamas en el mundo.
    //
    // SOLUCION: expandir el rango alrededor de 0.5 por un factor, y luego
    // aplicar una curva S suave para redistribuir la masa hacia los extremos
    // sin introducir discontinuidades (lo que crearia bordes duros).
    //
    // strength: 1.0 = sin cambio; 2.5-3.5 = expansion tipica para clima.
    static float expandContrast(float v, float strength) {
        // Centrar, escalar, recentrar.
        float e = (v - 0.5f) * strength + 0.5f;
        e = clamp(e, 0.0f, 1.0f);
        // Curva S suave: mantiene la continuidad (derivada continua) y
        // empuja los valores hacia 0 y 1.
        return e * e * (3.0f - 2.0f * e);
    }

    static float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    static float smoothstep(float edge0, float edge1, float v) {
        if (edge1 <= edge0) return v < edge0 ? 0.0f : 1.0f;
        float t = (v - edge0) / (edge1 - edge0);
        t = clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Valor pseudoaleatorio determinista en [0,1) para una posicion del mundo.
    // Uso: decidir colocacion de arboles, minerales, variaciones locales.
    // Es una funcion pura de (seed, x, z): no consume ni avanza ningun estado,
    // por lo que es seguro llamarla desde cualquier hilo en cualquier orden.
    static float valueAt2D(int seed, int x, int z) {
        return detail::hashToUnit(detail::hash2D(seed, x, z));
    }

    static float valueAt3D(int seed, int x, int y, int z) {
        return detail::hashToUnit(detail::hash3D(seed, x, y, z));
    }

    static uint32_t rawHash2D(int seed, int x, int z) {
        return detail::hash2D(seed, x, z);
    }
};

// ============================================================================
// SPLINE - Interpolacion por puntos de control
// ============================================================================
// ETAPA 3: convierte un valor de ruido en una altura de forma NO LINEAL.
// Es la tecnica que usa Minecraft 1.18+ para que, por ejemplo, la
// continentalidad produzca una plataforma continental plana, luego un talud
// abrupto y luego una meseta interior, en lugar de una rampa recta.
//
// Permite esculpir el perfil del terreno sin tocar el ruido subyacente.
// ============================================================================
class Spline {
public:
    struct Point {
        float t;     // entrada
        float value; // salida
    };

    // Evalua una spline definida por puntos ordenados por t ascendente.
    // Usa smoothstep entre puntos para evitar quiebres de pendiente (que en
    // el terreno se verian como aristas rectas antinaturales).
    static float eval(const Point* pts, int count, float t) {
        if (count <= 0) return 0.0f;
        if (t <= pts[0].t) return pts[0].value;
        if (t >= pts[count - 1].t) return pts[count - 1].value;

        for (int i = 0; i < count - 1; ++i) {
            if (t >= pts[i].t && t <= pts[i + 1].t) {
                const float span = pts[i + 1].t - pts[i].t;
                if (span <= 0.0f) return pts[i].value;
                float u = (t - pts[i].t) / span;
                u = u * u * (3.0f - 2.0f * u); // smoothstep
                return Noise::lerp(pts[i].value, pts[i + 1].value, u);
            }
        }
        return pts[count - 1].value;
    }
};

} // namespace TerrainGen

#endif // NOISE_SYSTEM_H
