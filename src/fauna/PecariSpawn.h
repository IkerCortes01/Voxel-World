#ifndef PECARI_SPAWN_H
#define PECARI_SPAWN_H

#include <cstdint>
#include <cmath>
#include "../terrain/NoiseSystem.h"
#include "../terrain/BiomeTypes.h"

// ============================================================================
// GENERACION DE PECARI DE COLLAR (Dicotyles tajacu)
// ============================================================================
// RESPONSABILIDAD UNICA: decidir DONDE aparece una manada de pecaries y de
// cuantos individuos consta. NO dibuja, NO simula comportamiento, NO mueve
// nada. Igual que DecorationSystem responde "aqui hay un arbol", esto
// responde "aqui hay una manada, y trae 8 miembros".
//
// ----------------------------------------------------------------------------
// LA UNIDAD DE SPAWN ES LA MANADA, NO EL INDIVIDUO
// ----------------------------------------------------------------------------
// Es la decision estructural de este archivo. 01_ARQUITECTURA_DEL_SISTEMA.json
// lo exige explicitamente en sus reglas de aparicion: "Aparecer como grupo si
// la especie es social, no como individuos sueltos".
//
// Y los datos lo confirman por dos vias independientes que CONCUERDAN:
//   - Densidad medida en Quintana Roo: 1.9 +/- 0.8 ind/km2
//     Y                                0.2 +/- 0.1 manadas/km2
//   - 1.9 / 0.2 = 9.5 individuos por manada, que cae dentro del rango
//     publicado de 6-10 para selva.
//
// Dos numeros medidos por separado que al dividirse dan un tercero tambien
// publicado: eso es una comprobacion de coherencia superada, no una
// coincidencia. Por eso el Poisson se aplica a MANADAS y el tamano del grupo
// se muestrea despues.
//
// ----------------------------------------------------------------------------
// POR QUE POISSON POR REJILLA Y NO random() < densidad
// ----------------------------------------------------------------------------
// Exactamente por lo mismo que los arboles: `random() < densidad` produce
// grumos y huecos. Aqui seria peor todavia, porque dos manadas pegadas
// romperian la estructura social (son grupos estables y territoriales, con
// solapamiento de ambitos del 20-40%, no una masa continua).
//
// Se reutiliza el patron ya probado de DecorationSystem::HasTree: rejilla con
// jitter determinista por hash. Funcion pura de (seed, x, z), sin estado, sin
// depender del orden de generacion de chunks, segura desde cualquier hilo.
//
// ----------------------------------------------------------------------------
// TRAZABILIDAD
// ----------------------------------------------------------------------------
// Cada constante numerica de este archivo lleva su origen y su nivel de
// confianza, con la misma escala que usa todo el AI simulator:
//   MEDIDO   - valor publicado, con la cita
//   DERIVADO - calculado a partir de MEDIDOs con formula explicita
//   INFERIDO - de especie hermana o del grupo, con la analogia declarada
//   ESTIMADO - ajuste de diseno sin base en literatura. Se marca sin disimulo.
//
// Fuentes en AI simulator/Mamiferos/10_PECARI_BIOLOGIA.json y
//              AI simulator/Mamiferos/14_PECARI_INVESTIGACION_2026.json
// ============================================================================

namespace Fauna {

using TerrainGen::Noise;
using TerrainGen::BiomeType;
using TerrainGen::BIOME_OCEAN_DEEP;
using TerrainGen::BIOME_OCEAN;
using TerrainGen::BIOME_BEACH;
using TerrainGen::BIOME_PLAINS;
using TerrainGen::BIOME_FOREST;
using TerrainGen::BIOME_DESERT;
using TerrainGen::BIOME_MOUNTAINS;
using TerrainGen::BIOME_MOUNTAIN_PEAKS;
using TerrainGen::BIOME_OCOTAL_BLANCO;
using TerrainGen::BIOME_OCOTAL_CHINO;
using TerrainGen::BIOME_OCOTAL_MIXTO;

// ----------------------------------------------------------------------------
// DATOS DE LA ESPECIE
// ----------------------------------------------------------------------------
namespace PecariDatos {

