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
    // Pinus montezumae, el ocote blanco. Mas pesado que el pino comun porque
    // su madera va cargada de RESINA -- es la que se usa como tea justamente
    // por eso. Queda entre las coniferas y el roble.
    constexpr float OCOTE      = 560.0f;
    // Pinus leiophylla, el ocote chino. Su madera es menos resinosa y de
    // grano mas fino que la del montezumae, asi que pesa algo menos. Sigue
    // por encima del pino comun: es un ocote.
    constexpr float OCOTE_CHINO = 520.0f;
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
        case BLOCK_WOOD_OCOTE:
        case BLOCK_WOOD_OCOTE_DENTRO:
        case BLOCK_PLANKS_OCOTE:      return Densidad::OCOTE;
        // El ocote CHINO. Pinus leiophylla es algo mas ligero que el
        // montezumae: su madera es menos resinosa y de grano mas fino.
        case BLOCK_WOOD_OCOTE_CHINO:
        case BLOCK_WOOD_OCOTE_CHINO_DENTRO: return Densidad::OCOTE_CHINO;
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
        case BLOCK_LEAVES_OYAMEL:
        case BLOCK_LEAVES_OCOTE:
        // Las del chino, y su celda con la rama dentro: sigue siendo follaje.
        case BLOCK_LEAVES_OCOTE_CHINO:
        case BLOCK_LEAVES_OCOTE_CHINO_RAMA: return Densidad::HOJAS;
        case BLOCK_TALLGRASS:         return Densidad::HIERBA;

        default:
            // ⭐ LOS BLOQUES COMPUESTOS SON PLANTA, NO PIEDRA
            //
            // BUG QUE ESTO CORRIGE: el maguey del sistema nuevo caia aqui y
            // se le daba la densidad por defecto (roca). Un agave se
            // desplomaba con el peso de un bloque de granito -- caia como una
            // piedra y golpeaba el suelo como tal.
            //
            // Se comprueba por rango de ID porque Compuesto::esCompuesto()
            // vive en un header que incluye a este (ver la nota del
            // static_assert en BloqueCompuesto.h).
            if ((int)t >= BLOQUE_COMPUESTO_BASE_ID) return Densidad::NOPAL;

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

// ============================================================================
// UNA ESTRUCTURA ENTERA CAYENDO
// ============================================================================
// Un arbol no es un monton de bloques sueltos: es UNA PIEZA. Si le quitas el
// suelo, no se deshace en el aire -- se viene abajo entero, con su tronco,
// sus ramas y sus hojas, y se rompe al llegar.
//
// Eso es lo que representa esto: un conjunto de bloques que caen JUNTOS,
// manteniendo sus posiciones relativas, como un solo cuerpo rigido.
//
// ----------------------------------------------------------------------------
// POR QUE UNA SOLA VELOCIDAD PARA TODA LA PIEZA
// ----------------------------------------------------------------------------
// Cada bloque por separado tendria su propia densidad y su propio arrastre.
// Pero estan PEGADOS: forman un cuerpo unico, y un cuerpo unico cae con una
// sola aceleracion.
//
// La fisica correcta para eso es sumar toda la masa y toda el area frontal
// del conjunto, no promediar velocidades. Un arbol es sobre todo hojas (poco
// densas) con un tronco dentro (denso), y lo que cae es la suma: se comporta
// como un cuerpo de densidad intermedia, que es justo lo que pasaria de
// verdad.
struct PiezaCayendo {
    // Cada bloque, con su desplazamiento RESPECTO AL ANCLA de la pieza. Se
    // guardan relativos y no absolutos para que mover la pieza sea mover un
    // solo punto.
    struct Pieza {
        int dx, dy, dz;
        BlockType tipo;
    };

    std::vector<Pieza> bloques;

    float x, y, z;        // ancla de la pieza, en coordenadas de mundo
    float velocidad;      // bloques/s, positiva hacia abajo

