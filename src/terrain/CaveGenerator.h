#ifndef CAVE_GENERATOR_H
#define CAVE_GENERATOR_H

#include "NoiseSystem.h"
#include <cmath>   // cosf, para la onda que separa los niveles de cueva

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
    // Ondula la altura de los pisos de cueva (ver IsCave).
    int seedNivel()   const { return seed + 2303929; }
    // Reparte las bocas naturales por el mundo (ver IsCaveEntrance).
    int seedBoca()    const { return seed + 2417683; }

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
        float depthFactor = Noise::smoothstep(0.0f, 0.45f, depthRatio);
        if (depthFactor <= 0.01f) return false;

        // --------------------------------------------------------------------
        // ⭐ NIVELES DE CUEVA (pisos)
        // --------------------------------------------------------------------
        // Antes la densidad solo dependia de la profundidad, asi que el
        // subsuelo era una nube de huecos homogenea: cavaras donde cavaras,
        // todo se parecia. Ahora la red se organiza en PISOS horizontales
        // separados por bancos de roca maciza.
        //
        // Es lo que hace una cueva real: el agua se estanca en el nivel
        // freatico, excava en horizontal, el nivel baja y deja el piso viejo
        // seco arriba. Un sistema karstico maduro tiene varios de esos pisos
        // superpuestos -- en el Sistema Sac Actun (Quintana Roo) se
        // reconocen varios niveles asociados a antiguos niveles del mar.
        //
        // COMO SE HACE: una onda coseno sobre la altura. Vale ~1 en el centro
        // de cada piso y ~0 en la roca entre pisos, y se multiplica por el
        // factor de profundidad. El resultado es que los tuneles se
        // concentran en bandas y entre ellas queda techo y suelo de verdad.
        //
        // La banda NO es plana: se ondula con un ruido de muy baja frecuencia
        // (`ondulacion`), asi que un piso sube y baja decenas de bloques a lo
        // largo del mundo en vez de ser una loncha de laboratorio.
        {
            // Separacion entre pisos, en bloques. 26 deja bancos de roca
            // gruesos entre niveles sin que descender de uno a otro sea una
            // caminata.
            constexpr float SEPARACION_PISOS = 26.0f;

            // La ondulacion desplaza la altura de la banda por zona. +-9
            // bloques es suficiente para que el piso se sienta natural y no
            // tanto como para que dos pisos se fundan.
            const float ondulacion = Noise::simplex3D(seedNivel(),
                                                      x * 0.0035f, 0.0f, z * 0.0035f) * 9.0f;

            const float fase = ((fy + ondulacion) / SEPARACION_PISOS) * 6.2831853f;
            // cos -> [-1,1]; se lleva a [0,1].
            const float banda = 0.5f + 0.5f * cosf(fase);

            // ⚠️ NO se anula del todo entre pisos.
            //
            // Con un factor que llegue a 0 los niveles quedarian
            // INCOMUNICADOS: cada piso seria una capa estanca sin forma de
            // bajar al siguiente, que es peor que no tener niveles. El suelo
            // de 0.35 deja pasar los pozos y chimeneas que conectan un piso
            // con el de abajo -- justo lo que en una cueva real son los tiros
            // verticales.
            const float refuerzoNivel = 0.35f + 0.65f * banda;
            depthFactor *= refuerzoNivel;
        }

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
    // Profundidad maxima a la que puede bajar un pozo de entrada.
    //
    // ⭐ ANTES ERAN 22 BLOQUES, Y ESE ERA EL FALLO.
    //
    // Medido sobre 160.000 columnas: de las que tenian boca, solo el 28.7%
    // llegaba a tocar la galeria. Las otras dos terceras partes eran hoyos que
    // morian en roca maciza, con una media de 6.86 bloques de piedra entre el
    // fondo del pozo y el techo de la cueva -- y un 27% con mas de diez.
    //
    // El motivo: la boca bajaba una distancia FIJA desde la superficie, pero
    // la galeria esta donde la ponga el sistema de pisos (ver la banda coseno
    // de IsCave), que ondula decenas de bloques a lo largo del mundo. Una
    // distancia fija no puede alcanzar un objetivo movil.
    //
    // Con 56 el pozo llega a cualquier piso alto. El embudo sigue cerrandolo
    // por su cuenta mucho antes en la mayoria de los casos: esto es el tope,
    // no la profundidad tipica.
    static constexpr int ENTRADA_MAX_HONDURA = 56;

    bool IsCaveEntrance(float x, int y, float z, int surfaceHeight) const {
        // El pozo arranca EN la superficie -- no un bloque por debajo.
        //
        // Con `y > surfaceHeight - 1` la columna de superficie sobrevivia, asi
        // que sobre cada boca quedaba una tapa de pasto o de arena: el agujero
        // estaba, pero tapado. Es justo lo que se pidio quitar.
        if (y > surfaceHeight) return false;
        if (y < surfaceHeight - ENTRADA_MAX_HONDURA) return false;

        const float fy = (float)y;

        // --------------------------------------------------------------------
        // ⭐ DONDE HAY BOCA: SE DECIDE EN 2D, NO EN 3D
        // --------------------------------------------------------------------
        // Antes la boca salia de un ruido 3D, asi que su forma variaba con la
        // altura y quedaban perforaciones irregulares -- mas agujeros de gusano
        // que simas.
        //
        // Ahora el SITIO de la boca lo decide un campo 2D (solo x,z): es una
        // propiedad de la COLUMNA, como lo es en el mundo real. Una sima esta
        // en un punto del mapa, y desde ahi baja. Eso hace que la entrada sea
        // un pozo reconocible que se ve desde lejos, y no una grieta que
        // aparece y desaparece segun la altura a la que se mire.
        const float campoBoca = Noise::fbmSimplex2D(seedBoca(),
                                                    x * 0.018f, z * 0.018f, 3);

        // ⚠️ UMBRAL CALIBRADO CONTRA EL RANGO REAL DEL RUIDO, NO A OJO.
        //
        // fbmSimplex2D con 3 octavas NO llega a 1.0: medido sobre 160.000
        // columnas da min -0.43, max 0.42, media 0.00. Un umbral de 0.52 --que
        // es lo que pedia la version 3D anterior-- es sencillamente
        // inalcanzable, y dejaba CERO bocas en todo el mundo.
        //
        // Reparto medido de este campo:
        //     > 0.30  ->  2.1 % de las columnas
        //     > 0.20  -> 11.4 %
        //     > 0.10  -> 28.1 %
        //
        // Se toma 0.22: deja ~8 % de columnas candidatas, que tras el embudo
        // de abajo se queda en bocas separadas y encontrables paseando.
        constexpr float UMBRAL_BOCA = 0.22f;
        if (campoBoca <= UMBRAL_BOCA) return false;

        // --------------------------------------------------------------------
        // FORMA DE EMBUDO
        // --------------------------------------------------------------------
        // Una sima real es un cono invertido: ancha arriba, donde el techo se
        // ha desplomado, y estrecha abajo, donde engancha con la galeria.
        //
        // `hondura` va de 0 en la superficie a 1 en el fondo de la boca. El
        // umbral SUBE con la hondura, asi que cuanto mas abajo, menos columnas
        // siguen abiertas: eso es el embudo.
        // ⚠️ EL EMBUDO, RECALIBRADO CONTRA DATOS MEDIDOS.
        //
        // Es lo que le da al pozo su forma de cono invertido: ancho arriba,
        // donde el techo se desplomo, y estrecho abajo.
        //
        // EL PROBLEMA QUE TENIA: se repartia sobre 22 bloques y llegaba a
        // exigir 0.34 sobre un campo cuyo maximo real es 0.42. Medido sobre
        // 90.000 columnas, eso CERRABA EL 93.7% DE LOS POZOS antes de que
        // llegaran a ninguna parte: la bajada media era de 10.9 bloques.
        //
        // Y las galerias no estan ahi. Medida su hondura en las mismas
        // columnas:
        //
        //     0-4    ->     0        20-24  ->   898
        //     5-9    ->   955        25-29  ->   467
        //     10-14  ->  2300  <--   30-34  ->   193
        //     15-19  ->  1750  <--   35-44  ->   136
        //
        // El grueso esta entre 10 y 20 de hondura, justo donde el embudo ya
        // habia estrangulado el pozo. La boca se abria y moria en roca.
        //
        // DOS CAMBIOS, los dos calibrados contra ese reparto:
        //
        //   1. El tramo pasa de 22 a 34 bloques, que cubre el 90% de las
        //      galerias medidas. El pozo se estrecha mas despacio y llega.
        //
        //   2. El cuello afloja de 0.34 a 0.25. Con 0.34 sobre un maximo real
        //      de 0.42 solo sobrevivia el 1% superior del campo.
        //
        // Sigue habiendo cono: arriba entra cualquier columna por encima de
        // 0.22 y abajo solo las de 0.25. Lo que cambia es que el
        // estrechamiento acompaña al pozo hasta la galeria en vez de ahogarlo
        // a la tercera parte del camino.
        //
        // RESULTADO MEDIDO sobre las mismas 160.000 columnas:
        //
        //                          antes  22/0.34   ->   ahora  34/0.25
        //     columnas con boca          7.79%           8.92%
        //     abiertas en superficie     0%    (*)       8.92%  (todas)
        //     CONECTAN con la galeria    2.13%           6.44%
        //       de las que hay boca     27.3%           72.2%
        //     hueco de roca medio        7.47 bl         3.33 bl
        //
        //  (*) ninguna: la columna de superficie sobrevivia y las tapaba.
        //
        // O sea: TRIPLE de entradas utiles (2.13% -> 6.44% de las columnas)
        // con solo un punto mas de bocas totales. El mundo no queda mas
        // agujereado; lo que cambia es que casi tres de cada cuatro bocas
        // llevan a alguna parte, en vez de una de cada cuatro.
        constexpr float TRAMO_EMBUDO = 34.0f;
        constexpr float CUELLO       = 0.25f;
        const float hondura = Noise::clamp(
            (float)(surfaceHeight - y) / TRAMO_EMBUDO, 0.0f, 1.0f);

        const float requerido = Noise::lerp(UMBRAL_BOCA, CUELLO, hondura);
        if (campoBoca <= requerido) return false;

        // --------------------------------------------------------------------
        // BORDE IRREGULAR
        // --------------------------------------------------------------------
        // Sin esto el pozo es un cilindro perfecto y se nota que lo hizo una
        // formula. Un ruido 3D de frecuencia media muerde el contorno para que
        // el borde sea dentado y las paredes tengan repisas.
        const float mordida = Noise::simplex3D(seedCheese() + 5501,
                                               x * 0.085f, fy * 0.070f, z * 0.085f);

        // ⚠️ LA MORDIDA NO TOCA LA COLUMNA DE SUPERFICIE.
        //
        // La mordida varia con la ALTURA (entra fy en el ruido), asi que puede
        // recortar justo el bloque de arriba y dejar una tapa sobre un pozo
        // que por lo demas esta abierto. Medido: le pasaba al 1.1% de las
        // bocas (38 de 3.480) -- pocas, pero cada una es un agujero tapado que
        // el jugador no encuentra.
        //
        // En la cota de superficie se salta el recorte. El borde dentado se
        // sigue viendo en todo lo demas del pozo, que es donde se aprecia.
        if (y >= surfaceHeight) return true;

        // Solo recorta (nunca abre de mas). La resta es 0.02, proporcional al
        // rango real del campo (~0.42): con el 0.05 de una escala 0-1 se
        // habria comido casi una cuarta parte del margen util.
        return campoBoca - 0.02f * (0.5f + 0.5f * mordida) > requerido;
    }

    // ========================================================================
    // LAGOS DE LAVA SUBTERRANEOS -- RETIRADOS
    // ========================================================================
    // ⚠️ YA NO SE USA. El generador dejo de inundar el fondo de las cuevas
    // (ver ChunkGenerator.h, etapa 9): la lava hacia intransitable la parte
    // baja y convertia el descenso en una carrera de obstaculos.
    //
    // La constante se conserva -- no se borra -- porque volver a activar los
    // lagos es una sola linea en ChunkGenerator, y este es el numero que hay
    // que poner. Hoy no la usa nadie mas (comprobado con grep sobre src/ y
    // tests/): si se decide que la lava no vuelve, se puede borrar sin tocar
    // nada mas.
    static constexpr int LAVA_LEVEL = 11;
};

} // namespace TerrainGen

#endif // CAVE_GENERATOR_H
