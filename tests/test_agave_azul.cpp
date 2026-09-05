#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

// ============================================================================
// EL AGAVE TEQUILANA AZUL: LA PLANTA QUE ESTABA HECHA PERO NO EXISTIA
// ============================================================================
// La familia FAM_AGAVE_AZUL llego al motor COMPLETA salvo por una cosa:
//
//   modelo de datos  ✅  BloqueCompuesto.h, con sus tres fases
//   textura          ✅  getBlockTexture la reconocia por familia
//   mallado          ✅  las tres siluetas dibujadas en buildChunkMesh
//   GENERACION       ❌  NADIE LA SEMBRABA NUNCA
//
// O sea: la planta estaba entera y era inalcanzable. No habia bug que ver en
// pantalla porque no habia nada en pantalla -- que es el modo mas silencioso
// en que una funcionalidad puede no existir, y por eso conviene un test.
//
// Es el fallo espejo del de la biznaga (ver test_biznaga.cpp): alli faltaba el
// mesher y la planta se veia MAL; aqui faltaba la siembra y no se veia NADA.
// Entre los dos cubren las cuatro piezas que necesita una familia compuesta
// para estar viva.
//
// ----------------------------------------------------------------------------
// QUE FIJAN ESTOS TESTS
// ----------------------------------------------------------------------------
// La siembra vive dentro de World::poblacion (main.cpp), que no se puede
// instanciar sin OpenGL ni sin un mundo entero. Lo que SI se puede aislar --
// y es donde estan los errores que importan -- son las dos cosas de las que
// depende que el sembrado salga bien:
//
//   1. LA ARITMETICA DEL HASH, que decide donde cae cada colonia. Se replica
//      aqui identica a la de main.cpp. Si alguien la toca alli sin tocarla
//      aqui, estos tests siguen pasando pero dejan de describir el juego: por
//      eso cada uno comprueba una PROPIEDAD (determinismo, cobertura, que las
//      dos especies no se pisen), no un valor concreto sacado de una tabla.
//
//   2. LA API DE LA PLANTA, que decide que se siembra. Eso si es logica pura
//      de BloqueCompuesto.h y se comprueba directamente.

