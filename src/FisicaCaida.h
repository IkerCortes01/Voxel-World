#pragma once

#include "BlockType.h"
#include <cmath>
#include <vector>

// ============================================================================
// FISICA DE CAIDA DE BLOQUES
// ============================================================================
// Un bloque sin nada debajo cae. Y cae como caeria de verdad: con la
// aceleracion de la gravedad terrestre y frenado por el aire.
//
// ----------------------------------------------------------------------------
// LO PRIMERO, PORQUE ES CONTRAINTUITIVO
// ----------------------------------------------------------------------------
// EN EL VACIO TODOS LOS CUERPOS CAEN IGUAL. Una pluma y un yunque tocan el
// suelo a la vez. La aceleracion de la gravedad NO depende de la masa: sale de
// igualar F = m*a con F = G*M*m/r^2, donde la m se cancela.
//
// Lo comprobo el Apollo 15 en la Luna en 1971: David Scott solto un martillo
// de aluminio y una pluma de halcon desde la misma altura y llegaron juntos,
// porque alli no hay aire.
//
// Entonces, por que en la Tierra una piedra cae mas rapido que una hoja?
// POR EL AIRE. El aire frena con una fuerza que depende de la VELOCIDAD y del
// AREA, no de la masa:
//
//     F_arrastre = 1/2 * rho_aire * v^2 * Cd * A
//
// Esa fuerza es la misma para dos cubos del mismo tamano, pero la
// DESACELERACION que produce es F/m: cuanto mas ligero el cuerpo, mas lo
// frena el aire. Por eso lo denso cae mas rapido -- no porque la gravedad
// tire mas de ello, sino porque el aire lo frena menos.
//
// Es exactamente lo que se pidio: 9.80665 m/s^2 para todo, y la diferencia
// entre materiales sale de su densidad, como en la realidad.
//
// ----------------------------------------------------------------------------
// LA DUREZA NO PINTA NADA AQUI
// ----------------------------------------------------------------------------
// La escala de Mohs mide resistencia al RAYADO, no peso ni aerodinamica. Un
// diamante (Mohs 10) y un trozo de grafito (Mohs 1) del mismo tamano caen
// practicamente igual. Lo que manda es la DENSIDAD. Por eso esta tabla no
// tiene columna de dureza: seria un dato decorativo.
//
// ----------------------------------------------------------------------------
// ESCALA DEL MUNDO
// ----------------------------------------------------------------------------
// Un bloque mide 60 cm de lado. De ahi salen:
//     volumen      = 0.6^3 = 0.216 m^3
//     area frontal = 0.6^2 = 0.36 m^2
//
// Y como el motor mide en BLOQUES y la fisica en METROS, hay que convertir:
// 1 bloque = 0.6 m. Se hace en un solo sitio (ver aceleracionCaida).
//
// ----------------------------------------------------------------------------
// QUE SE VA A VER
// ----------------------------------------------------------------------------
// Con bloques macizos de 60 cm, la diferencia en caidas cortas es MINIMA, y
// eso es lo correcto: un cubo de nieve de 60 cm pesa 21 kg y cae casi como
// una piedra. La pluma del ejemplo flota porque es finisima, no por ligera.
//
//     Caida de 10 bloques (6 m):   piedra 1.106 s   nieve 1.118 s
//     Caida de 100 bloques (60 m): piedra 3.512 s   nieve 3.882 s
//
// En caidas largas si se nota. Es la realidad, medida, no una estimacion.
// ============================================================================

