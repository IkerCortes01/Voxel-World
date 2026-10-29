#pragma once

#include "BlockType.h"
#include <cstdint>

// ============================================================================
// CRISTALES DE MINERAL: EL MINERAL DEJA DE SER UNA TEXTURA
// ============================================================================
// Antes un bloque de mineral era un cubo de piedra con manchas pintadas. Ahora
// de la roca SALEN CRISTALES: prismas que sobresalen hacia el hueco, con la
// forma y el habito de cada especie mineral.
//
// ----------------------------------------------------------------------------
// SOLO EN LA CARA QUE DA AL HUECO
// ----------------------------------------------------------------------------
// Un cristal crece hacia donde hay sitio. En roca maciza no hay cristal: hay
// mineral diseminado, que es justo lo que la textura ya representaba bien.
//
// Y ademas es lo unico que hace esto viable. Medido sobre el volumen que el
// jugador tiene cargado (176x176 columnas):
//
//     bloques de mineral            152.708
//     de esos, EXPUESTOS al hueco    65.678   (43%)
//     CARAS expuestas en total      208.601   (3.18 por bloque)
//
// Poner cristales en las seis caras de cada bloque expuesto seria dibujar
// cinco sextas partes de geometria que nadie ve nunca. Por cara expuesta, en
// cambio, cada quad emitido se ve.
//
// ----------------------------------------------------------------------------
// EL COSTE, DICHO CLARO
// ----------------------------------------------------------------------------
// Con 4 quads por cara expuesta son ~834.000 caras sobre las ~440.000 que ya
// tiene el mundo a la vista: TRIPLICA la geometria. Es una decision de diseño
// tomada a sabiendas, no un descuido -- se midio antes de implementarla.
//
// Lo que NO cuesta: sacar los minerales del greedy meshing. Un mineral rodeado
// de piedra ya emitia sus caras como quads sueltos (la clave de fusion es
// textura + luz, y su textura no es la de la piedra), y encima PARTIA la
// fusion de la roca vecina. Ese coste ya se estaba pagando.
//
// ----------------------------------------------------------------------------
// LA FORMA SALE DE LA MINERALOGIA
// ----------------------------------------------------------------------------
// Cada especie cristaliza en un sistema distinto, y eso da siluetas que se
// reconocen de un vistazo. No son formas inventadas para dar variedad:
//
//   PIRITA      cubico. Cristales CUBICOS perfectos, a veces maclados. Es el
//               mineral con la forma mas reconocible que existe -- se le llama
//               "el oro de los tontos" justamente porque sus cubos dorados
//               engañaban.
//   DIAMANTE    cubico, habito OCTAEDRICO: dos piramides unidas por la base.
//   ORO         cubico, pero rara vez cristaliza: aparece en PEPITAS y
//               dendritas (formas ramificadas), no en prismas.
//   PLATA       igual que el oro: dendritica, en hilos y ramas retorcidas.
//   CARBON      NO ES UN MINERAL CRISTALINO. Es roca organica, mate, sin
//               forma propia. Se representa como masa irregular, no como
//               prisma -- fingir cristales de carbon seria inventarse
//               geologia.
//   HIERRO      la hematites forma rosetas de laminas; la goethita, agujas
//               radiales. Se toma la lamina como forma.
//   DESECHO     no es una especie mineral: es chatarra. Fragmentos angulosos.
// ============================================================================