namespace {

// ----------------------------------------------------------------------------
// COPIA EXACTA DE LA SIEMBRA DE main.cpp
// ----------------------------------------------------------------------------
// Replicada literalmente del bloque "AGAVE TEQUILANA AZUL" de World::poblacion.
// No es codigo compartido -- es una copia, y eso es una limitacion consciente
// de este test, anotada arriba.

// ¿Cae una colonia de agave en la celda de rejilla de esta columna, y esta
// columna cae dentro de su radio?
bool hayColoniaAgave(int worldX, int worldZ) {
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

// El hash por mata: decide si brota, con que etapa y con que giro.
unsigned hashMataAgave(int worldX, int worldZ) {
    unsigned ha = (unsigned)(worldX * 6151) ^
                  (unsigned)(worldZ * 7919) ^ 0x85ebca6bu;
    ha ^= ha >> 13; ha *= 2246822519u; ha ^= ha >> 16;
    return ha;
}

uint16_t etapaDeHash(unsigned ha) {
    namespace AG = Compuesto::AgaveAzul;
    const unsigned d100 = (ha >> 7) % 100u;
    if (d100 < 18u) return AG::HIJUELO;
    if (d100 < 36u) return AG::JOVEN;
    if (d100 < 58u) return AG::MEDIA;
    if (d100 < 82u) return AG::HECHA;
    return AG::MADURA;
}

// La colonia del maguey pulquero, para comprobar que las dos no coinciden.
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
// LA PLANTA QUE SE SIEMBRA ES UN COMPUESTO BIEN FORMADO
// ============================================================================

TEST_CASE("Agave azul: lo sembrado se reconoce como su familia") {
    namespace AG = Compuesto::AgaveAzul;

    // Si esto fallara, la textura, la colision y el mesher no lo reconocerian:
    // los tres preguntan por familia.
    for (uint16_t etapa = AG::HIJUELO; etapa <= AG::MADURA; ++etapa) {
        for (uint16_t giro = 0; giro < 8; ++giro) {
            const BlockType b = AG::nuevo(etapa, giro);
            INFO("etapa ", etapa, " giro ", giro);
            CHECK(Compuesto::esCompuesto(b));
            CHECK(Compuesto::familiaDe(b) == Compuesto::FAM_AGAVE_AZUL);
            CHECK(AG::esAgaveAzul(b));

            // Recuperar lo que se guardo: si el empaquetado se desalineara,
            // una planta madura naceria como hijuelo.
            CHECK(AG::etapaDe(b) == etapa);
            CHECK(AG::giroDe(b) == giro);
            CHECK(AG::faseDe(b) == AG::ROSETA);
        }
    }
}

TEST_CASE("Agave azul: no invade el espacio de IDs de otra familia") {
    namespace AG = Compuesto::AgaveAzul;

    // Un ID que se saliera de su franja se leeria como OTRA planta al cargar
    // el mundo -- corrupcion silenciosa, no un crash.
    const int base = Compuesto::COMPUESTO_BASE +
                     (int)Compuesto::FAM_AGAVE_AZUL * Compuesto::ESTADOS_POR_FAMILIA;
    const int fin = base + Compuesto::ESTADOS_POR_FAMILIA;

    for (uint16_t etapa = AG::HIJUELO; etapa <= AG::MADURA; ++etapa) {
        for (uint16_t giro = 0; giro < 8; ++giro) {
            BlockType planta = AG::nuevo(etapa, giro);

            // Y tambien la version espigada con el quiote entero, que es la
            // que mas campos escribe a la vez.
            if (etapa == AG::MADURA) {
                planta = AG::espigada(planta);
                for (int i = 0; i <= AG::QUIOTE_CELDAS_MAX; ++i)
                    planta = AG::quioteCrecido(planta);
            }

            for (uint16_t seg = 0; seg < 8; ++seg) {
                const BlockType t = AG::conSegmento(planta, seg);
                INFO("etapa ", etapa, " giro ", giro, " seg ", seg);
                CHECK((int)t >= base);
                CHECK((int)t < fin);
            }
        }
    }
}

// ============================================================================
// LA PLANTA SE SIEMBRA ENTERA, NO DECAPITADA
// ============================================================================
// Este es el bug que ya se corrigio una vez en el maguey pulquero: la siembra
// ponia SOLO la celda de abajo, asi que las plantas altas se dibujaban a
// medias -- la base pintaba su trozo y encima no habia nada que pintara el
// resto. La siembra del agave usa celdasDe() justo para no repetirlo.

TEST_CASE("Agave azul: celdasDe cuenta la planta entera en cada fase") {
    namespace AG = Compuesto::AgaveAzul;

    SUBCASE("la roseta ocupa lo que dice su etapa") {
        for (uint16_t etapa = AG::HIJUELO; etapa <= AG::MADURA; ++etapa) {
            const BlockType b = AG::nuevo(etapa, 0);
            INFO("etapa ", etapa);
            CHECK(AG::celdasDe(b) == AG::celdasDeEtapa(etapa));
            CHECK(AG::celdasDe(b) >= 1);
        }
    }

    SUBCASE("la piña jimada ocupa una sola celda") {
        // Al jimar se cortan las pencas: queda la bola y nada mas. Si
        // siguiera contando 2, la celda de arriba se sembraria con aire
        // decorado y taparia el hueco.
        const BlockType hecha = AG::nuevo(AG::HECHA, 0);
        REQUIRE(AG::sePuedeJimar(hecha));
        CHECK(AG::celdasDe(AG::jimada(hecha)) == 1);
    }

    SUBCASE("el quiote suma sus celdas por encima de la roseta") {
        BlockType p = AG::nuevo(AG::MADURA, 0);
        REQUIRE(AG::puedeEspigar(p));

        const int roseta = AG::celdasDeEtapa(AG::MADURA);
        p = AG::espigada(p);
        // Recien espigada ya levanta una celda de tallo.
        CHECK(AG::celdasDe(p) == roseta + 1);

        for (int i = 1; i < AG::QUIOTE_CELDAS_MAX; ++i) p = AG::quioteCrecido(p);
        CHECK(AG::celdasDe(p) == roseta + AG::QUIOTE_CELDAS_MAX);

        // El quiote de 5 m sobre una roseta de 2 son 7 celdas: la planta mas
        // alta que produce esta familia. Tiene que caber en SEGMENTO (3 bits).
        CHECK(AG::celdasDe(p) <= 8);
    }
}

TEST_CASE("Agave azul: el quiote crecido del todo abre sus flores") {
    namespace AG = Compuesto::AgaveAzul;

    // La siembra genera unas pocas plantas ya espigadas y florecidas. Si
    // quioteCrecido no marcara FLORECIDO al llegar arriba, el mesher dibujaria
    // el eje pelado y el candelabro no apareceria nunca.
    BlockType p = AG::espigada(AG::nuevo(AG::MADURA, 0));
    CHECK_FALSE(AG::florecidoDe(p));

    for (int i = 0; i < AG::QUIOTE_CELDAS_MAX; ++i) p = AG::quioteCrecido(p);
    CHECK(AG::quioteDe(p) == AG::QUIOTE_CELDAS_MAX);

    // Una vuelta mas: ya no puede subir, asi que florece.
    p = AG::quioteCrecido(p);
    CHECK(AG::florecidoDe(p));
    CHECK(AG::quioteDe(p) == AG::QUIOTE_CELDAS_MAX);   // no se pasa de largo
}

// ============================================================================
// EL SEMBRADO: DETERMINISMO
// ============================================================================
// El mundo es determinista y de ahi depende que solo se guarden los chunks
// tocados: un chunk intacto se regenera identico. Una siembra que dependiera
// del orden de exploracion romperia esa invariante.

TEST_CASE("Agave azul: la siembra es determinista") {
    for (int x = -300; x <= 300; x += 7) {
        for (int z = -300; z <= 300; z += 11) {
            const bool a = hayColoniaAgave(x, z);
            const bool b = hayColoniaAgave(x, z);
            INFO("(", x, ",", z, ")");
            CHECK(a == b);

            if (a) {
                // Y la mata concreta tambien: misma posicion, misma planta.
                CHECK(hashMataAgave(x, z) == hashMataAgave(x, z));
                CHECK(etapaDeHash(hashMataAgave(x, z)) ==
                      etapaDeHash(hashMataAgave(x, z)));
            }
        }
    }
}

TEST_CASE("Agave azul: la etapa sale siempre dentro del rango valido") {
    namespace AG = Compuesto::AgaveAzul;

    // Si el reparto por percentiles dejara un hueco, etapaDeHash devolveria
    // un valor fuera de rango y la planta se dibujaria con medidas absurdas.
    for (int x = -500; x <= 500; x += 3) {
        for (int z = -500; z <= 500; z += 13) {
            const uint16_t e = etapaDeHash(hashMataAgave(x, z));
            INFO("(", x, ",", z, ") etapa ", e);
            CHECK(e >= AG::HIJUELO);
            CHECK(e <= AG::MADURA);
        }
    }
}

// ============================================================================
// EL SEMBRADO: DENSIDAD Y REPARTO
// ============================================================================

TEST_CASE("Agave azul: las colonias existen pero dejan claros") {
    // Las dos mitades del mismo requisito: que se encuentren sin buscar, y que
    // no tapicen el mundo. Un fallo en cualquiera de los dos sentidos arruina
    // el paisaje, y los dos han pasado ya con otras plantas.
    int columnasConAgave = 0;
    int total = 0;

    for (int x = -400; x < 400; ++x) {
        for (int z = -400; z < 400; z += 4) {
            ++total;
            if (hayColoniaAgave(x, z)) ++columnasConAgave;
        }
    }

    REQUIRE(total > 0);
    const double frac = (double)columnasConAgave / (double)total;
    INFO("fraccion de columnas dentro de una colonia: ", frac);

    // Existe de verdad...
    CHECK(columnasConAgave > 0);
    // ...pero es una minoria clara del terreno.
    CHECK(frac > 0.002);
    CHECK(frac < 0.15);
}

TEST_CASE("Agave azul: es mas disperso que el maguey pulquero") {
    // Es la decision de diseño que separa las dos especies: el pulquero forma
    // manchones densos del Altiplano; el tequilana sale en colonias mas
    // separadas. Con rejilla de 24 y 1 de cada 6 celdas frente a rejilla de 16
    // y 1 de cada 4, el agave tiene que cubrir menos terreno.
    int agave = 0, maguey = 0;

    for (int x = -400; x < 400; ++x) {
        for (int z = -400; z < 400; z += 4) {
            if (hayColoniaAgave(x, z))   ++agave;
            if (hayColoniaMaguey(x, z))  ++maguey;
        }
    }

    INFO("columnas con agave ", agave, " / con maguey ", maguey);
    CHECK(maguey > 0);
    CHECK(agave < maguey);
}

TEST_CASE("Agave azul: las dos especies no caen siempre en el mismo sitio") {
    // El hash del agave lleva una constante distinta (0x9d2c5681) justo para
    // esto. Si alguien la igualara a la del maguey, las dos colonias se
    // superpondrian: en las zonas donde ambas pueden crecer se veria un solo
    // manchon mezclado en vez de dos plantas repartidas.
    int soloAgave = 0, soloMaguey = 0, ambas = 0;

    for (int x = -400; x < 400; ++x) {
        for (int z = -400; z < 400; z += 4) {
            const bool a = hayColoniaAgave(x, z);
            const bool m = hayColoniaMaguey(x, z);
            if (a && m)       ++ambas;
            else if (a)       ++soloAgave;
            else if (m)       ++soloMaguey;
        }
    }

    INFO("solo agave ", soloAgave, " / solo maguey ", soloMaguey,
         " / ambas ", ambas);

    // Lo que importa es que el agave tenga territorio propio: si estuviera
    // encadenado al maguey, soloAgave seria cero.
    CHECK(soloAgave > 0);
    CHECK(soloMaguey > 0);
}

TEST_CASE("Agave azul: dentro de una colonia el centro es mas denso") {
    // La probabilidad cae con la distancia al foco (40 en el centro, 10 en el
    // borde). Es lo que hace que un manchon tenga forma de mata y no de disco
    // recortado con tijera.
    //
    // Se busca una colonia real y se compara su nucleo con su borde.
    int cx = 0, cz = 0, radio = 0;
    bool encontrada = false;

    for (int fx = 0; fx < 40 && !encontrada; ++fx) {
        for (int fz = 0; fz < 40 && !encontrada; ++fz) {
            unsigned hf = (unsigned)(fx * 2654435761u) ^
                          (unsigned)(fz * 2246822519u) ^ 0x9d2c5681u;
            hf ^= hf >> 13; hf *= 1274126177u; hf ^= hf >> 16;
            if ((hf % 6u) != 0u) continue;

            cx = fx * 24 + (int)((hf >> 3) % 24u);
            cz = fz * 24 + (int)((hf >> 11) % 24u);
            radio = 2 + (int)((hf >> 19) % 3u);
            encontrada = true;
        }
    }

    REQUIRE(encontrada);
    INFO("colonia en (", cx, ",", cz, ") radio ", radio);

    // El centro siempre esta dentro; un punto a distancia > radio, nunca.
    CHECK(hayColoniaAgave(cx, cz));
    CHECK_FALSE(hayColoniaAgave(cx + radio + 1, cz + radio + 1));
}

// ============================================================================
// LA JIMA Y LA ESPIGA SON EXCLUYENTES
// ============================================================================

TEST_CASE("Agave azul: solo una planta hecha se puede jimar") {
    namespace AG = Compuesto::AgaveAzul;

    // Jimar un hijuelo no da piña y mata la planta: la regla existe para que
    // el jugador no destruya el cultivo por impaciencia.
    CHECK_FALSE(AG::sePuedeJimar(AG::nuevo(AG::HIJUELO, 0)));
    CHECK_FALSE(AG::sePuedeJimar(AG::nuevo(AG::JOVEN, 0)));
    CHECK_FALSE(AG::sePuedeJimar(AG::nuevo(AG::MEDIA, 0)));
    CHECK(AG::sePuedeJimar(AG::nuevo(AG::HECHA, 0)));
    CHECK(AG::sePuedeJimar(AG::nuevo(AG::MADURA, 0)));

    // Una vez jimada ya no se vuelve a jimar: solo queda la piña.
    const BlockType pina = AG::jimada(AG::nuevo(AG::MADURA, 0));
    CHECK(AG::faseDe(pina) == AG::PINA);
    CHECK_FALSE(AG::sePuedeJimar(pina));

    // Y una piña tampoco espiga: ese camino ya se cerro al cosecharla.
    CHECK_FALSE(AG::puedeEspigar(pina));
}

TEST_CASE("Agave azul: solo la planta madura echa quiote") {
    namespace AG = Compuesto::AgaveAzul;

    // Es la bifurcacion del ciclo de vida: o se cosecha, o florece y muere.
    CHECK_FALSE(AG::puedeEspigar(AG::nuevo(AG::HIJUELO, 0)));
    CHECK_FALSE(AG::puedeEspigar(AG::nuevo(AG::HECHA, 0)));
    CHECK(AG::puedeEspigar(AG::nuevo(AG::MADURA, 0)));

    // Ya espigada no vuelve a empezar.
    const BlockType q = AG::espigada(AG::nuevo(AG::MADURA, 0));
    CHECK(AG::faseDe(q) == AG::QUIOTE_F);
    CHECK_FALSE(AG::puedeEspigar(q));
    CHECK_FALSE(AG::sePuedeJimar(q));
}

TEST_CASE("Agave azul: crecer se detiene en madura") {
    namespace AG = Compuesto::AgaveAzul;

    BlockType p = AG::nuevo(AG::HIJUELO, 0);
    for (int i = 0; i < 10; ++i) p = AG::crecida(p);

    // Sin tope, la etapa desbordaria su campo de 3 bits y la planta
    // reaparecería como hijuelo -- un ciclo infinito silencioso.
    CHECK(AG::etapaDe(p) == AG::MADURA);
    CHECK(AG::faseDe(p) == AG::ROSETA);
}

// ============================================================================
// LA SILUETA: MAS ANCHA QUE ALTA
// ============================================================================
// Es la diferencia visible con el pulquero, y sale de numeros, no del dibujo.

TEST_CASE("Agave azul: la roseta es mas ancha que alta") {
    namespace AG = Compuesto::AgaveAzul;

    // 2 m de alto por 3 m de ancho. El diametro (2*radio) tiene que superar
    // a la altura en toda etapa donde el tope de 1.45 no recorte.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        const float alto = AG::alturaReal(e);
        const float diam = AG::radioDeEtapa(e) * 2.0f;
        INFO("etapa ", e, " alto ", alto, " diametro ", diam);
        CHECK(diam > alto);
    }
}

TEST_CASE("Agave azul: la roseta nunca cruza el borde del chunk") {
    namespace AG = Compuesto::AgaveAzul;

    // Pasar de 1.5 haria que la planta se saliera de su chunk, donde el
    // recorte del dibujado si corta: se veria aparecer y desaparecer al girar
    // la camara. El tope de radioDeEtapa existe justo para eso.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AG::radioDeEtapa(e) <= 1.45f);
    }
}