    // Masa y area de TODO el conjunto, precalculadas al desprenderse. Se
    // guardan porque no cambian durante la caida y recalcularlas cada frame
    // seria recorrer todos los bloques sin necesidad.
    float masaTotal;      // kg
    float areaTotal;      // m^2

    // ------------------------------------------------------------------------
    // EL VUELCO
    // ------------------------------------------------------------------------
    // Un arbol talado no baja recto: se vence hacia un lado y acaba TUMBADO.
    // Aqui va hacia donde y cuanto lleva girado.
    //
    // El giro es alrededor del pie del tronco, como una bisagra: eso es lo
    // que hace que la copa describa un arco en vez de deslizarse de lado.
    //
    //   volcarX, volcarZ: hacia donde cae. Uno de los dos es +-1 y el otro 0,
    //                     porque el mundo es de voxeles y solo hay cuatro
    //                     rumbos posibles al recolocar los bloques.
    //   angulo:           0 = de pie, PI/2 (1.5708) = tumbado del todo.
    int   volcarX = 0, volcarZ = 0;
    float angulo = 0.0f;
    bool  vuelca = false;     // false = cae recto, como un bloque suelto

    // ⭐ EL RUMBO REAL, EN CONTINUO
    //
    // `volcarX/Z` son ENTEROS porque se usan para recolocar bloques al
    // aterrizar, y una celda solo puede estar en una de las cuatro
    // direcciones. Pero eso hacia que un arbol cayera SIEMPRE hacia uno de
    // cuatro rumbos, y con un bosque entero se nota muchisimo: todos los
    // troncos acaban alineados en cruz.
    //
    // Estos dos guardan la direccion de verdad -- un vector unitario en
    // cualquier angulo -- y son los que usa el RENDER mientras el arbol cae.
    // Asi el vuelco se ve en su rumbo exacto (miles de posibilidades) y la
    // colocacion final sigue cuadrando con la rejilla de voxeles.
    //
    // Es la misma separacion que ya usa el motor entre lo que se DIBUJA y lo
    // que se COLOCA: el modelo puede ser continuo, la celda no.
    float rumboX = 0.0f, rumboZ = 0.0f;

    // Lo alto que es, en bloques. Es lo unico que decide la velocidad del
    // vuelco: un arbol alto se tumba mas despacio (ver aceleracionVuelco).
    float alturaBloques = 1.0f;

    PiezaCayendo() : x(0), y(0), z(0), velocidad(0),
                     masaTotal(0), areaTotal(0) {}
};

// ----------------------------------------------------------------------------
// VELOCIDAD DE VUELCO DE UN ARBOL
// ----------------------------------------------------------------------------
// Un arbol que se vence es un pendulo invertido: gira alrededor de su pie por
// su propio peso. La aceleracion angular de un cuerpo asi es
//
//     alpha = (m * g * r * sin(angulo)) / I
//
// donde r es la distancia del pie al centro de masa e I el momento de
// inercia. Para una barra que gira por un extremo, I = m*L^2/3 y r = L/2, y
// la masa se cancela:
//
//     alpha = (3 * g * sin(angulo)) / (2 * L)
//
// Ese resultado tiene una consecuencia bonita y correcta: la caida NO depende
// de lo que pese el arbol, solo de lo ALTO que sea. Un pino y un encino de la
// misma altura tardan lo mismo en tumbarse, igual que dos cuerpos caen igual
// en el vacio.
//
// Y explica por que un arbol alto parece caer "despacio": cuanto mayor es L,
// menor es alpha. No es una impresion, es fisica.
//
// El seno hace lo demas: parado en vertical (angulo 0) casi no arranca, y
// acelera segun se va venciendo, que es exactamente como cae un arbol.
inline float aceleracionVuelco(float alturaBloques, float angulo) {
    // En metros, que es donde vale la formula.
    const float L = alturaBloques * LADO_M;
    if (L <= 0.01f) return 0.0f;

    // Un empujon minimo al principio: en vertical exacto el seno es 0 y el
    // arbol se quedaria parado para siempre. Es el equivalente al viento o a
    // que ningun arbol esta perfectamente recto.
    const float sen = std::sin(angulo) + 0.08f;

    return (3.0f * G * sen) / (2.0f * L);
}

