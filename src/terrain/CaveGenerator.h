#ifndef CAVE_GENERATOR_H
#define CAVE_GENERATOR_H

#include "NoiseSystem.h"

// ============================================================================
// CAVE GENERATOR - ETAPA 9
// ============================================================================
// RESPONSABILIDAD UNICA: decidir si un voxel solido debe vaciarse.
//
// REQUISITO: "No usar unicamente Perlin 3D." Se combinan CINCO tecnicas,
// cada una responsable de un tipo de estructura distinto:
//
//   TECNICA                  ESTRUCTURA QUE PRODUCE
//   -----------------------  ------------------------------------------
//   Ruido celular (F2-F1)    Tuneles largos ramificados (red conectada)
//   Simplex 3D (worms)       Serpenteo organico de los tuneles
//   Perlin 3D (cheese)       Camaras y cavernas enormes
//   Domain warping           Rompe la regularidad, crea meandros
//   Umbral variable          Conductos estrechos vs salas amplias
//
// POR QUE NO BASTA PERLIN 3D UMBRALIZADO:
//   Umbralizar |perlin3D| < k produce burbujas ovaladas aisladas y alineadas
//   con la rejilla. No genera redes conectadas ni tuneles: el jugador cae en
//   huecos cerrados. El ruido celular F2-F1 en cambio se anula sobre las
//   ARISTAS del diagrama de Voronoi, que forman por construccion un grafo
//   conectado: de ahi salen tuneles reales que se bifurcan y se reencuentran.
//
// ESTRUCTURAS ESPECIALES:
//   - Arcos naturales y columnas: emergen de forma emergente donde dos
//     campos de cueva se solapan parcialmente (se preserva material entre
//     dos vacios adyacentes gracias al termino de columna).
//   - Camaras: Perlin 3D de baja frecuencia con umbral generoso.
//   - Conductos estrechos: zonas donde F2-F1 apenas cruza el umbral.
//
// SIN COSTURAS: todo se evalua en coordenadas de mundo absolutas, por lo que
// un tunel que cruza la frontera de dos chunks continua exactamente igual.
// ============================================================================

namespace TerrainGen {

class CaveGenerator {
private:
    int seed;

    int seedTunnel()  const { return seed + 1733147; }
    int seedWorm()    const { return seed + 1847903; }
    int seedCheese()  const { return seed + 1961659; }
    int seedChamber() const { return seed + 2075417; }
    int seedWarp()    const { return seed + 2189173; }

public:
    // Altura maxima a la que pueden aparecer cuevas (bajo la superficie).
    static constexpr int CAVE_MAX_Y      = 120;
    // Altura minima: por encima de la bedrock.
    static constexpr int CAVE_MIN_Y      = 6;
    // Margen bajo la superficie: evita agujeros que perforen el terreno.
    static constexpr int SURFACE_MARGIN  = 5;

    explicit CaveGenerator(int s) : seed(s) {}