    // --- Tamano de manada ---
    // MEDIDO: 6-10 en selva, 5-15 en semiarido (10_PECARI_BIOLOGIA).
    // Se toma la union de ambos rangos, y el bioma decide dentro de el.
    constexpr int MANADA_MIN = 5;
    constexpr int MANADA_MAX = 15;

    // DERIVADO: 1.9 ind/km2 / 0.2 manadas/km2 = 9.5.
    // Coincide con el rango de selva publicado (6-10), lo que valida ambos.
    constexpr float MANADA_MEDIA_SELVA = 9.5f;

    // --- Altitud ---
    // MEDIDO: limite tipico ~1800 m; maximo documentado 2335 m en pinon-enebro
    // (Reserva Zuni, Nuevo Mexico). Por encima de eso no aparece.
    constexpr float ALTITUD_MAX_TIPICA_M = 1800.0f;
    constexpr float ALTITUD_MAX_ABSOLUTA_M = 2335.0f;

    // --- Densidad objetivo ---
    // MEDIDO en Quintana Roo, Mexico: 0.2 +/- 0.1 manadas/km2.
    // ADVERTENCIA DE LA INVESTIGACION: en Costa Rica se midieron 19.1-65.9
    // ind/km2, hasta 35x el dato mexicano. Se usa el MEXICANO a proposito:
    // el juego esta ambientado en Mexico, y el valor costarricense llenaria
    // el mundo de pecaries.
    constexpr float MANADAS_POR_KM2 = 0.2f;

} // namespace PecariDatos

// ----------------------------------------------------------------------------
// RESULTADO DE UNA CONSULTA DE SPAWN
// ----------------------------------------------------------------------------
struct ManadaSpawn {
    bool  existe = false;   // false: aqui no nace ninguna manada
    int   miembros = 0;     // cuantos individuos trae
    int   centroX = 0;      // columna del centro de la manada
    int   centroZ = 0;
    float dispersionBloques = 0.0f;  // radio en el que se reparten los miembros
};

// ============================================================================
// SISTEMA DE APARICION
// ============================================================================
class PecariSpawn {
private:
    int seed;

    // Semillas derivadas, con el mismo patron de numeros primos grandes que
    // usa DecorationSystem para que los distintos sistemas no correlacionen.
    int seedManada()  const { return seed + 3011749; }
    int seedTamano()  const { return seed + 3125507; }
    int seedBioma()   const { return seed + 3239263; }

public:
    // ------------------------------------------------------------------------
    // TAMANO DE CELDA DEL POISSON
    // ------------------------------------------------------------------------
    // DERIVADO de la densidad medida, no elegido a ojo.
    //
    // El calculo, explicito para que se pueda auditar:
    //   - Lado de bloque del motor: 0.60 m (Fisica::LADO_M)
    //   - 1 km2 = 1000 m x 1000 m = 1666.67 x 1666.67 bloques
    //   - Densidad objetivo: 0.2 manadas/km2
    //   - Area por manada = 1 km2 / 0.2 = 5 km2
    //   - Lado de esa area = sqrt(5) km = 2.236 km = 3726 bloques
    //
    // Una celda de 3726 bloques es INMANEJABLE: el jugador cruzaria mundos
    // enteros sin ver un pecari, y el ambito hogareno real de una manada es de
    // 150-250 ha (unos 1600-2000 bloques de lado), mucho menor que esa celda.
    //
    // DECISION DE DISENO DECLARADA: se COMPRIME la escala espacial.
    // Se usa 96 bloques (~57.6 m de lado real).
    //
    // Esto NO es densidad real y no se disimula: es densidad de JUEGO. El
    // motivo es el mismo que 01_ARQUITECTURA reconoce al hablar de Ultima
    // Online: un ecosistema que el jugador nunca ve tiene valor cero. Una
    // manada cada 3.7 km es invisible en la practica.
    //
    // ELECCION DEL AUTOR DEL JUEGO: se pidieron pecaries ABUNDANTES, y este
    // valor lo cumple sin romper la lectura visual de manada. Ver el limite
    // duro documentado abajo.
    //
    // El factor de compresion queda explicito para que cualquiera pueda
    // recuperar la densidad real si algun dia se quiere simulacion fiel.
    //
    // confianza: ESTIMADO (el valor 96) / MEDIDO (la densidad de la que sale)
    static constexpr int MANADA_CELL = 96;