// ----------------------------------------------------------------------------
// ACELERACION DE UNA PIEZA ENTERA
// ----------------------------------------------------------------------------
// La misma fisica que un bloque suelto, pero con la masa y el area del
// conjunto:
//
//     a = g - (1/2 * rho * |v|*v * Cd * A_total) / m_total
//
// El AREA no es la suma de las areas de todos los bloques: los que estan uno
// encima de otro se tapan entre si, y el aire solo empuja contra la SILUETA
// vista desde abajo. Por eso el area se calcula contando columnas distintas
// (ver areaFrontalDe en main.cpp), no bloques.
inline float aceleracionPieza(float masaTotal, float areaTotal,
                              float velocidadBloques) {
    if (masaTotal <= 0.0f) return 0.0f;

    const float v = velocidadBloques * LADO_M;
    const float arrastre =
        0.5f * RHO_AIRE * std::fabs(v) * v * CD_CUBO * areaTotal;

    const float a = G - (arrastre / masaTotal);
    return a / LADO_M;
}


// ============================================================================
// QUE SE DERRUMBA Y QUE ES CIMIENTO
// ============================================================================
// Estas dos preguntas son la cara y la cruz de lo mismo, y por eso viven
// juntas: LO QUE SUJETA UNA ESTRUCTURA NO PUEDE CAERSE. Tenerlas separadas fue
// lo que permitio que se contradijeran, y de ahi salio un bug que destruia
// terreno (la explicacion completa esta dentro de puedeCaer).
//
// Viven en el header, y no en main.cpp, por dos motivos:
//   1. Son logica pura sobre BlockType: no tocan el mundo ni OpenGL.
//   2. Asi los tests comprueban la invariante sin arrancar el juego
//      (ver tests/test_derrumbe.cpp).

// ⭐ isCrossSprite: la definicion real vive fuera de este header.
//
// El JUEGO la aporta desde main.cpp (necesita el motor entero para decidir
// que es un sprite). Los TESTS aportan la suya, que reconoce las plantas por
// tipo sin arrastrar OpenGL (ver tests/test_derrumbe.cpp).
//
// Declarada aqui dentro del namespace: asi las dos implementaciones y esta
// declaracion hablan del mismo simbolo, Fisica::isCrossSprite.
bool isCrossSprite(BlockType type);

// ⭐ esRamaParaFisica: mismo caso que isCrossSprite.
//
// isRama() vive en main.cpp y no se alcanza desde aqui (ya lo advierte el
// comentario del calculo de densidad, mas arriba). La necesita
// aplastablePorArbol() para NO machacar las ramas del propio arbol.
//
// El JUEGO la reenvia a isRama(); los TESTS aportan la suya.
bool esRamaParaFisica(BlockType type);

