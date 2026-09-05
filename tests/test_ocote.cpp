#include <doctest/doctest.h>
#include "BlockType.h"
#include "FisicaCaida.h"
#include "AciculaOcote.h"
#include <cmath>

// ============================================================================
// EL OCOTE (Pinus montezumae): UNA ESPECIE NUEVA COMPLETA
// ============================================================================
// Añadir un arbol al motor no es declarar un ID: es darlo de alta en una
// docena de sitios repartidos por el codigo. Si falta uno, el fallo no es un
// crash sino algo peor -- silencioso y raro:
//
//   sin esOrganicoParaHacha   -> el arbol NO SE PUEDE TALAR y el hacha no se gasta
//   sin esTerrenoNatural      -> el arbol NO CAE al cortarlo (el tronco sujeta)
//   sin getBlockTexture       -> sale con TEXTURA DE PIEDRA (cae en el default)
//   sin GetTreeType           -> no se genera NUNCA aunque exista el generador
//   sin BLOCK_TYPE_MAX        -> el item sale SIN TEXTURA en el inventario
//   sin esSueloParaNopal      -> crecen NOPALES encima del tronco
//
// Estos tests cubren los que son logica pura y se pueden comprobar sin
// arrancar OpenGL. Los que dependen del render (texturas) o del mundo
// (generacion) se verifican compilando y jugando, y quedan anotados abajo.

// ============================================================================
// LOS IDs EXISTEN Y NO PISAN A NADIE
// ============================================================================

TEST_CASE("Ocote: sus cinco bloques son IDs validos y distintos") {
    const BlockType piezas[5] = {
        BLOCK_WOOD_OCOTE, BLOCK_WOOD_OCOTE_DENTRO,
        BLOCK_LEAVES_OCOTE, BLOCK_PLANKS_OCOTE, BLOCK_RAMA_OCOTE
    };

    for (int i = 0; i < 5; ++i) {
        INFO("pieza ", i);
        CHECK((int)piezas[i] > 0);

        // ⭐ POR DEBAJO DEL TOPE. prewarmItemTextures recorre 0..BLOCK_TYPE_MAX
        // para registrar los iconos de item: un bloque por encima saldria SIN
        // TEXTURA en la mano y en el inventario. Es el fallo que ya ocurrio con
        // las piezas del agave azul.
        CHECK((int)piezas[i] <= BLOCK_TYPE_MAX);

        for (int j = i + 1; j < 5; ++j) {
            INFO("frente a la pieza ", j);
            CHECK(piezas[i] != piezas[j]);
        }
    }
}

TEST_CASE("Ocote: no colisiona con las otras especies de arbol") {
    // Las cuatro especies tienen que ser distinguibles: si dos IDs coincidieran,
    // un tronco de ocote se leeria como oyamel al cargar el mundo.
    const BlockType troncos[4] = { BLOCK_WOOD, BLOCK_WOOD_ENCINO,
                                   BLOCK_WOOD_OYAMEL, BLOCK_WOOD_OCOTE };
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            INFO("troncos ", i, " y ", j);
            CHECK(troncos[i] != troncos[j]);
        }

    const BlockType hojas[4] = { BLOCK_LEAVES, BLOCK_LEAVES_ENCINO,
                                 BLOCK_LEAVES_OYAMEL, BLOCK_LEAVES_OCOTE };
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            INFO("hojas ", i, " y ", j);
            CHECK(hojas[i] != hojas[j]);
        }
}

// ============================================================================
// SE PUEDE TALAR
// ============================================================================

TEST_CASE("Ocote: el hacha puede con su madera") {
    // Sin esto el arbol es INDESTRUCTIBLE a efectos practicos: el hacha no lo
    // reconoce como organico, asi que ni corta rapido ni se gasta.
    CHECK(esOrganicoParaHacha(BLOCK_WOOD_OCOTE));
    CHECK(esOrganicoParaHacha(BLOCK_WOOD_OCOTE_DENTRO));
    CHECK(esOrganicoParaHacha(BLOCK_RAMA_OCOTE));

    // Y el hacha se gasta con ella, como con las demas maderas.
    CHECK(desgasteHacha(BLOCK_WOOD_OCOTE) > 0);
    CHECK(desgasteHacha(BLOCK_RAMA_OCOTE) > 0);
}