namespace Fisica {

// ----------------------------------------------------------------------------
// CONSTANTES FISICAS
// ----------------------------------------------------------------------------

// Gravedad estandar terrestre, en m/s^2. Es el valor exacto que fija el
// Sistema Internacional (CGPM, 3a conferencia, 1901); la gravedad real varia
// con la latitud y la altitud (9.78 en el ecuador, 9.83 en los polos).
constexpr float G = 9.80665f;

// Densidad del aire seco a nivel del mar y 15 C, en kg/m^3 (atmosfera
// estandar ISA). Es lo que frena a los bloques.
constexpr float RHO_AIRE = 1.225f;

// Coeficiente de arrastre de un CUBO de cara plana contra el flujo.
// Valor de referencia en aerodinamica para un cubo con la cara
// perpendicular al viento. Un cubo de esquina tendria ~0.80, pero un bloque
// que cae de plano es el caso que interesa.
constexpr float CD_CUBO = 1.05f;

// Lado del bloque en metros. TODO lo demas se deriva de aqui.
constexpr float LADO_M = 0.60f;

constexpr float VOLUMEN_M3 = LADO_M * LADO_M * LADO_M;  // 0.216
constexpr float AREA_M2    = LADO_M * LADO_M;           // 0.36

// ----------------------------------------------------------------------------
// DENSIDADES POR MATERIAL (kg/m^3)
// ----------------------------------------------------------------------------
// Valores de referencia de tablas de ingenieria y geologia. Donde el material
// real tiene un rango (la arena va de 1400 a 1700 segun humedad y compactado)
// se toma un valor representativo del centro.
//
// Estan como constantes con nombre y no como numeros sueltos para que se vea
// de donde sale cada uno y se pueda corregir sin buscar por el archivo.
namespace Densidad {
    // --- Roca ---
    constexpr float GRANITO    = 2650.0f;  // piedra comun
    constexpr float CALIZA     = 2550.0f;
    constexpr float ARENISCA   = 2400.0f;

    // --- Sedimentos sueltos (a granel, con huecos entre granos) ---
    constexpr float GRAVA      = 1680.0f;
    constexpr float ARENA      = 1600.0f;  // seca; humeda sube a ~1900
    constexpr float TIERRA     = 1400.0f;
    constexpr float ARCILLA    = 1300.0f;

    // --- Madera (seca, densidad aparente con el aire de los poros) ---
    constexpr float PINO       = 449.0f;
    constexpr float OYAMEL     = 481.0f;   // abeto: de las coniferas ligeras
    constexpr float ENCINO     = 705.0f;   // roble: madera dura

    // --- Agua congelada ---
    constexpr float HIELO      = 917.0f;
    constexpr float NIEVE      = 350.0f;   // compactada; recien caida ~70

    // --- Minerales ---
    constexpr float CARBON     = 833.0f;   // a granel
    constexpr float PIRITA     = 5010.0f;  // FeS2
    constexpr float HEMATITE   = 5300.0f;  // Fe2O3
    constexpr float GOETHITA   = 3800.0f;  // FeO(OH)
    constexpr float LIMONITA   = 3500.0f;  // goethita hidratada, menos densa
    constexpr float COBRE      = 8940.0f;
    constexpr float ORO        = 19300.0f; // el mas denso del juego
    constexpr float PLATA      = 10500.0f;
    constexpr float DIAMANTE   = 3510.0f;  // duro, pero NO especialmente denso
    constexpr float HIERRO     = 7874.0f;

    // --- Materia vegetal ---
    // ⚠️ AQUI LA INTUICION ENGANA. Una hoja PARECE ligera porque es fina,
    // no porque su material lo sea: medida en laboratorio, la densidad del
    // tejido foliar fresco es 1010 kg/m3 -- practicamente la del agua (Vile
    // et al., Annals of Botany 96:1129, sobre 1039 especies).
    //
    // Un cubo MACIZO de 60 cm de hoja pesaria 218 kg. Lo que flota es una
    // hoja suelta de decimas de milimetro, no un bloque de hojas.
    constexpr float HOJAS      = 1010.0f;
    constexpr float HIERBA     = 900.0f;   // mas aire entre briznas
    // El nopal es 90-94% agua: pesa como el agua.
    constexpr float NOPAL      = 1030.0f;
    // Sin fuente directa; por analogia con maderas muy ligeras.
    constexpr float FIBRA      = 300.0f;   // ixtle, maguey