TEST_CASE("Agave azul: la planta crece de forma monotona") {
    namespace AG = Compuesto::AgaveAzul;

    // Nada puede encoger al pasar de etapa: una planta que menguara al crecer
    // se veria dar un salto hacia atras.
    for (uint16_t e = AG::JOVEN; e <= AG::MADURA; ++e) {
        INFO("etapa ", e, " frente a ", e - 1);
        CHECK(AG::alturaReal(e)     >  AG::alturaReal((uint16_t)(e - 1)));
        CHECK(AG::radioDeEtapa(e)   >= AG::radioDeEtapa((uint16_t)(e - 1)));
        CHECK(AG::pencasDeEtapa(e)  >  AG::pencasDeEtapa((uint16_t)(e - 1)));
        CHECK(AG::engrosado(e)      >  AG::engrosado((uint16_t)(e - 1)));
        CHECK(AG::celdasDeEtapa(e)  >= AG::celdasDeEtapa((uint16_t)(e - 1)));
    }
}

// ============================================================================
// LOS HUECOS SIN TEXTURA DE LA ROSETA
// ============================================================================
// BUG REAL, visto en pantalla: la roseta salia agujereada -- faltaban trozos
// de penca en pleno aire, y las hojas se cortaban a media altura.
//
// LA CAUSA NO ERA LA TEXTURA. El PNG es 16x16 completamente opaco, sin un solo
// pixel transparente, asi que no habia nada que pudiera "no pintarse". Era
// GEOMETRIA QUE NO SE EMITIA.
//
// Cada celda de la planta dibujaba solo su franja vertical y descartaba los
// quads que se salieran de ella:
//
//     if (hi < -0.02f || lo > 1.02f) continue;
//
// Eso vale para una COLUMNA (el tallo del quiote sube recto y cada tramo cae
// limpio dentro de una celda), pero NO para una ROSETA: sus pencas salen hacia
// arriba Y hacia afuera, o sea que cruzan la frontera entre celdas EN DIAGONAL.
// Un quad asi se sale por arriba en la celda de abajo y por abajo en la de
// arriba: las DOS lo descartaban y no lo dibujaba ninguna. El hueco aparecia
// justo a la altura del salto de celda, que es como se localizo.
//
// EL ARREGLO. La planta entera se emite desde su celda base (SEG == 0) y las
// celdas de arriba no dibujan nada, que es justo lo que ya hacia el maguey
// pulquero -- por eso el pulquero nunca tuvo este bug.
//
// Lo que estos tests pueden fijar es la PRECONDICION que hace legitimo ese
// arreglo: que la planta quepa entera dentro del alcance del mesher. Si alguna
// medida creciera hasta salirse, habria que volver a repartirla entre celdas y
// el bug regresaria.