    // ------------------------------------------------------------------------
    // EL LIMITE POR DEBAJO DEL CUAL ESTO SE ROMPE
    // ------------------------------------------------------------------------
    // No es una opinion: es geometria, y conviene dejarla escrita para que
    // nadie baje MANADA_CELL sin saber lo que pasa.
    //
    // La separacion minima GARANTIZADA entre dos manadas vecinas es
    // 2 * JITTER_MARGIN = MANADA_CELL / 2 bloques.
    // El radio de dispersion de una manada es DISPERSION_BLOQUES = 8.
    //
    // Para que dos manadas se lean como grupos DISTINTOS y no como una masa
    // continua, la separacion minima debe superar con holgura el diametro de
    // un grupo (16 bloques):
    //
    //   MANADA_CELL / 2  >  2 * DISPERSION_BLOQUES
    //   MANADA_CELL      >  4 * 8 = 32 bloques
    //
    // Con margen de seguridad, el suelo practico es ~48. Por debajo de 32 las
    // manadas se solapan fisicamente y el concepto de manada deja de existir:
    // se convierte en una nube uniforme de pecaries con el mismo nombre.
    //
    // A 96 hay holgura de 3x sobre ese limite.
    static constexpr int MANADA_CELL_MINIMO_SENSATO = 48;
    static_assert(MANADA_CELL >= MANADA_CELL_MINIMO_SENSATO,
                  "MANADA_CELL por debajo del limite: las manadas se solaparian "
                  "y dejarian de leerse como grupos separados");

    // Factor de compresion respecto a la densidad medida en Quintana Roo.
    // 3726 / 96 = 38.8x mas denso que la realidad, en lineal;
    // en area, 38.8^2 = ~1507x.
    // Se declara para que el numero no quede escondido.
    static constexpr float COMPRESION_LINEAL = 38.8f;

    explicit PecariSpawn(int s) : seed(s) {}