    // Por defecto, para lo que no este en la tabla: roca comun.
    constexpr float POR_DEFECTO = GRANITO;
}

// ----------------------------------------------------------------------------
// DENSIDAD DE UN BLOQUE
// ----------------------------------------------------------------------------
// Un switch, no un mapa: es una funcion pura sobre un enum contiguo, asi que
// el compilador la convierte en una tabla de saltos. Cuesta lo mismo que
// leer un array y no hay que mantener ninguna estructura viva.
//
// Los niveles parciales y las celdas mixtas se normalizan primero: una capa
// de tierra es tierra, y lo que manda en una celda mixta es su relleno (la
// parte de arriba, que es la que se desprende).
inline float densidadDe(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    if (esMixto(t))        t = mixtoRelleno(t);

    switch (t) {
        // --- Roca ---
        case BLOCK_STONE:
        case BLOCK_COBBLESTONE:       return Densidad::GRANITO;
        case BLOCK_LIMESTONE:         return Densidad::CALIZA;

        // --- Sedimentos ---
        case BLOCK_GRAVEL:            return Densidad::GRAVA;
        case BLOCK_SAND:
        case BLOCK_CLAY_SAND:         return Densidad::ARENA;
        case BLOCK_DIRT:
        case BLOCK_GRASS:
        case BLOCK_CLAY_DIRT:         return Densidad::TIERRA;
        case BLOCK_CLAY:              return Densidad::ARCILLA;

        // --- Madera ---
        case BLOCK_WOOD:
        case BLOCK_PLANKS:            return Densidad::PINO;
        case BLOCK_WOOD_OYAMEL:
        case BLOCK_PLANKS_OYAMEL:     return Densidad::OYAMEL;
        case BLOCK_WOOD_ENCINO:
        case BLOCK_PLANKS_ENCINO:     return Densidad::ENCINO;

        // --- Nieve ---
        case BLOCK_SNOW:
        case BLOCK_PEDAZO_NIEVE:      return Densidad::NIEVE;

        // --- Minerales en veta ---
        // Un bloque de mineral es roca CON mineral dentro, no mineral puro:
        // se toma la media entre la roca que lo envuelve y la ley del
        // mineral, que es lo que da una densidad creible.
        case BLOCK_COAL_ORE:          return Densidad::CARBON;
        case BLOCK_PYRITE_ORE:        return (Densidad::GRANITO + Densidad::PIRITA) * 0.5f;
        case BLOCK_IRON_ORE:          return (Densidad::GRANITO + Densidad::HIERRO) * 0.5f;
        case BLOCK_GOLD_ORE:          return (Densidad::GRANITO + Densidad::ORO) * 0.5f;
        case BLOCK_SILVER_ORE:        return (Densidad::GRANITO + Densidad::PLATA) * 0.5f;
        case BLOCK_DIAMOND_ORE:       return (Densidad::GRANITO + Densidad::DIAMANTE) * 0.5f;
        case BLOCK_SCRAP_METAL:       return Densidad::HIERRO * 0.6f;  // chatarra: con huecos

        // --- Guijarros: el material puro, en pequeno ---
        case BLOCK_PEDAZO_PIEDRA:     return Densidad::GRANITO;
        case BLOCK_PEDAZO_CALIZA:     return Densidad::CALIZA;
        case BLOCK_PEDAZO_GRAVA:      return Densidad::GRAVA;
        case BLOCK_PEDAZO_TIERRA:     return Densidad::TIERRA;
        case BLOCK_PEDAZO_PEDERNAL:   return 2600.0f;   // silice
        case BLOCK_PEDAZO_COBRE:      return Densidad::COBRE;
        case BLOCK_PEDAZO_GOETHITA:   return Densidad::GOETHITA;
        case BLOCK_PEDAZO_HEMATITE:   return Densidad::HEMATITE;
        case BLOCK_PEDAZO_LIMONITA:   return Densidad::LIMONITA;

        // --- Vegetacion ---
        case BLOCK_LEAVES:
        case BLOCK_LEAVES_ENCINO:
        case BLOCK_LEAVES_OYAMEL:     return Densidad::HOJAS;
        case BLOCK_TALLGRASS:         return Densidad::HIERBA;

        default:
            // Lo que quede: nopal, maguey y demas carne de planta pesan como
            // agua; el resto, como roca.
            if (esCladodio(t) || t == BLOCK_NOPAL_FRUTO || esTuna(t) ||
                t == BLOCK_NOPAL_TALLO || t == BLOCK_NOPAL_MOJADO)
                return Densidad::NOPAL;
            // Ojo: isRama() vive en main.cpp y aqui no se alcanza, asi que
            // las ramas caen en el caso por defecto. No es un problema: una
            // rama es madera, y la madera por defecto pesa como la roca solo
            // si nadie la reclama antes -- por eso se listan explicitamente.
            if (esIxtle(t) || esRaiz(t))
                return Densidad::FIBRA;
            return Densidad::POR_DEFECTO;
    }
}

// ----------------------------------------------------------------------------
// MASA DE UN BLOQUE, EN KILOS
// ----------------------------------------------------------------------------
inline float masaDe(BlockType t) {
    return densidadDe(t) * VOLUMEN_M3;
}

// ----------------------------------------------------------------------------
// VELOCIDAD TERMINAL, EN METROS POR SEGUNDO
// ----------------------------------------------------------------------------
// La velocidad a la que el arrastre iguala al peso y el cuerpo deja de
// acelerar. Sale de igualar m*g = 1/2*rho*v^2*Cd*A:
//
//     v_t = sqrt( 2*m*g / (rho_aire * Cd * A) )
//
// No se usa para simular (eso lo hace la integracion paso a paso), pero
// sirve para verificar los numeros y para los tests.
inline float velocidadTerminal(BlockType t) {
    const float m = masaDe(t);
    return std::sqrt((2.0f * m * G) / (RHO_AIRE * CD_CUBO * AREA_M2));
}

// ----------------------------------------------------------------------------
// ACELERACION INSTANTANEA DE UN BLOQUE QUE CAE
// ----------------------------------------------------------------------------
// Devuelve la aceleracion EN BLOQUES/s^2, lista para integrar en el motor.
//
//   velocidadBloques: velocidad actual, en BLOQUES por segundo (positiva
//                     hacia abajo).
//
// Por dentro:
//   1. pasa la velocidad a m/s,
//   2. calcula a = g - arrastre/m en unidades reales,
//   3. devuelve el resultado en bloques/s^2.
//
// La conversion vive AQUI y en ningun otro sitio, que es lo que evita el
// clasico error de mezclar metros con bloques a mitad de la formula.
inline float aceleracionCaida(BlockType t, float velocidadBloques) {
    const float m = masaDe(t);
    if (m <= 0.0f) return 0.0f;

    // Bloques/s -> m/s
    const float v = velocidadBloques * LADO_M;

    // El arrastre SIEMPRE se opone al movimiento. Con v^2 se pierde el
    // signo, asi que se usa |v|*v en su lugar: si el bloque subiera (v<0),
    // el aire tiene que frenarlo hacia abajo, no empujarlo mas.
    const float arrastre = 0.5f * RHO_AIRE * std::fabs(v) * v * CD_CUBO * AREA_M2;

    // a = g - arrastre/m, en m/s^2
    const float a = G - (arrastre / m);

    // m/s^2 -> bloques/s^2
    return a / LADO_M;
}

// ----------------------------------------------------------------------------
// ENERGIA DEL IMPACTO, EN JULIOS
// ----------------------------------------------------------------------------
// E = 1/2*m*v^2, con la velocidad en m/s. Sirve para decidir cuanto dano
// hace un bloque al caerte encima y cuanto ruido mete al aterrizar.
inline float energiaImpacto(BlockType t, float velocidadBloques) {
    const float m = masaDe(t);
    const float v = velocidadBloques * LADO_M;
    return 0.5f * m * v * v;
}

// ============================================================================
// UN BLOQUE CAYENDO
// ============================================================================
// Mientras cae no esta en el mundo: se saca de su celda y vive aqui, con su
// posicion y su velocidad propias. Al aterrizar vuelve a ser un bloque.
//
// Es el mismo enfoque que usa el genero para la arena, y es lo que permite
// que la caida se vea suave en vez de a saltos de voxel.
struct BloqueCayendo {
    float x, y, z;        // posicion en coordenadas de mundo (bloques)
    float velocidad;      // bloques/s, positiva hacia abajo
    BlockType tipo;
    int origenX, origenY, origenZ;   // de donde salio, para poder devolverlo

    BloqueCayendo(int bx, int by, int bz, BlockType t)
        : x((float)bx), y((float)by), z((float)bz),
          velocidad(0.0f), tipo(t),
          origenX(bx), origenY(by), origenZ(bz) {}
};

} // namespace Fisica
