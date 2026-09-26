#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include "BlockType.h"

// ============================================================================
// EL AGAVE ABISAII: LA TERCERA ROSETA
// ============================================================================
// Tercera especie de agave del motor, y la primera que arraiga en ROCA. Las
// otras dos se reparten el suelo blando:
//
//   PULQUERO   suelo blando del Altiplano   (pasto, tierra)
//   TEQUILANA  llano seco y ladera          (desierto, montana)
//   ABISAII    afloramiento de roca         (faldas, 60-85, calido no desierto)
//
// ----------------------------------------------------------------------------
// QUE FIJAN ESTOS TESTS, Y QUE NO
// ----------------------------------------------------------------------------
// Igual que en test_agave_azul.cpp: la siembra vive dentro de World::poblacion
// (main.cpp), que no se puede instanciar sin OpenGL ni sin un mundo entero. Lo
// que si se aisla son las dos cosas de las que depende que salga bien:
//
//   1. LA API DE LA PLANTA (logica pura de BloqueCompuesto.h) -- directo.
//   2. LA ARITMETICA DEL HASH de la siembra -- REPLICADA aqui.
//
// ⚠️ El punto 2 es una COPIA, no codigo compartido. Si alguien toca la siembra
// en main.cpp sin tocarla aqui, estos tests siguen pasando pero dejan de
// describir el juego. Por eso cada uno comprueba una PROPIEDAD (determinismo,
// que las tres especies no se pisen, que la densidad sea encontrable) y no un
// valor concreto de tabla. Es la misma limitacion consciente que ya tiene el
// test del tequilana, anotada alli igual.

namespace {

// ----------------------------------------------------------------------------
// COPIA DE LA SIEMBRA DE main.cpp (bloque "AGAVE ABISAII")
// ----------------------------------------------------------------------------

bool hayColoniaAbisaii(int worldX, int worldZ) {
    const int fx = (worldX >= 0 ? worldX : worldX - 19) / 20;
    const int fz = (worldZ >= 0 ? worldZ : worldZ - 19) / 20;
    unsigned hf = (unsigned)(fx * 374761393) ^
                  (unsigned)(fz * 668265263) ^ 0xc2b2ae35u;
    hf ^= hf >> 13; hf *= 1274126177u; hf ^= hf >> 16;

    if ((hf % 1u) != 0u) return false;   // RAREZA_FOCO = 1: toda celda

    const int cx = fx * 20 + (int)((hf >> 3) % 20u);
    const int cz = fz * 20 + (int)((hf >> 11) % 20u);
    const int dx = worldX - cx;
    const int dz = worldZ - cz;
    const int radio = 3 + (int)((hf >> 19) % 4u);
    return (dx * dx + dz * dz) <= radio * radio;
}

// ¿Brota de verdad esta columna, dentro de una colonia?
bool brotaAbisaii(int worldX, int worldZ) {
    if (!hayColoniaAbisaii(worldX, worldZ)) return false;

    const int fx = (worldX >= 0 ? worldX : worldX - 19) / 20;
    const int fz = (worldZ >= 0 ? worldZ : worldZ - 19) / 20;
    unsigned hf = (unsigned)(fx * 374761393) ^
                  (unsigned)(fz * 668265263) ^ 0xc2b2ae35u;
    hf ^= hf >> 13; hf *= 1274126177u; hf ^= hf >> 16;

    const int cx = fx * 20 + (int)((hf >> 3) % 20u);
    const int cz = fz * 20 + (int)((hf >> 11) % 20u);
    const int dx = worldX - cx;
    const int dz = worldZ - cz;
    const int d2 = dx * dx + dz * dz;
    const int radio = 3 + (int)((hf >> 19) % 4u);

    unsigned hb = (unsigned)(worldX * 4517) ^
                  (unsigned)(worldZ * 8291) ^ 0x27d4eb2fu;
    hb ^= hb >> 13; hb *= 2246822519u; hb ^= hb >> 16;

    const int borde = radio * radio;
    const unsigned prob = 45u -
        (unsigned)((30 * d2) / (borde > 0 ? borde : 1));
    return (hb % 100u) < prob;
}

// Las colonias de las otras dos especies, para comprobar que no coinciden.
bool hayColoniaTequilana(int worldX, int worldZ) {
    const int fx = (worldX >= 0 ? worldX : worldX - 23) / 24;
    const int fz = (worldZ >= 0 ? worldZ : worldZ - 23) / 24;
    unsigned hf = (unsigned)(fx * 2654435761u) ^
                  (unsigned)(fz * 2246822519u) ^ 0x9d2c5681u;
    hf ^= hf >> 13; hf *= 1274126177u; hf ^= hf >> 16;
    if ((hf % 6u) != 0u) return false;
    const int cx = fx * 24 + (int)((hf >> 3) % 24u);
    const int cz = fz * 24 + (int)((hf >> 11) % 24u);
    const int dx = worldX - cx;
    const int dz = worldZ - cz;
    const int radio = 2 + (int)((hf >> 19) % 3u);
    return (dx * dx + dz * dz) <= radio * radio;
}

bool hayColoniaMaguey(int worldX, int worldZ) {
    const int fx = (worldX >= 0 ? worldX : worldX - 15) / 16;
    const int fz = (worldZ >= 0 ? worldZ : worldZ - 15) / 16;
    unsigned hf = (unsigned)(fx * 374761393) ^
                  (unsigned)(fz * 668265263);
    hf ^= hf >> 13; hf *= 1274126177u; hf ^= hf >> 16;
    if ((hf % 4u) != 0u) return false;
    const int cx = fx * 16 + (int)((hf >> 3) % 16u);
    const int cz = fz * 16 + (int)((hf >> 11) % 16u);
    const int dx = worldX - cx;
    const int dz = worldZ - cz;
    const int radio = 2 + (int)((hf >> 19) % 3u);
    return (dx * dx + dz * dz) <= radio * radio;
}

} // namespace