namespace Cristal {

// ----------------------------------------------------------------------------
// HABITO CRISTALINO
// ----------------------------------------------------------------------------
// La forma que toma el cristal al crecer. Es lo que decide su geometria.
enum class Habito : uint8_t {
    PRISMA = 0,   // columna de seis caras, terminada en punta
    CUBO,         // cubos sueltos, como la pirita
    OCTAEDRO,     // dos piramides base con base: el diamante
    DENDRITA,     // hilos y ramas: el oro y la plata nativos
    LAMINA,       // placas planas: la hematites
    MASA          // sin forma propia: el carbon, la chatarra
};

// ----------------------------------------------------------------------------
// DATOS DE UNA ESPECIE
// ----------------------------------------------------------------------------
struct Especie {
    Habito habito;
    float  altura;      // cuanto sobresale del bloque, en fraccion de voxel
    float  grosor;      // semiancho del cristal
    int    cuantos;     // cuantos cristales por cara expuesta
    float  brillo;      // multiplicador de color: los metales relucen
};

// ============================================================================
// ⚠️ NINGUN BLOQUE LLEVA YA CRISTALES: LOS MINERALES SE RETIRARON
// ============================================================================
// Los siete bloques de mineral de veta se eliminaron del juego. Con ellos se
// fue lo UNICO que este archivo sabia decorar, asi que tieneCristales()
// devuelve false para todo y el mesher no llega a pedir una especie nunca.
//
// El resto del archivo -- los habitos cristalinos, la geometria de cada forma
// y el reparto por cara expuesta -- se conserva INTACTO a proposito: es
// mineralogia, no depende de que bloques existan, y volver a encender los
// cristales el dia que haya un bloque que los luzca es devolver la tabla de
// abajo a la vida. Borrarlo obligaria a rehacer ese trabajo desde cero.
//
// La tabla que habia aqui asignaba a cada mineral su habito real:
//
//     carbon    MASA      (organico, no cristaliza)
//     pirita    CUBO      (el cubico por excelencia)
//     desecho   MASA      (fragmentos angulosos)
//     hierro    LAMINA    (rosetas de hematites)
//     oro       DENDRITA  (nativo, en hilos ramificados)
//     plata     DENDRITA  (igual que el oro)
//     diamante  OCTAEDRO  (dos piramides por la base)
//
// Queda escrita aqui para que no se pierda el dato al recuperarla.
inline Especie EspecieDe(BlockType t) {
    (void)t;
    return { Habito::PRISMA,   0.20f, 0.15f, 3, 1.00f };
}

// ¿Este bloque lleva cristales? Hoy, ninguno: ver la nota de arriba.
inline bool tieneCristales(BlockType t) {
    (void)t;
    return false;
}

// ----------------------------------------------------------------------------
// CUANTOS CRISTALES HAY DE VERDAD EN ESTE BLOQUE
// ----------------------------------------------------------------------------
// El numero de la tabla es el TIPICO; el real varia por posicion, para que dos
// bloques vecinos no salgan calcados. De 60% a 140% del valor de la tabla.
//
// Sale de un hash de la posicion, asi que es determinista: el mismo bloque
// tiene siempre los mismos cristales y no hace falta guardarlo.
inline int CuantosEn(BlockType t, int x, int y, int z) {
    const Especie e = EspecieDe(t);
    unsigned h = (unsigned)(x * 73856093) ^ (unsigned)(y * 19349663) ^
                 (unsigned)(z * 83492791);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;

    // 0.6 .. 1.4 del valor de tabla.
    const float f = 0.6f + (float)(h % 800u) / 1000.0f;
    int n = (int)((float)e.cuantos * f + 0.5f);
    if (n < 1) n = 1;
    if (n > 6) n = 6;   // tope duro: es lo que acota el coste
    return n;
}

// ----------------------------------------------------------------------------
// LA LEY DEL BLOQUE: CUANTO MINERAL DA AL PICARLO
// ----------------------------------------------------------------------------
// ⭐ EL DROP SALE DE LOS CRISTALES QUE SE VEN.
//
// Es lo que se pidio: "si rompes la piedra te da los minerales porque picaste
// los cristales". Un bloque con veta rica da mas que uno con un cristal
// suelto, y la diferencia se VE antes de picar -- el jugador puede elegir a
// que bloque dedicarle el pico.
//
// Se cuentan los cristales de las SEIS caras, no solo de las visibles: al
// romper el bloque se lleva todo lo que tenia dentro, incluido lo que daba a
// la roca. Si solo contara lo visible, el mismo bloque daria distinto segun
// desde donde se hubiera excavado, que seria absurdo.
inline int LeyDelBloque(BlockType t, int x, int y, int z) {
    if (!tieneCristales(t)) return 1;

    // Se suman los cristales de las seis caras. Cada cara usa el hash de la
    // posicion desplazada, igual que hace el mesher al dibujarlas.
    int total = 0;
    static const int CARAS[6][3] = {
        { 1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    for (const auto& c : CARAS)
        total += CuantosEn(t, x + c[0] * 7, y + c[1] * 7, z + c[2] * 7);

    // De ~6-36 cristales a una cantidad jugable. El oro y la plata son
    // dendriticos (5 por cara = hasta 30 hilos) y darian montones absurdos si
    // se contara uno por uno: lo que se recoge es el METAL, no cada hilo.
    int unidades = total / 6;
    if (unidades < 1) unidades = 1;

    // Tope por especie: un diamante no puede dar ocho de golpe.
    const Especie e = EspecieDe(t);
    const int tope = (e.habito == Habito::OCTAEDRO)  ? 3    // diamante
                   : (e.habito == Habito::DENDRITA)  ? 4    // oro, plata
                                                     : 5;
    if (unidades > tope) unidades = tope;
    return unidades;
}

} // namespace Cristal