TEST_CASE("Ocote: el pico no se gasta con sus aciculas") {
    // Las hojas no son roca. Si lo fueran, picarlas mellaria el pico.
    CHECK_FALSE(esRocaParaPico(BLOCK_LEAVES_OCOTE));
}

// ============================================================================
// SE DERRUMBA AL TALARLO
// ============================================================================

TEST_CASE("Ocote: su tronco y sus hojas no sujetan el terreno") {
    namespace F = Fisica;

    // ⭐ ESTE ES EL SITIO QUE MAS FACIL SE OLVIDA.
    //
    // esTerrenoNatural() decide que bloques SUJETAN lo que tienen encima. Si el
    // tronco de una especie se quedara fuera, el motor lo trataria como suelo
    // firme: al talar la base, el resto del arbol se quedaria CLAVADO EN EL
    // AIRE en vez de venirse abajo.
    CHECK_FALSE(F::esTerrenoNatural(BLOCK_WOOD_OCOTE));
    CHECK_FALSE(F::esTerrenoNatural(BLOCK_WOOD_OCOTE_DENTRO));
    CHECK_FALSE(F::esTerrenoNatural(BLOCK_LEAVES_OCOTE));

    // Los tablones tampoco son terreno: son construccion, y una construccion
    // sin pilares se cae.
    CHECK_FALSE(F::esTerrenoNatural(BLOCK_PLANKS_OCOTE));
}

TEST_CASE("Ocote: se comporta igual que las otras especies al caer") {
    namespace F = Fisica;

    // La invariante de familia: ninguna madera de ningun arbol sujeta.
    const BlockType maderas[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_WOOD_OCOTE, BLOCK_WOOD_OCOTE_DENTRO
    };
    for (BlockType m : maderas) {
        INFO("madera ", (int)m);
        CHECK_FALSE(F::esTerrenoNatural(m));
    }
}

// ============================================================================
// SU DENSIDAD ES LA QUE TOCA
// ============================================================================

TEST_CASE("Ocote: pesa mas que el pino y menos que el encino") {
    namespace F = Fisica;

    // Pinus montezumae es mas denso que el pino comun porque su madera va
    // cargada de RESINA -- es la que se usa como tea justamente por eso. Queda
    // entre las coniferas ligeras y el roble.
    const float pino   = F::densidadDe(BLOCK_WOOD);
    const float oyamel = F::densidadDe(BLOCK_WOOD_OYAMEL);
    const float ocote  = F::densidadDe(BLOCK_WOOD_OCOTE);
    const float encino = F::densidadDe(BLOCK_WOOD_ENCINO);

    INFO("pino ", pino, " oyamel ", oyamel, " ocote ", ocote, " encino ", encino);
    CHECK(ocote > pino);
    CHECK(ocote > oyamel);
    CHECK(ocote < encino);

    // El corte y los tablones pesan lo mismo que el tronco: es la misma madera.
    CHECK(F::densidadDe(BLOCK_WOOD_OCOTE_DENTRO) == doctest::Approx(ocote));
    CHECK(F::densidadDe(BLOCK_PLANKS_OCOTE)      == doctest::Approx(ocote));
}

TEST_CASE("Ocote: sus aciculas pesan lo mismo que cualquier hoja") {
    namespace F = Fisica;
    // Si una especie tuviera hojas con otra densidad, su copa caeria a otra
    // velocidad que las demas sin ningun motivo.
    CHECK(F::densidadDe(BLOCK_LEAVES_OCOTE) ==
          doctest::Approx(F::densidadDe(BLOCK_LEAVES)));
}

// ============================================================================
// NO ROMPE OTRAS REGLAS DEL MOTOR
// ============================================================================