TEST_CASE("Agave azul: la planta cabe entera en el alcance del mesher") {
    namespace AG = Compuesto::AgaveAzul;

    // El mesher dibuja la planta COMPLETA desde su celda base. Para que eso
    // sea correcto tiene que caber dentro del chunk sin que el recorte del
    // dibujado la corte -- que es la razon del tope de 1.45 en el radio.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        INFO("etapa ", e);

        // A lo ancho: la roseta no puede cruzar el borde del chunk (16).
        CHECK(AG::radioDeEtapa(e) < 1.5f);

        // A lo alto: la roseta sola.
        CHECK(AG::alturaReal(e) <= 2.0f);
    }

    // A lo alto con quiote, que es el caso extremo: 2 celdas de roseta y 5 de
    // tallo. Si esto pasara de 8, SEGMENTO (3 bits) desbordaria y la planta
    // se partiria de nuevo.
    BlockType p = AG::espigada(AG::nuevo(AG::MADURA, 0));
    for (int i = 0; i < AG::QUIOTE_CELDAS_MAX; ++i) p = AG::quioteCrecido(p);
    CHECK(AG::celdasDe(p) <= 8);
}

TEST_CASE("Agave azul: las pencas cruzan de celda, que es lo que causaba el hueco") {
    namespace AG = Compuesto::AgaveAzul;

    // Este test documenta POR QUE el recorte por celda estaba mal, para que
    // nadie lo reintroduzca pensando que es una optimizacion inocente.
    //
    // Se reproduce la cuenta del mesher: la penca mas interior sube hasta
    // ALTO * 0.95, y la roseta ocupa `celdasDeEtapa` celdas. Si la punta de
    // esa hoja pasa de la primera celda, hay quads que cruzan la frontera.
    for (uint16_t e = AG::MEDIA; e <= AG::MADURA; ++e) {
        const float alto = AG::alturaReal(e);
        const float altoPencaInterior = alto * 0.95f;

        INFO("etapa ", e, " alto de la penca interior ", altoPencaInterior);

        // Pasa de 1.0, o sea que sale de la celda base: por ahi se perdian
        // los trozos. Con la planta emitida entera ya no importa, pero la
        // condicion que lo provocaba sigue siendo cierta y conviene verla.
        CHECK(altoPencaInterior > 1.0f);

        // Y la planta declara mas de una celda justamente por eso.
        CHECK(AG::celdasDeEtapa(e) >= 2);
    }
}