// ============================================================================
// LA PLANTA ES UN COMPUESTO BIEN FORMADO
// ============================================================================

TEST_CASE("Abisaii: lo sembrado se reconoce como su familia") {
    namespace AB = Compuesto::AgaveAbisaii;

    // Si esto fallara, ni la textura ni el mesher lo reconocerian: los dos
    // preguntan por familia antes que por nada.
    for (uint16_t etapa = AB::HIJUELO; etapa <= AB::MADURA; ++etapa) {
        for (uint16_t giro = 0; giro < 8; ++giro) {
            const BlockType b = AB::nuevo(etapa, giro);
            INFO("etapa ", etapa, " giro ", giro);
            CHECK(Compuesto::esCompuesto(b));
            CHECK(Compuesto::familiaDe(b) == Compuesto::FAM_AGAVE_ABISAII);
            CHECK(AB::esAgaveAbisaii(b));

            // Recuperar lo guardado: si el empaquetado se desalineara, una
            // planta madura naceria como hijuelo.
            CHECK(AB::etapaDe(b) == etapa);
            CHECK(AB::giroDe(b) == giro);
            CHECK(AB::segmentoDe(b) == 0);
            CHECK(AB::esBase(b));
        }
    }
}

TEST_CASE("Abisaii: no invade el espacio de IDs de otra familia") {
    namespace AB = Compuesto::AgaveAbisaii;

    // Un ID fuera de su franja se leeria como OTRA planta al cargar el mundo:
    // corrupcion silenciosa, no un crash.
    const int base = Compuesto::COMPUESTO_BASE +
                     (int)Compuesto::FAM_AGAVE_ABISAII *
                     Compuesto::ESTADOS_POR_FAMILIA;
    const int fin = base + Compuesto::ESTADOS_POR_FAMILIA;

    for (uint16_t etapa = AB::HIJUELO; etapa <= AB::MADURA; ++etapa) {
        for (uint16_t giro = 0; giro < 8; ++giro) {
            const BlockType planta = AB::nuevo(etapa, giro);
            for (uint16_t seg = 0; seg < 8; ++seg) {
                const BlockType t = AB::conSegmento(planta, seg);
                INFO("etapa ", etapa, " giro ", giro, " seg ", seg);
                CHECK((int)t >= base);
                CHECK((int)t < fin);
                // El segmento se recupera intacto: es lo que decide quien
                // dibuja la planta (solo la base) y quien solo ocupa sitio.
                CHECK(AB::segmentoDe(t) == seg);
                // Y no pisa los otros campos al escribirse.
                CHECK(AB::etapaDe(t) == etapa);
                CHECK(AB::giroDe(t) == giro);
            }
        }
    }
}

TEST_CASE("Abisaii: la familia sabe dibujarse") {
    namespace AB = Compuesto::AgaveAbisaii;
    // Sin esto la planta se sembraria y NO se veria: puedeDibujarse() es el
    // portero que decide si una familia tiene geometria. Es justo el fallo
    // que tuvo la biznaga en su dia.
    CHECK(Compuesto::familiaTieneModelo(Compuesto::FAM_AGAVE_ABISAII));
    CHECK(Compuesto::puedeDibujarse(AB::nuevo(AB::MADURA, 0)));
}

// ============================================================================
// SE SIEMBRA ENTERA, NO DECAPITADA
// ============================================================================
// El bug que ya se corrigio DOS veces en este motor (pulquero y tequilana):
// sembrar solo la celda de abajo deja la planta a medias, porque la base
// dibuja su trozo y encima no hay nada que dibuje el resto.