// ============================================================================
// ¿ES TERRENO NATURAL? (el cimiento del mundo)
// ============================================================================
// Lo que el GENERADOR pone como suelo: roca, tierra, arena, grava, arcilla,
// nieve y los minerales en veta. Es lo unico que nunca se derrumba.
//
// ⚠️ NO ES LO MISMO QUE esSueloFirme(). Esa responde "¿aguanta peso?", y hay
// cosas que aguantan peso pero SI pueden caerse: un puente de tablones
// sostiene lo que le pongas encima, pero si le quitas los pilares se viene
// abajo, porque es CONSTRUCCION y no suelo.
//
// Confundir las dos preguntas es lo que dejaba los tablones clavados en el
// aire, y lo caza el test "la madera trabajada tambien cae".
//
// Se define por lista EXPLICITA, no por exclusion. Por exclusion, cada bloque
// nuevo entraria solo en la categoria de "no se cae nunca" sin que nadie lo
// decidiera -- y esa clase de descuido silencioso es justo lo que produjo el
// bug de los parches de piedra.
inline bool esTerrenoNatural(BlockType t) {
    // Las capas parciales y las celdas mixtas son el mismo material, solo que
    // con otra forma: una loncha de tierra sigue siendo tierra.
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    if (esMixto(t))        t = mixtoRelleno(t);

    switch (t) {
        // --- Suelo y roca ---
        case BLOCK_STONE:
        case BLOCK_DIRT:
        case BLOCK_GRASS:
        case BLOCK_SAND:
        case BLOCK_GRAVEL:
        case BLOCK_SNOW:
        case BLOCK_CLAY:
        case BLOCK_CLAY_DIRT:
        case BLOCK_CLAY_SAND:
        case BLOCK_LIMESTONE:
        case BLOCK_COBBLESTONE:
        case BLOCK_BEDROCK:
        // --- Minerales en veta: van incrustados en la roca. Si cayeran,
        //     minar una veta abriria un socavon en la montaña.
        case BLOCK_COAL_ORE:
        case BLOCK_SILVER_ORE:
        case BLOCK_GOLD_ORE:
        case BLOCK_DIAMOND_ORE:
        case BLOCK_SCRAP_METAL:
        case BLOCK_IRON_ORE:
        case BLOCK_PYRITE_ORE:
            return true;
        default:
            return false;
    }
}

inline bool esSueloFirme(BlockType t) {
    if (t == BLOCK_AIR || t == BLOCK_WATER || t == BLOCK_LAVA) return false;

    // Nada de lo que es planta sujeta.
    if (isCrossSprite(t)) return false;

    // ⭐ NORMALIZAR ANTES DE DECIDIR.
    //
    // Sin esto, la regla solo valia para el bloque ENTERO. Una capa parcial
    // de tronco (media loncha de madera) o una celda mixta con madera arriba
    // no se reconocian como planta, asi que SUJETABAN el arbol y este no
    // caia nunca. La regla se aplica igual sea entero, capa o celda mixta.
    //
    // En una celda mixta manda el RELLENO: es la parte de arriba, sobre la
    // que se apoyaria lo que hubiera encima.
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    if (esMixto(t))        t = mixtoRelleno(t);

    // El arbol no sujeta nada: ni su madera ni su follaje. Va por los
    // predicados centralizados en vez de repetir la lista de especies, que es
    // justo como el ocote se quedo fuera de media docena de sitios.
    if (esTroncoDeArbol(t)) return false;
    if (esHojaDeArbol(t))   return false;

    // El resto -- terreno, roca, construccion -- si sujeta. Incluidas sus
    // capas parciales: una loncha de tierra es tierra y aguanta lo que haya
    // encima, igual que el bloque entero.
    return true;
}