TEST_CASE("Ocote: su madera no admite capas parciales") {
    // La madera no tiene niveles en este motor -- son los 6 huecos reservados
    // de la tabla. El ocote no se añadio a esa tabla A PROPOSITO: su tamaño
    // entra en la aritmetica de las celdas mixtas, y tocarlo reinterpretaria
    // los IDs mixtos ya guardados en disco.
    //
    // Al no estar en la tabla, admiteNiveles() devuelve false por defecto, que
    // es justo el comportamiento que queremos.
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD_OCOTE));
    CHECK_FALSE(admiteNiveles(BLOCK_PLANKS_OCOTE));
}

// ============================================================================
// LOS MECHONES DE ACICULAS (el follaje 2.5D)
// ============================================================================
// La copa no es un cubo con textura de hoja: cada celda emite VARIOS sprites
// orientados en todas direcciones. Lo que se prueba aqui es lo que puede
// romperse en silencio -- un buffer corto, una direccion degenerada, un patron
// visible por falta de variacion.

TEST_CASE("Aciculas: nunca se escriben mas mechones de los que caben") {
    // ⭐ ESTE ES EL TEST QUE PROTEGE DE UN DESBORDAMIENTO.
    //
    // El mesher reserva su array con MAX_MECHONES. Si alguien sube la densidad
    // en la tabla de especies por encima de ese tope, MechonesDe escribiria
    // fuera del buffer: corrupcion de memoria silenciosa dentro del mesher, que
    // es de los sitios mas dificiles de diagnosticar.
    //
    // Se barren muchas celdas porque el numero VARIA por posicion (+-2).
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    const BlockType especies[2] = { BLOCK_LEAVES_OCOTE, BLOCK_LEAVES_OCOTE_CHINO };

    for (BlockType sp : especies) {
        for (int x = -40; x <= 40; x += 7)
            for (int y = 0; y <= 120; y += 11)
                for (int z = -40; z <= 40; z += 7) {
                    const int n = Acicula::MechonesDe(sp, x, y, z,
                                                      buf, Acicula::MAX_MECHONES);
                    INFO("especie ", (int)sp, " en ", x, ",", y, ",", z);
                    CHECK(n >= 3);
                    CHECK(n <= Acicula::MAX_MECHONES);
                }
    }
}

TEST_CASE("Aciculas: el tope del buffer cubre la densidad de las dos especies") {
    // La comprobacion estatica del mismo riesgo: que el tope siga por encima
    // del maximo que la tabla puede pedir (densidad declarada + 2 de variacion).
    const Acicula::Especie blanco = Acicula::EspecieDe(BLOCK_LEAVES_OCOTE);
    const Acicula::Especie chino  = Acicula::EspecieDe(BLOCK_LEAVES_OCOTE_CHINO);

    INFO("blanco ", blanco.mechones, " chino ", chino.mechones,
         " tope ", Acicula::MAX_MECHONES);
    CHECK(blanco.mechones + 2 <= Acicula::MAX_MECHONES);
    CHECK(chino.mechones  + 2 <= Acicula::MAX_MECHONES);
}

TEST_CASE("Aciculas: son muchas, que es lo que hace que se lean como follaje") {
    // Con pocos mechones por celda la copa se ve como sprites CONTABLES en vez
    // de masa de aguja. Este numero es el que decide eso, asi que se fija: si
    // alguien lo baja, el aspecto se degrada y conviene que sea deliberado.
    CHECK(Acicula::EspecieDe(BLOCK_LEAVES_OCOTE).mechones >= 12);
    CHECK(Acicula::EspecieDe(BLOCK_LEAVES_OCOTE_CHINO).mechones >= 12);

    // El chino es la especie de manojo mas apretado: mas mechones que el blanco.
    CHECK(Acicula::EspecieDe(BLOCK_LEAVES_OCOTE_CHINO).mechones >=
          Acicula::EspecieDe(BLOCK_LEAVES_OCOTE).mechones);
}

TEST_CASE("Aciculas: la direccion de cada mechon es unitaria") {
    // El mesher construye los dos ejes del sprite con productos vectoriales
    // sobre esta direccion. Si no fuera unitaria, el ancho del quad saldria
    // escalado por un factor arbitrario y los mechones tendrian tamanos que
    // nadie pidio.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    for (int x = -20; x <= 20; x += 3)
        for (int z = -20; z <= 20; z += 3) {
            const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, x, 64, z,
                                              buf, Acicula::MAX_MECHONES);
            for (int i = 0; i < n; ++i) {
                const float len = std::sqrt(buf[i].dx*buf[i].dx +
                                            buf[i].dy*buf[i].dy +
                                            buf[i].dz*buf[i].dz);
                INFO("mechon ", i, " en ", x, ",", z, " largo de dir ", len);
                CHECK(len == doctest::Approx(1.0f).epsilon(0.01f));
            }
        }
}