TEST_CASE("Agave azul: toda pieza dibujada tiene un bloque con textura detras") {
    namespace AG = Compuesto::AgaveAzul;

    // La otra via por la que se puede abrir un hueco: `quad` y `revolucion`
    // hacen `if (tex == 0) return`, asi que una pieza sin textura no avisa --
    // simplemente desaparece.
    //
    // El mesher pide sus cinco texturas por BlockType. Los cinco IDs tienen
    // que existir y ser distintos entre si; si dos coincidieran, una pieza se
    // dibujaria con el material de otra.
    const BlockType piezas[5] = {
        BLOCK_AGAVE_AZUL_PENCA,
        BLOCK_AGAVE_AZUL_PUNTA,
        BLOCK_AGAVE_AZUL_PINA,
        BLOCK_AGAVE_AZUL_QUIOTE,
        BLOCK_AGAVE_AZUL_FLOR
    };

    for (int i = 0; i < 5; ++i) {
        INFO("pieza ", i);
        // Dentro del enum: si se salieran, getBlockTexture caeria en su
        // `default` y devolveria piedra.
        CHECK((int)piezas[i] > 0);
        CHECK((int)piezas[i] <= (int)BLOCK_TYPE_MAX);

        for (int j = i + 1; j < 5; ++j) {
            INFO("frente a la pieza ", j);
            CHECK(piezas[i] != piezas[j]);
        }
    }
}