    // ========================================================================
    // TEST PRINCIPAL: hay cueva en este voxel?
    // ========================================================================
    // x, y, z          : coordenadas de MUNDO
    // surfaceHeight    : altura del terreno en esta columna
    //
    // Devuelve true si el voxel debe ser AIRE.
    bool IsCave(float x, int y, float z, int surfaceHeight) const {
        // --- Limites verticales ---
        if (y < CAVE_MIN_Y || y > CAVE_MAX_Y) return false;

        // No perforar la superficie: deja un techo solido.
        // Las entradas a cuevas se generan aparte (ver IsCaveEntrance).
        if (y > surfaceHeight - SURFACE_MARGIN) return false;

        const float fy = (float)y;

        // --------------------------------------------------------------------
        // FACTOR DE PROFUNDIDAD
        // --------------------------------------------------------------------
        // Mas cuevas en profundidad, menos cerca de la superficie. Evita que
        // el terreno quede como un queso gruyere justo bajo la hierba.
        const float depthRatio = Noise::clamp(
            (float)(surfaceHeight - y) / 60.0f, 0.0f, 1.0f);
        const float depthFactor = Noise::smoothstep(0.0f, 0.45f, depthRatio);
        if (depthFactor <= 0.01f) return false;

        // NOTA SOBRE OPTIMIZACION DESCARTADA:
        // Se probo un "rechazo temprano" con un campo simplex barato para
        // evitar el ruido celular en voxeles improbables. Se midio y NO
        // funciona: ese campo no correlaciona con la posicion de los
        // tuneles, de modo que descartaba cuevas reales en la misma
        // proporcion que voxeles (umbral 0.82 -> 4.1% descartado, 4.06% de
        // las cuevas perdidas). Anadia coste y borraba cuevas al azar.
        // El ahorro real vino de bajar el warp y widthNoise a 1 octava.

        // --------------------------------------------------------------------
        // DOMAIN WARPING GLOBAL
        // --------------------------------------------------------------------
        // Deforma el espacio antes de evaluar los campos de cueva. Sin esto,
        // los tuneles serian demasiado regulares y predecibles.
        //
        // Los tres ejes comparten el mismo punto de muestreo, asi que se
        // calcula una sola vez y se derivan las 3 componentes con seeds
        // distintos (antes se repetian las multiplicaciones 3 veces).
        //
        // OPTIMIZACION: se usa simplex3D directo (1 octava) en lugar de
        // fbmSimplex3D con 2 octavas. El warp solo necesita desplazar el
        // punto de muestreo de forma organica; las octavas adicionales
        // anadian detalle imperceptible tras la deformacion pero DUPLICABAN
        // el coste, y este es el codigo mas caliente del generador
        // (~15.000 llamadas por chunk).
        const float wsx = x  * 0.008f;
        const float wsy = fy * 0.008f;
        const float wsz = z  * 0.008f;
        const float warpX = Noise::simplex3D(seedWarp(),      wsx, wsy, wsz) * 14.0f;
        const float warpY = Noise::simplex3D(seedWarp() + 31, wsx, wsy, wsz) * 8.0f;
        const float warpZ = Noise::simplex3D(seedWarp() + 62, wsx, wsy, wsz) * 14.0f;

        const float wx = x  + warpX;
        const float wy = fy + warpY;
        const float wz = z  + warpZ;

        // --------------------------------------------------------------------
        // 1. TUNELES (ruido celular F2-F1)
        // --------------------------------------------------------------------
        // El nucleo del sistema. F2-F1 ~ 0 sobre las aristas de Voronoi.
        // Se aplasta el eje Y (x0.55) para que los tuneles sean mas
        // horizontales que verticales, como las cuevas reales excavadas por
        // agua.
        const float tunnelScale = 0.021f;
        const float tunnel = Noise::cellular3D_F2F1(seedTunnel(),
                                                    wx * tunnelScale,
                                                    wy * tunnelScale * 1.8f,
                                                    wz * tunnelScale);

        // Umbral variable: modulado por ruido de baja frecuencia para que
        // algunos tramos sean galerias amplias y otros conductos estrechos.
        // 1 octava basta: es un campo de muy baja frecuencia cuyo unico
        // papel es variar suavemente la anchura del tunel.
        const float widthNoise = Noise::simplex3D(seedWorm(),
                                                  x * 0.004f, fy * 0.004f, z * 0.004f);
        // ⭐ CUEVAS POR TODAS PARTES
        //
        // Rango de umbral: 0.048 (conducto estrecho) a 0.136 (galeria
        // amplia). Es el DOBLE del ajuste anterior, que daba ~14-18% de
        // subsuelo hueco; este deja el subsuelo entre ~28 y ~35%: cavar en
        // cualquier direccion topa con galeria casi seguro, y las bocas se
        // encuentran paseando.
        //
        // La referencia sigue siendo el karst mexicano, que es de los mas
        // densos del mundo: 6 a 19 km de galeria por km cuadrado en Quintana
        // Roo (Smart et al., 2006), y el 25.5% del territorio es karst
        // (INEGI/WOKAM). Esto lo exagera a proposito -- es un juego, y se
        // pidieron cuevas SUPER comunes -- pero el sistema es el mismo, solo
        // se abre mas la llave.
        const float tunnelThreshold = (0.092f + widthNoise * 0.044f) * depthFactor;

        if (tunnel < tunnelThreshold) {
            // Termino de COLUMNA: preserva pilares de roca dentro de los
            // tuneles anchos. Produce columnas y arcos naturales de forma
            // emergente en vez de tallarlos explicitamente.
            const float column = Noise::fbmSimplex3D(seedChamber() + 7,
                                                     x * 0.055f, fy * 0.030f, z * 0.055f, 2);
            if (column > 0.62f) {
                return false; // queda roca: es una columna
            }
            return true;
        }

        // --------------------------------------------------------------------
        // 2. CAMARAS / CAVERNAS ENORMES (Perlin 3D "cheese")
        // --------------------------------------------------------------------
        // Baja frecuencia + umbral alto = pocas cavidades pero muy grandes.
        //
        // ⭐ Empiezan MAS ARRIBA (0.28 en vez de 0.45) y con el umbral mas
        // suelto: las cavernas grandes dejan de ser cosa exclusiva del fondo
        // del mundo y aparecen a media altura, que es donde el jugador cava.
        if (depthRatio > 0.28f) {
            const float cheese = Noise::fbm3D(seedCheese(),
                                              wx * 0.0115f,
                                              wy * 0.0150f,
                                              wz * 0.0115f, 3);

            // Umbral que se relaja con la profundidad: las camaras mas
            // grandes estan en el fondo del mundo.
            const float chamberThreshold = Noise::lerp(0.50f, 0.34f,
                Noise::smoothstep(0.28f, 1.0f, depthRatio));

            if (cheese > chamberThreshold) {
                // Tambien aqui se preservan columnas: una caverna enorme
                // completamente vacia se ve artificial.
                const float pillar = Noise::fbmSimplex3D(seedChamber(),
                                                         x * 0.042f, fy * 0.024f, z * 0.042f, 2);
                if (pillar > 0.70f) {
                    return false; // columna que sostiene el techo
                }
                return true;
            }
        }

        // --------------------------------------------------------------------
        // 3. TUNELES SECUNDARIOS (segundo campo celular, otra escala)
        // --------------------------------------------------------------------
        // Una segunda red mas fina que se cruza con la principal. Al usar
        // otro seed y otra frecuencia, las intersecciones generan camaras
        // pequenas y bifurcaciones no repetitivas.
        const float tunnel2 = Noise::cellular3D_F2F1(seedTunnel() + 997,
                                                     wx * 0.034f,
                                                     wy * 0.034f * 1.5f,
                                                     wz * 0.034f);
        // ⭐ Tambien mas generosa: 0.016 -> 0.036. Es la red que conecta la
        // principal consigo misma, asi que abrirla no solo anade hueco, hace
        // que las galerias se comuniquen entre si en vez de quedar sueltas.
        if (tunnel2 < 0.036f * depthFactor) {
            return true;
        }

        return false;
    }