TEST_CASE("Aciculas: el ladeo esta centrado en cero y tiene los dos signos") {
    // ⭐ EL LADEO TIENE QUE SER SIMETRICO.
    //
    // Es lo que inclina cada sprite sobre su propio eje. Si el rango solo
    // fuera positivo, TODOS los mechones se torcerian hacia el mismo lado y la
    // copa quedaria "peinada" -- que es otra version del patron visible que el
    // ladeo viene a romper.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    int positivos = 0, negativos = 0;
    float suma = 0.0f;
    int total = 0;

    const float tope = Acicula::EspecieDe(BLOCK_LEAVES_OCOTE).ladeo;

    for (int x = -30; x <= 30; x += 3)
        for (int z = -30; z <= 30; z += 3) {
            const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, x, 70, z,
                                              buf, Acicula::MAX_MECHONES);
            for (int i = 0; i < n; ++i) {
                const float L = buf[i].ladeo;
                // Dentro del rango declarado por la especie.
                INFO("ladeo ", L, " tope ", tope);
                CHECK(std::fabs(L) <= tope + 1e-4f);

                if (L > 0.0f) ++positivos;
                if (L < 0.0f) ++negativos;
                suma += L;
                ++total;
            }
        }

    REQUIRE(total > 100);
    // Los dos signos aparecen, y en proporciones parecidas.
    CHECK(positivos > total / 4);
    CHECK(negativos > total / 4);
    // La media ronda cero: no hay sesgo hacia un lado.
    CHECK(std::fabs(suma / (float)total) < tope * 0.15f);
}

TEST_CASE("Aciculas: dos celdas vecinas no salen calcadas") {
    // La variacion sale del hash de la POSICION, no de un RNG con estado. Eso
    // da dos garantias a la vez: la misma celda se ve siempre igual (no hace
    // falta guardar nada) y dos celdas contiguas se ven distintas (la copa no
    // repite un patron).
    Acicula::Mechon a[Acicula::MAX_MECHONES];
    Acicula::Mechon b[Acicula::MAX_MECHONES];

    const int na = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 10, 64, 10,
                                       a, Acicula::MAX_MECHONES);
    const int nb = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 11, 64, 10,
                                       b, Acicula::MAX_MECHONES);

    bool alguna_diferencia = false;
    const int n = (na < nb) ? na : nb;
    for (int i = 0; i < n; ++i) {
        if (a[i].dx != b[i].dx || a[i].ladeo != b[i].ladeo ||
            a[i].ox != b[i].ox) {
            alguna_diferencia = true;
            break;
        }
    }
    CHECK((alguna_diferencia || na != nb));
}

TEST_CASE("Aciculas: la misma celda da siempre el mismo resultado") {
    // Determinismo: es lo que permite que el follaje no se guarde en disco. Si
    // esto fallara, un chunk se veria distinto cada vez que se remalla.
    Acicula::Mechon a[Acicula::MAX_MECHONES];
    Acicula::Mechon b[Acicula::MAX_MECHONES];

    const int na = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, -7, 88, 23,
                                       a, Acicula::MAX_MECHONES);
    const int nb = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, -7, 88, 23,
                                       b, Acicula::MAX_MECHONES);

    REQUIRE(na == nb);
    for (int i = 0; i < na; ++i) {
        INFO("mechon ", i);
        CHECK(a[i].dx    == b[i].dx);
        CHECK(a[i].dy    == b[i].dy);
        CHECK(a[i].dz    == b[i].dz);
        CHECK(a[i].ladeo == b[i].ladeo);
        CHECK(a[i].largo == b[i].largo);
    }
}