    // ------------------------------------------------------------------------
    // IDONEIDAD DEL BIOMA
    // ------------------------------------------------------------------------
    // Devuelve 0.0 (no aparece) a 1.0 (habitat optimo).
    //
    // El pecari de collar es una especie ADAPTABLE: la literatura la registra
    // en selva tropical, bosque, matorral xerico, desierto, sabana y pastizal.
    // Eso es un dato, no una licencia para ponerlo en todas partes: no vive en
    // el oceano ni en la nieve.
    static float IdoneidadBioma(BiomeType biome) {
        switch (biome) {
            // --- OPTIMOS ---
            // MEDIDO: la densidad mexicana de referencia (1.9 ind/km2,
            // Quintana Roo) se midio en selva tropical. Y la dieta de
            // Calakmul (57.9% frutos, 37 especies vegetales) es de selva.
            case BIOME_FOREST:    return 1.00f;

            // Los tres OCOTALES cuentan como bosque. El pecari es generalista
            // y esta registrado en bosque de pino-encino mexicano, que es
            // exactamente este bioma.
            //
            // Se les da el mismo 1.00 que al bosque y no un valor menor: no hay
            // dato publicado de densidad de D. tajacu en pinar puro, y bajarlo
            // "porque un pinar da menos fruto" seria inventar biologia -- justo
            // lo que el protocolo de este sistema prohibe. Si algun dia
            // aparece el dato, este es el sitio donde cambiarlo.
            case BIOME_OCOTAL_BLANCO:
            case BIOME_OCOTAL_CHINO:
            case BIOME_OCOTAL_MIXTO:  return 1.00f;

            // MEDIDO: en zona semiarida el nopal es el 25-80% de la dieta y
            // le resuelve la sed (contiene ~87% de agua). El desierto no es
            // habitat marginal para esta especie: es donde se le llama
            // "javelina" y donde mejor se ha estudiado (Arizona, Texas).
            case BIOME_DESERT:    return 0.95f;

            // --- BUENOS ---
            // MEDIDO: "pastizales y sabanas tropicales y subtropicales" estan
            // entre los habitats registrados.
            case BIOME_PLAINS:    return 0.70f;

            // --- MARGINALES ---
            // MEDIDO: "canones rocosos donde cavernas y oquedades ofrecen
            // proteccion" es habitat descrito, y hay registros hasta 2335 m.
            // Pero la altitud lo limita, asi que la idoneidad es baja.
            case BIOME_MOUNTAINS: return 0.30f;

            // --- EXCLUIDOS ---
            // La playa no es habitat: es transito. Y por encima de la cota de
            // nieve no hay registros.
            case BIOME_BEACH:         return 0.0f;
            case BIOME_MOUNTAIN_PEAKS:return 0.0f;
            case BIOME_OCEAN:         return 0.0f;
            case BIOME_OCEAN_DEEP:    return 0.0f;
        }
        return 0.0f;
    }

    // ------------------------------------------------------------------------
    // TAMANO DE MANADA SEGUN BIOMA
    // ------------------------------------------------------------------------
    // MEDIDO: 6-10 en selva, 5-15 en semiarido.
    //
    // La diferencia no es ruido: en zona semiarida los recursos estan mas
    // agregados (manchones de nopal), lo que permite y exige grupos mas
    // variables. Se respeta el rango publicado de cada bioma en vez de usar
    // uno solo para todos.
    static void RangoManada(BiomeType biome, int& minOut, int& maxOut) {
        switch (biome) {
            case BIOME_DESERT:
                // MEDIDO: 5-15 en semiarido
                minOut = 5; maxOut = 15;
                break;
            case BIOME_FOREST:
            // Los ocotales usan el rango de bosque por el mismo motivo que
            // comparten idoneidad: es bosque, y no hay dato propio de pinar.
            case BIOME_OCOTAL_BLANCO:
            case BIOME_OCOTAL_CHINO:
            case BIOME_OCOTAL_MIXTO:
                // MEDIDO: 6-10 en selva
                minOut = 6; maxOut = 10;
                break;
            case BIOME_PLAINS:
                // INFERIDO: intermedio entre los dos rangos medidos.
                // No hay dato de manada en pastizal mexicano.
                minOut = 5; maxOut = 12;
                break;
            case BIOME_MOUNTAINS:
                // INFERIDO: habitat marginal, grupos pequenos.
                // ESTIMADO en su magnitud concreta.
                minOut = 4; maxOut = 8;
                break;
            default:
                minOut = 0; maxOut = 0;
                break;
        }
    }