TEST_CASE("Abisaii: celdasDeEtapa cuenta la planta entera") {
    namespace AB = Compuesto::AgaveAbisaii;

    // Nunca cero: una planta de 0 celdas no se sembraria nunca.
    for (uint16_t e = AB::HIJUELO; e <= AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::celdasDeEtapa(e) >= 1);
        // Y cabe en los 3 bits del campo SEGMENTO.
        CHECK(AB::celdasDeEtapa(e) <= 8);
    }

    // Crece con la edad, nunca encoge.
    for (uint16_t e = AB::HIJUELO; e < AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::celdasDeEtapa(e) <= AB::celdasDeEtapa((uint16_t)(e + 1)));
    }
}

// ============================================================================
// LA MORFOLOGIA ES LA DE LA ESPECIE, NO LA DEL TEQUILANA
// ============================================================================
// Estos tests son los que impiden que la especie se convierta en una copia
// del tequilana si alguien "limpia" los numeros sin saber por que estan.

TEST_CASE("Abisaii: el radio nunca cruza el borde de chunk") {
    namespace AB = Compuesto::AgaveAbisaii;
    // ⚠️ Pasar de 1.5 haria que la planta cruzara la frontera del chunk,
    // donde el recorte del dibujado SI corta: se veria aparecer y desaparecer
    // al girar la camara. El tope de 1.45 existe exactamente por esto, en las
    // tres especies.
    for (uint16_t e = AB::HIJUELO; e <= AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::radioDeEtapa(e) <= 1.45f);
        CHECK(AB::radioDeEtapa(e) > 0.0f);
    }
}

TEST_CASE("Abisaii: es mas ancho que alto, y mas que el tequilana") {
    namespace AB = Compuesto::AgaveAbisaii;
    namespace AG = Compuesto::AgaveAzul;

    for (uint16_t e = AB::HIJUELO; e <= AB::MADURA; ++e) {
        INFO("etapa ", e);
        // Proporcion 1:1.6 -> el diametro (2*radio) supera al alto.
        CHECK(AB::radioDeEtapa(e) * 2.0f > AB::alturaReal(e));
    }

    // Y la proporcion es MAS ancha que la del tequilana (1:1.5), que es lo
    // que separa visualmente las dos especies. Se compara la RELACION, no el
    // tamano absoluto: el abisaii tambien es mas bajo.
    const float propAbisaii =
        (AB::radioDeEtapa(AB::MADURA) * 2.0f) / AB::alturaReal(AB::MADURA);
    const float propTequilana =
        (AG::radioDeEtapa(AG::MADURA) * 2.0f) / AG::alturaReal(AG::MADURA);
    CHECK(propAbisaii > propTequilana);
}

TEST_CASE("Abisaii: la hoja se ARQUEA, que es su rasgo") {
    namespace AB = Compuesto::AgaveAbisaii;
    namespace AG = Compuesto::AgaveAzul;

    // El tequilana es rigido (hoja como espada); este se vence por su peso.
    // Si los dos factores se acercaran, las dos especies se confundirian y el
    // trabajo de modelarlas aparte se perderia -- es la misma advertencia que
    // ya lleva escrita la rama del tequilana en el mesher.
    for (uint16_t e = AB::HIJUELO; e <= AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::caidaDeEtapa(e) > AG::caidaDeEtapa(e));
    }

    // Y la caida crece con la edad: la hoja vieja pesa mas y se vence mas.
    for (uint16_t e = AB::HIJUELO; e < AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::caidaDeEtapa(e) <= AB::caidaDeEtapa((uint16_t)(e + 1)));
    }
}

TEST_CASE("Abisaii: menos hojas que el tequilana, porque son mas anchas") {
    namespace AB = Compuesto::AgaveAbisaii;
    namespace AG = Compuesto::AgaveAzul;

    // Con las mismas hojas que el tequilana la roseta se cierra en un bulto
    // macizo y se pierde la luz entre hoja y hoja.
    for (uint16_t e = AB::HIJUELO; e <= AB::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AB::hojasDeEtapa(e) < AG::pencasDeEtapa(e));
        CHECK(AB::hojasDeEtapa(e) >= 3);   // una roseta necesita cuerpo
    }
}

// ============================================================================
// LA SIEMBRA: DETERMINISMO Y REPARTO
// ============================================================================

TEST_CASE("Abisaii: la siembra es determinista") {
    // La misma columna da siempre la misma respuesta. Si no, la planta
    // apareceria y desapareceria al recargar el chunk.
    for (int i = 0; i < 2000; ++i) {
        const int wx = (i * 37) % 900 - 450;
        const int wz = (i * 53) % 900 - 450;
        INFO("(", wx, ", ", wz, ")");
        CHECK(brotaAbisaii(wx, wz) == brotaAbisaii(wx, wz));
        CHECK(hayColoniaAbisaii(wx, wz) == hayColoniaAbisaii(wx, wz));
    }
}