TEST_CASE("Aciculas: el mechon nace dentro del voxel") {
    // El origen se dispersa alrededor del centro. Si se saliera de [0,1] el
    // sprite arrancaria en la celda de al lado y el follaje se veria descosido
    // en las fronteras.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    for (int x = -15; x <= 15; x += 5)
        for (int z = -15; z <= 15; z += 5) {
            const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, x, 64, z,
                                              buf, Acicula::MAX_MECHONES);
            for (int i = 0; i < n; ++i) {
                INFO("origen ", buf[i].ox, ",", buf[i].oy, ",", buf[i].oz);
                CHECK(buf[i].ox >= 0.0f); CHECK(buf[i].ox <= 1.0f);
                CHECK(buf[i].oy >= 0.0f); CHECK(buf[i].oy <= 1.0f);
                CHECK(buf[i].oz >= 0.0f); CHECK(buf[i].oz <= 1.0f);
            }
        }
}

TEST_CASE("Aciculas: cuelgan, no se erizan") {
    // Miden hasta 35 cm y son muy flexibles: se vencen por su propio peso. El
    // sesgo hacia abajo es lo que distingue un pino de un erizo, asi que la
    // media de la componente vertical tiene que salir NEGATIVA.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    float sumaY = 0.0f;
    int total = 0;
    for (int x = -20; x <= 20; x += 4)
        for (int z = -20; z <= 20; z += 4) {
            const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, x, 64, z,
                                              buf, Acicula::MAX_MECHONES);
            for (int i = 0; i < n; ++i) { sumaY += buf[i].dy; ++total; }
        }

    REQUIRE(total > 50);
    INFO("media de dy ", sumaY / (float)total);
    CHECK(sumaY / (float)total < 0.0f);
}

TEST_CASE("Aciculas: las cuatro variantes de bloque se reconocen") {
    // Las dos especies, cada una con su celda suelta y su celda compartida con
    // la rama. Si una se quedara fuera, esa celda se dibujaria como CUBO OPACO
    // en medio de la copa.
    CHECK(Acicula::esAciculaOcote(BLOCK_LEAVES_OCOTE));
    CHECK(Acicula::esAciculaOcote(BLOCK_LEAVES_OCOTE_RAMA));
    CHECK(Acicula::esAciculaOcote(BLOCK_LEAVES_OCOTE_CHINO));
    CHECK(Acicula::esAciculaOcote(BLOCK_LEAVES_OCOTE_CHINO_RAMA));

    // Y no se confunde con la hoja de otra especie.
    CHECK_FALSE(Acicula::esAciculaOcote(BLOCK_LEAVES));
    CHECK_FALSE(Acicula::esAciculaOcote(BLOCK_LEAVES_OYAMEL));
    CHECK_FALSE(Acicula::esAciculaOcote(BLOCK_WOOD_OCOTE));

    // La distincion de especie, que decide densidad y largo del mechon.
    CHECK(Acicula::esChino(BLOCK_LEAVES_OCOTE_CHINO));
    CHECK(Acicula::esChino(BLOCK_LEAVES_OCOTE_CHINO_RAMA));
    CHECK_FALSE(Acicula::esChino(BLOCK_LEAVES_OCOTE));
}

TEST_CASE("Ocote: sigue siendo compatible con el formato de guardado") {
    // Los IDs nuevos van al FINAL del enum. Insertar en medio correria los
    // valores y un mundo guardado leeria otro bloque en su lugar.
    //
    // Se comprueba que el ocote queda por encima de todo lo que existia antes,
    // y muy por debajo del espacio de los bloques compuestos.
    CHECK((int)BLOCK_WOOD_OCOTE > (int)BLOCK_AGAVE_AZUL_FLOR);

    // Lo que importa no es que el ocote sea el ULTIMO -- el enum sigue
    // creciendo y eso lo fija test_agave_azul.cpp -- sino que quede DENTRO del
    // tope. Por encima de BLOCK_TYPE_MAX el item sale sin textura y el
    // deserializador lo da por invalido.
    CHECK((int)BLOCK_RAMA_OCOTE <= BLOCK_TYPE_MAX);
    CHECK(BLOCK_TYPE_MAX < BLOQUE_COMPUESTO_BASE_ID);
}