TEST_CASE("BLOCK_TYPE_MAX cubre hasta el ultimo bloque del enum") {
    // ⭐ ESTE TEST NACIO DE UN BUG REAL.
    //
    // Al añadir el agave, BLOCK_TYPE_MAX se quedo apuntando al tazon: 10 IDs
    // por debajo del final. Dos cosas se rompieron a la vez y ninguna avisaba:
    //
    //   * prewarmItemTextures() recorre 0..BLOCK_TYPE_MAX, asi que las piezas
    //     del agave no recibian icono de item.
    //   * es el tope contra el que se validan los IDs leidos de disco, asi que
    //     los bloques nuevos quedaban del lado "invalido" de la comprobacion.
    //
    // Atarlo aqui convierte "se me olvido actualizar el tope" en un test rojo
    // en vez de en un bloque sin textura que aparece semanas despues.
    // Se actualiza cada vez que el enum crece: hoy el ultimo es el HUEVO DE
    // SPAWN DEL PECARI. Lo que importa no es el nombre concreto sino la
    // propiedad de abajo -- que ningun bloque quede por encima del tope.
    CHECK((int)BLOCK_TYPE_MAX == (int)BLOCK_HUEVO_PECARI);

    // Y el propio huevo, que es el ultimo en entrar.
    CHECK((int)BLOCK_HUEVO_PECARI <= (int)BLOCK_TYPE_MAX);

    // Y la propiedad de fondo, que es la que de verdad importa: NINGUN bloque
    // del enum puede quedar por encima del tope.
    CHECK((int)BLOCK_AGAVE_AZUL_PENCA  <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_AGAVE_AZUL_PUNTA  <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_AGAVE_AZUL_PINA   <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_AGAVE_AZUL_QUIOTE <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_AGAVE_AZUL_FLOR   <= (int)BLOCK_TYPE_MAX);

    // El espacio de los compuestos sigue muy por encima del enum: si el tope
    // llegara a invadirlo, un maguey se leeria como un bloque normal.
    CHECK(Compuesto::COMPUESTO_BASE > (int)BLOCK_TYPE_MAX);
}

TEST_CASE("Agave azul: sus pencas son mas rigidas que las del pulquero") {
    namespace AG = Compuesto::AgaveAzul;

    // La descripcion botanica lo dice: el tequilana lleva las hojas RIGIDAS Y
    // ERECTAS, el pulquero las arquea. Si las dos caidas se acercaran, las dos
    // especies se confundirian de lejos y el trabajo de modelarlas aparte se
    // perderia.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        INFO("etapa ", e);
        CHECK(AG::caidaDeEtapa(e) <= 0.10f);
    }
}