TEST_CASE("Abisaii: la planta es ENCONTRABLE en roca valida") {
    // ⭐ ESTE TEST EXISTE POR UN RIESGO CONCRETO.
    //
    // Las tres condiciones de entrada (piedra + altura 60-85 + calido no
    // desierto) se multiplican, y la de piedra es MUY restrictiva: la
    // superficie generada es casi toda pasto, tierra o arena. Si encima el
    // hash de colonia fuera duro, la planta seria inencontrable -- existiria
    // en el codigo y no en el juego, que es el fallo mas silencioso posible
    // (y el que tuvo el tequilana: estaba entero y nadie lo sembraba).
    //
    // Se mide SOLO lo que quita el hash, suponiendo que la roca ya paso.
    long candidatas = 0, brotan = 0;
    for (int wx = -300; wx < 300; ++wx) {
        for (int wz = -300; wz < 300; ++wz) {
            ++candidatas;
            if (brotaAbisaii(wx, wz)) ++brotan;
        }
    }

    const double pct = 100.0 * (double)brotan / (double)candidatas;
    MESSAGE("[ABISAII] brotan ", brotan, " de ", candidatas,
            " columnas de roca valida (", pct, "%)");

    // Ni desierta ni alfombra. La horquilla es ancha a proposito: lo que se
    // fija es que la planta EXISTA en el mundo y que no lo tape, no un valor
    // exacto que habria que reajustar a cada retoque.
    CHECK(pct > 0.5);
    CHECK(pct < 15.0);
}

TEST_CASE("Abisaii: las tres agavaceas no caen siempre juntas") {
    // ⚠️ CADA ESPECIE LLEVA SU CONSTANTE DE HASH, Y ESTE TEST ES EL PORQUE.
    //
    // Con la misma constante las tres colonias caerian en las mismas
    // coordenadas: donde hubiera una habria las tres, y el reparto del mundo
    // por especies --que es el sentido de tenerlas separadas-- se perderia.
    long soloAbisaii = 0, coincideTeq = 0, coincideMag = 0, totalAbisaii = 0;

    for (int wx = -400; wx < 400; ++wx) {
        for (int wz = -400; wz < 400; ++wz) {
            if (!hayColoniaAbisaii(wx, wz)) continue;
            ++totalAbisaii;
            const bool t = hayColoniaTequilana(wx, wz);
            const bool m = hayColoniaMaguey(wx, wz);
            if (t) ++coincideTeq;
            if (m) ++coincideMag;
            if (!t && !m) ++soloAbisaii;
        }
    }

    REQUIRE(totalAbisaii > 0);

    const double pctSolo = 100.0 * (double)soloAbisaii / (double)totalAbisaii;
    MESSAGE("[ABISAII] de ", totalAbisaii, " celdas con colonia: ",
            pctSolo, "% son solo suyas");

    // La mayoria de sus colonias son suyas y de nadie mas. No se exige el
    // 100%: que se solapen A VECES es natural y hasta deseable -- lo que no
    // puede pasar es que vayan SIEMPRE juntas, que es lo que delataria una
    // constante de hash repetida.
    CHECK(pctSolo > 60.0);
}

// ============================================================================
// LAS TEXTURAS Y EL TOPE DEL ENUM
// ============================================================================

TEST_CASE("Abisaii: sus dos texturas estan dentro del tope del enum") {
    // Si quedaran por encima de BLOCK_TYPE_MAX, prewarmItemTextures() no las
    // recorreria (icono sin textura) y la validacion de IDs de disco las
    // daria por invalidas. Es el bug que ya costo dos veces.
    CHECK((int)BLOCK_AGAVE_ABISAII_PENCA <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_AGAVE_ABISAII_PUNTA <= (int)BLOCK_TYPE_MAX);

    // Y son DOS bloques distintos: si alguien las fundiera en uno, la espina
    // pasaria a dibujarse con la imagen de la hoja.
    CHECK((int)BLOCK_AGAVE_ABISAII_PENCA != (int)BLOCK_AGAVE_ABISAII_PUNTA);
}

TEST_CASE("Abisaii: sus piezas son planta, no roca") {
    // ⚠️ "Roca" se define por EXCLUSION en esRocaParaPico(), asi que cada
    // planta nueva entra sola en la lista del pico si nadie se acuerda de
    // excluirla. Ese agujero ya salio una vez con el maguey.
    CHECK_FALSE(esRocaParaPico(BLOCK_AGAVE_ABISAII_PENCA));
    CHECK_FALSE(esRocaParaPico(BLOCK_AGAVE_ABISAII_PUNTA));
}