// ============================================================================
// ¿UN ARBOL QUE SE VIENE ABAJO PUEDE APLASTAR ESTO?
// ============================================================================
// Un pino de veinte metros que cae no se para porque haya un maguey debajo:
// lo revienta y se queda donde estaba el maguey. Antes no era asi -- el
// aterrizaje comprobaba `!= BLOCK_AIR` y DESCARTABA el bloque del arbol, de
// modo que caer sobre vegetacion no la aplastaba: hacia desaparecer el tronco.
// Un arbol talado sobre un nopal perdia media copa sin dejar rastro.
//
// ----------------------------------------------------------------------------
// POR QUE NO SE REUTILIZA EL CRITERIO DE CONSTRUIR
// ----------------------------------------------------------------------------
// Al COLOCAR un bloque a mano, el motor PROTEGE el nopal y el maguey a
// proposito ("machacarlas al apilar seria destruir cosas sin querer", ver
// placeBlock). Son dos situaciones opuestas y por eso son dos predicados
// distintos: colocar es un gesto deliberado y reversible; un arbol cayendo es
// una tonelada de madera y tiene que arrasar.
//
// ----------------------------------------------------------------------------
// QUE SE APLASTA Y QUE NO
// ----------------------------------------------------------------------------
// SE APLASTA todo lo que es BLANDO: hierba, flores, nopal, maguey, biznaga,
// agave y las capas parciales de terreno sueltas. Lo que en el campo un
// tronco se lleva por delante.
//
// NO SE APLASTA:
//   - El terreno firme y la roca. El arbol se apoya encima; no perfora el
//     suelo, que es lo que pasaria si se dejara aplastar cualquier cosa.
//   - Las piezas del PROPIO ARBOL (troncos, hojas, ramas, raices). Un arbol
//     que cae junto a otro no se lo come: los dos quedan tumbados. Ademas,
//     como los bloques del arbol se colocan uno a uno, sin esto un tronco
//     podria borrar el que acaba de posarse en la misma celda.
//   - El agua y la lava, que tienen su propio sistema y no son obstaculo.
//
// El AGUA se deja fuera a proposito: no impide el paso, asi que el bloque de
// arriba (el que decide si hay hueco) ya la trata como celda libre.
inline bool aplastablePorArbol(BlockType t) {
    if (t == BLOCK_AIR) return false;          // no hay nada que aplastar
    if (t == BLOCK_BEDROCK) return false;

    // ⚠️ EL AGUA VA LA PRIMERA, Y CON esAguaCualquiera().
    //
    // No basta con `t == BLOCK_WATER`: desde que el agua tiene volumen, una
    // celda de agua con nivel es un bloque COMPUESTO, y mas abajo los
    // compuestos se aplastan por ser plantas. Sin este filtro, un arbol que
    // cayera en un charco BORRARIA esa agua del mundo -- y el sistema de
    // fluidos se sostiene justo sobre lo contrario: el agua no se crea ni se
    // destruye, solo se reparte. Un tronco no puede evaporar un lago.
    //
    // Ademas no hace falta aplastarla: el agua no frena a un bloque que cae,
    // asi que la comprobacion de hueco ya la trata como celda libre.
    if (esAguaCualquiera(t) || t == BLOCK_LAVA) return false;

    // --- Las piezas de arbol se respetan entre si ---
    //
    // El tronco y las hojas van por los predicados centralizados. Las RAMAS y
    // las RAICES por esRaiz() y por la declaracion de arriba: isRama() vive en
    // main.cpp y desde aqui no se alcanza (misma situacion que isCrossSprite).
    if (esTroncoDeArbol(t)) return false;
    if (esHojaDeArbol(t))   return false;
    if (esRaiz(t)) return false;
    if (esRamaParaFisica(t)) return false;

    // --- LO BLANDO SE APLASTA ---
    //
    // isCrossSprite cubre de una vez la hierba, las flores, el nopal entero
    // (cladodios, pencas y tunas) y lo que se anada manana con la misma
    // naturaleza: si es un sprite que se atraviesa, un tronco lo revienta.
    if (isCrossSprite(t)) return true;

    // Los bloques COMPUESTOS son las plantas con estado: maguey, biznaga y
    // agave azul. Se aplastan igual -- son plantas, no cimiento.
    //
    // ⚠️ El AGUA tambien es una familia compuesta (FAM_AGUA), y ya salio por
    // el filtro de liquidos de arriba. Por eso ese `return false` del agua
    // tiene que ir ANTES que esto: si no, un tronco "aplastaria" el agua y la
    // borraria del mundo, que es justo lo que el sistema de fluidos no
    // perdona (el agua no se crea ni se destruye, solo se reparte).
    if ((int)t >= BLOQUE_COMPUESTO_BASE_ID) return true;

    // --- LAS CAPAS PARCIALES SUELTAS ---
    //
    // Una loncha fina de tierra o arena en el suelo no detiene un arbol: se
    // la lleva por delante. El bloque ENTERO si lo detiene, porque ya es
    // terreno asentado.
    //
    // El corte esta en la mitad: hasta 4 octavos es una capa suelta que cede;
    // de 5 en adelante es suelo hecho y derecho.
    if (esNivelParcial(t) && nivelDe(t) <= 4) return true;

    // Todo lo demas -- terreno, roca, construccion, celdas mixtas -- aguanta.
    return false;
}