    // ------------------------------------------------------------------------
    // CONSULTA PRINCIPAL
    // ------------------------------------------------------------------------
    // Devuelve si en la columna (worldX, worldZ) nace el centro de una manada.
    //
    // Es una funcion PURA, igual que HasTree: mismo (seed, x, z) da siempre el
    // mismo resultado, independientemente del chunk que pregunte, del orden de
    // generacion y del hilo. Es lo que permite que dos chunks vecinos no
    // dupliquen ni corten una manada en la frontera.
    //
    // alturaTerreno: altura del suelo en bloques, para el filtro de altitud.
    // pendiente:     0 = llano, 1 = pared vertical.
    ManadaSpawn ConsultarManada(int worldX, int worldZ,
                                BiomeType biome,
                                float alturaTerreno,
                                float pendiente) const {
        ManadaSpawn r;

        // --- 1. Filtro de bioma ---
        const float idoneidad = IdoneidadBioma(biome);
        if (idoneidad <= 0.0f) return r;

        // --- 2. Filtro de pendiente ---
        // Un ungulado de patas cortas no vive en una pared. El umbral es mas
        // estricto que el de los arboles (0.85) porque un animal tiene que
        // poder DESPLAZARSE por el terreno, no solo estar plantado en el.
        // confianza: ESTIMADO
        constexpr float PENDIENTE_MAX = 0.55f;
        if (pendiente > PENDIENTE_MAX) return r;

        // --- 3. Filtro de altitud ---
        // MEDIDO: limite tipico 1800 m, maximo absoluto 2335 m.
        // Entre ambos, la probabilidad decae en vez de cortarse de golpe:
        // un limite duro produce una linea recta visible en el mundo.
        const float alturaM = alturaTerreno * 0.60f;  // Fisica::LADO_M
        if (alturaM > PecariDatos::ALTITUD_MAX_ABSOLUTA_M) return r;

        float factorAltitud = 1.0f;
        if (alturaM > PecariDatos::ALTITUD_MAX_TIPICA_M) {
            const float t = (alturaM - PecariDatos::ALTITUD_MAX_TIPICA_M) /
                            (PecariDatos::ALTITUD_MAX_ABSOLUTA_M -
                             PecariDatos::ALTITUD_MAX_TIPICA_M);
            factorAltitud = 1.0f - t;   // decae linealmente a 0
        }

        // --- 4. Celda de Poisson ---
        // Division con floor correcto para coordenadas negativas: el mismo
        // cuidado que DecorationSystem, porque `-1 / 192` en C++ da 0, no -1,
        // y eso duplicaria la celda 0 al cruzar el origen.
        const int cellX = (worldX >= 0) ? (worldX / MANADA_CELL)
                                        : ((worldX - MANADA_CELL + 1) / MANADA_CELL);
        const int cellZ = (worldZ >= 0) ? (worldZ / MANADA_CELL)
                                        : ((worldZ - MANADA_CELL + 1) / MANADA_CELL);

        // --- 5. Punto candidato con jitter determinista ---
        // Margen amplio: dos manadas vecinas no deben quedar pegadas en la
        // frontera de sus celdas. El solapamiento de ambitos hogarenos medido
        // es del 20-40%, no del 100%.
        constexpr int JITTER_MARGIN = MANADA_CELL / 4;
        constexpr int JITTER_RANGE  = MANADA_CELL - 2 * JITTER_MARGIN;
        static_assert(JITTER_RANGE >= 1, "MANADA_CELL demasiado pequeno para el margen");

        // Dos hashes INDEPENDIENTES para X y Z, por la misma razon que en los
        // arboles: derivar uno del otro los correlaciona y sesga la
        // distribucion.
        const uint32_t hX = Noise::rawHash2D(seedManada(),      cellX, cellZ);
        const uint32_t hZ = Noise::rawHash2D(seedManada() + 31, cellX, cellZ);
        const int offX = JITTER_MARGIN + (int)(hX % (uint32_t)JITTER_RANGE);
        const int offZ = JITTER_MARGIN + (int)(hZ % (uint32_t)JITTER_RANGE);

        const int manadaX = cellX * MANADA_CELL + offX;
        const int manadaZ = cellZ * MANADA_CELL + offZ;

        // Solo la columna exacta del candidato alberga el centro de la manada.
        if (worldX != manadaX || worldZ != manadaZ) return r;

        // --- 6. Test de densidad ---
        // El candidato existe; ahora se decide si prospera, segun la idoneidad
        // del bioma y la altitud.
        const float roll = Noise::valueAt2D(seedBioma(), cellX, cellZ);
        if (roll > idoneidad * factorAltitud) return r;

        // --- 7. Tamano de la manada ---
        int minM = 0, maxM = 0;
        RangoManada(biome, minM, maxM);
        if (maxM <= 0) return r;

        const uint32_t hT = Noise::rawHash2D(seedTamano(), cellX, cellZ);
        const int rango = maxM - minM + 1;
        r.miembros = minM + (int)(hT % (uint32_t)rango);

        // --- 8. Dispersion de los miembros ---
        // Los miembros NO aparecen todos en la misma columna.
        //
        // MEDIDO (Byers y Bekoff 1981): "la unidad social es una manada
        // cohesiva, en la que se mantienen distancias interindividuales
        // PEQUENAS". La dispersion debe ser corta: es un grupo apretado, no
        // una nube.
        //
        // 8 bloques = 4.8 m de radio real. Con 9 individuos, eso da una
        // densidad de grupo que se lee como manada compacta.
        // confianza: ESTIMADO (el valor) / MEDIDO (que debe ser pequeno)
        r.dispersionBloques = 8.0f;

        r.existe  = true;
        r.centroX = manadaX;
        r.centroZ = manadaZ;
        return r;
    }