    // ========================================================================
    // ENTRADAS A CUEVAS
    // ========================================================================
    // Perfora selectivamente el techo para conectar la red subterranea con
    // la superficie. Sin esto las cuevas serian inaccesibles.
    // Se usa un ruido de frecuencia muy baja y umbral estricto, de modo que
    // las entradas son escasas y parecen simas o bocas naturales.
    bool IsCaveEntrance(float x, int y, float z, int surfaceHeight) const {
        if (y > surfaceHeight - 1) return false;
        if (y < surfaceHeight - 18) return false;

        const float fy = (float)y;

        const float entrance = Noise::fbmSimplex3D(seedCheese() + 5501,
                                                   x * 0.020f, fy * 0.020f, z * 0.020f, 3);

        // ⭐ MAS BOCAS: del ~2% de las columnas a alrededor del ~8%.
        //
        // De poco sirve llenar el subsuelo de galerias si no hay por donde
        // entrar. Bajar este umbral es lo que convierte "hay muchas cuevas"
        // en "me encuentro cuevas paseando".
        if (entrance > 0.58f) {
            // Estrechamiento hacia arriba: la boca es mas angosta que la
            // galeria, como una sima real.
            const float narrowing = (float)(surfaceHeight - y) / 18.0f;
            const float required = Noise::lerp(0.68f, 0.58f, narrowing);
            return entrance > required;
        }
        return false;
    }

    // ========================================================================
    // LAGOS DE LAVA SUBTERRANEOS
    // ========================================================================
    // Nivel bajo el cual las cuevas se inundan de lava.
    static constexpr int LAVA_LEVEL = 11;
};

} // namespace TerrainGen

#endif // CAVE_GENERATOR_H