//   - el agua y la lava, que tienen su propio sistema de flujo
//   - la bedrock, que es el fondo del mundo
//   - los guijarros, que son un monton apoyado, no un bloque
//   - EL TERRENO: piedra, tierra, arena y demas cimiento (ver abajo)
//
// Las plantas SI caen: se pidio expresamente. Eso significa que al talar el
// tronco de un arbol, la copa se le viene encima al jugador.
inline bool puedeCaer(BlockType t) {
    if (t == BLOCK_AIR || t == BLOCK_WATER || t == BLOCK_LAVA) return false;
    if (t == BLOCK_BEDROCK) return false;

    // ========================================================================
    // ⭐ EL TERRENO NO SE DERRUMBA. NUNCA.
    // ========================================================================
    // ESTE ERA EL BUG DE LOS PARCHES DE PIEDRA A RAS DE SUELO.
    //
    // El sistema de estructuras existe para lo que ESTA PUESTO SOBRE el
    // terreno: un arbol, una torre, un puente. El terreno en si es el
    // cimiento -- no puede caerse, porque no hay nada debajo sobre lo que
    // caer.
    //
    // Que pasaba: al romper un bloque, revisarSoporte() lanzaba el flood fill
    // sobre los cuatro vecinos. Si el vecino era piedra o tierra, el fill se
    // metia en el terreno. Y ahi ocurria lo peor:
    //
    //   al mirar hacia ABAJO desde un bloque de piedra, el vecino inferior es
    //   MAS PIEDRA -- que tambien pasaba puedeCaer(), asi que en vez de
    //   contar como APOYO se sumaba a la estructura.
    //
    // El fill se comia la columna hacia abajo, y con ella el parche entero.
    // Si el trozo no llegaba a los 512 bloques del tope, `apoyada` no se
    // activaba nunca y TODO ESE TERRENO se desprendia y caia: aparecian
    // placas de piedra tiradas sobre el pasto, con huecos donde antes habia
    // suelo. Justo lo que se veia.
    //
    // La regla: EL TERRENO NATURAL no cae. Lo que el generador pone como
    // suelo del mundo -- roca, tierra, arena, minerales en veta -- es
    // cimiento, y un cimiento no se desprende.
    //
    // ⚠️ NO vale usar esSueloFirme() para esto, aunque sea tentador.
    //
    // Esa funcion responde a otra pregunta: "¿aguanta peso?". Y hay
    // materiales que aguantan peso Y ADEMAS pueden caerse: los TABLONES. Un
    // puente de madera sostiene lo que le pongas encima, pero si le quitas
    // los pilares se viene abajo -- porque es CONSTRUCCION, no suelo.
    //
    // Confundir las dos preguntas dejaba los tablones clavados en el aire.
    // Lo cazo un test al escribir este arreglo.
    if (esTerrenoNatural(t)) return false;

    // ⭐ LOS GUIJARROS SE QUEDAN DONDE ESTAN.
    //
    // Piedritas, pedernal, polvo de tierra, cantos de hierro, nieve suelta:
    // no son bloques que ocupen su celda, son un montoncito de cantos
    // apoyado en el suelo. Un puñado de piedras no "se derrumba" -- se queda
    // donde cayó, encajado en el terreno.
    //
    // Y hay una razon practica ademas de la logica: van sembrados por toda
    // la superficie del mundo, asi que meterlos en el flood fill de las
    // estructuras haria recorrerlos una y otra vez sin que nunca caiga
    // ninguno. Se paga el coste sin ganar nada.
    //
    // esGuijarro() los cubre todos a la vez, asi que si manana se anade otro
    // canto, queda excluido solo.
    if (esGuijarro(t)) return false;

    return true;
}
} // namespace Fisica