    // ------------------------------------------------------------------------
    // POSICION DE UN MIEMBRO DENTRO DE LA MANADA
    // ------------------------------------------------------------------------
    // Reparte el miembro `indice` alrededor del centro, de forma determinista.
    //
    // Se usa una espiral de Fermat (angulo aureo) en vez de un reparto al
    // azar: con pocos puntos, el azar produce grumos y solapamientos visibles,
    // mientras que el angulo aureo reparte de forma uniforme y natural. Es la
    // misma familia de razones por la que los arboles usan Poisson.
    //
    // El jitter por hash rompe la regularidad perfecta, que se veria
    // artificial.
    void PosicionMiembro(const ManadaSpawn& m, int indice,
                         int& outX, int& outZ) const {
        if (indice <= 0) {
            // El miembro 0 va exactamente en el centro.
            outX = m.centroX;
            outZ = m.centroZ;
            return;
        }

        // Angulo aureo: 2*pi / phi^2 ~= 2.39996 rad
        constexpr float ANGULO_AUREO = 2.39996323f;
        const float ang = (float)indice * ANGULO_AUREO;

        // El radio crece con sqrt(indice) para que la densidad sea uniforme
        // en area, no en radio.
        const float radioMax = m.dispersionBloques;
        const float radio = radioMax * std::sqrt((float)indice /
                                                 (float)(m.miembros > 1 ? m.miembros - 1 : 1));

        // Jitter determinista para romper la simetria perfecta.
        const uint32_t hJ = Noise::rawHash2D(seedManada() + 71,
                                             m.centroX + indice, m.centroZ);
        const float jitter = ((float)(hJ % 100u) / 100.0f - 0.5f) * 2.0f;  // [-1,1]

        outX = m.centroX + (int)std::lround(radio * std::cos(ang) + jitter);
        outZ = m.centroZ + (int)std::lround(radio * std::sin(ang) + jitter);
    }

    // ------------------------------------------------------------------------
    // DENSIDAD EFECTIVA (para auditar y para tests)
    // ------------------------------------------------------------------------
    // Devuelve cuantas manadas por km2 REAL produce la configuracion actual.
    // Existe para que el numero no quede escondido: si alguien cambia
    // MANADA_CELL, esto revela de inmediato cuanto se ha desviado del dato
    // medido de 0.2 manadas/km2.
    static float DensidadEfectivaManadasPorKm2() {
        const float ladoCeldaM = (float)MANADA_CELL * 0.60f;   // Fisica::LADO_M
        const float areaCeldaKm2 = (ladoCeldaM * ladoCeldaM) / 1000000.0f;
        return 1.0f / areaCeldaKm2;
    }
};

} // namespace Fauna

#endif // PECARI_SPAWN_H
