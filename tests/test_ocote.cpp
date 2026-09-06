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

// ============================================================================
// LA FISICA DE LA HOJA
// ============================================================================
// Una acicula es una viga empotrada por la base: la punta se mueve, el
// arranque no. Lo que sigue fija esa invariante y las que la acompanan.

TEST_CASE("Fisica de hoja: la base NO se mueve, pase lo que pase") {
    // ⭐ ESTA ES LA INVARIANTE PRINCIPAL DEL EFECTO.
    //
    // El vertice pegado a la rama tiene que quedarse EXACTAMENTE donde estaba,
    // aunque el jugador este encima. Si se moviera, el mechon se despegaria de
    // su rama y se veria flotar -- que es justo lo que distingue una hoja bien
    // anclada de un sprite suelto.
    float dx, dy, dz;

    // El caso mas exigente: el jugador clavado en el punto de la hoja.
    Acicula::DesplazarHoja(10.0f, 70.0f, 10.0f,
                           /*t01=*/0.0f, /*fase=*/1.3f, /*tiempo=*/7.5f,
                           10.0f, 70.0f, 10.0f,
                           dx, dy, dz);
    CHECK(dx == 0.0f);
    CHECK(dy == 0.0f);
    CHECK(dz == 0.0f);

    // Y en un barrido de tiempos y posiciones del jugador.
    for (float t = 0.0f; t < 12.0f; t += 0.7f)
        for (float ox = -2.0f; ox <= 2.0f; ox += 0.5f) {
            Acicula::DesplazarHoja(10.0f, 70.0f, 10.0f, 0.0f, 2.0f, t,
                                   10.0f + ox, 70.0f, 10.0f, dx, dy, dz);
            INFO("t=", t, " offset=", ox);
            CHECK(dx == 0.0f);
            CHECK(dy == 0.0f);
            CHECK(dz == 0.0f);
        }
}

TEST_CASE("Fisica de hoja: la punta se mueve mucho mas que el medio") {
    // El perfil de viga en voladizo es CUADRATICO, no lineal: a media hoja el
    // desplazamiento tiene que ser ~1/4 del de la punta, no 1/2. Eso es lo que
    // hace que la hoja se vea DOBLARSE en arco en vez de trasladarse rigida.
    float px, py, pz, mx, my, mz;

    // Jugador cerca, para que domine el empuje sobre la brisa.
    Acicula::DesplazarHoja(10.5f, 70.0f, 10.0f, 1.0f, 0.0f, 3.0f,
                           10.0f, 70.0f, 10.0f, px, py, pz);
    Acicula::DesplazarHoja(10.5f, 70.0f, 10.0f, 0.5f, 0.0f, 3.0f,
                           10.0f, 70.0f, 10.0f, mx, my, mz);

    const float punta = std::sqrt(px*px + py*py + pz*pz);
    const float medio = std::sqrt(mx*mx + my*my + mz*mz);

    INFO("punta ", punta, " medio ", medio);
    REQUIRE(punta > 1e-4f);
    CHECK(medio < punta);
    // La razon teorica es 0.5^2 = 0.25. Se deja holgura por la brisa, que no
    // depende del empuje.
    CHECK(medio / punta < 0.45f);
}

TEST_CASE("Fisica de hoja: el jugador la aparta ALEJANDOLA de si") {
    // El empuje es radial y saliente: una hoja al este del jugador se va mas
    // al este. Si el signo estuviera invertido, las hojas se meterian DENTRO
    // del jugador, que es exactamente lo contrario de lo que se ve al andar
    // entre ramas.
    float dx, dy, dz;

    // Hoja al ESTE del jugador (x mayor): debe empujarse hacia +x.
    Acicula::DesplazarHoja(10.6f, 70.0f, 10.0f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    CHECK(dx > 0.0f);

    // Hoja al OESTE: hacia -x.
    Acicula::DesplazarHoja(9.4f, 70.0f, 10.0f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    CHECK(dx < 0.0f);

    // Hoja al SUR (z mayor): hacia +z.
    Acicula::DesplazarHoja(10.0f, 70.0f, 10.6f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    CHECK(dz > 0.0f);
}

TEST_CASE("Fisica de hoja: fuera del radio solo queda la brisa") {
    // El empuje tiene alcance limitado. Una hoja lejos del jugador se mueve
    // SOLO por la brisa, que es un orden de magnitud mas pequena. Sin este
    // corte, cruzar un bosque agitaria copas a decenas de bloques.
    float lejos[3], cerca[3];

    Acicula::DesplazarHoja(40.0f, 70.0f, 40.0f, 1.0f, 0.0f, 2.0f,
                           10.0f, 70.0f, 10.0f, lejos[0], lejos[1], lejos[2]);
    Acicula::DesplazarHoja(10.4f, 70.0f, 10.0f, 1.0f, 0.0f, 2.0f,
                           10.0f, 70.0f, 10.0f, cerca[0], cerca[1], cerca[2]);

    const float dLejos = std::sqrt(lejos[0]*lejos[0] + lejos[1]*lejos[1] +
                                   lejos[2]*lejos[2]);
    const float dCerca = std::sqrt(cerca[0]*cerca[0] + cerca[1]*cerca[1] +
                                   cerca[2]*cerca[2]);

    INFO("lejos ", dLejos, " cerca ", dCerca);
    CHECK(dLejos < Acicula::BRISA * 3.0f);   // solo brisa
    CHECK(dCerca > dLejos * 2.0f);           // el empuje domina de cerca
}

TEST_CASE("Fisica de hoja: el desplazamiento nunca se dispara") {
    // Una cota dura. Si un caso limite --el jugador en el mismo punto, una
    // division por una distancia diminuta-- produjera un valor enorme, la hoja
    // saldria disparada al infinito y se veria un triangulo cruzando la
    // pantalla. Es el fallo tipico de un empuje radial mal acotado.
    float dx, dy, dz;
    const float tope = Acicula::EMPUJE_MAX + Acicula::BRISA * 4.0f;

    for (float ox = -0.05f; ox <= 0.05f; ox += 0.01f)
        for (float oz = -0.05f; oz <= 0.05f; oz += 0.01f)
            for (float t = 0.0f; t < 6.0f; t += 0.9f) {
                Acicula::DesplazarHoja(10.0f + ox, 70.0f, 10.0f + oz,
                                       1.0f, 0.7f, t,
                                       10.0f, 70.0f, 10.0f, dx, dy, dz);
                const float d = std::sqrt(dx*dx + dy*dy + dz*dz);
                INFO("offset ", ox, ",", oz, " t=", t, " -> ", d);
                CHECK(std::isfinite(d));
                CHECK(d <= tope);
            }
}

TEST_CASE("Fisica de hoja: no reacciona a un jugador muy por encima o debajo") {
    // El empuje se limita al alto del cuerpo. Pasar por debajo de una copa no
    // puede agitar hojas que estan diez bloques mas arriba.
    float dx, dy, dz;

    // Jugador 10 bloques por debajo de la hoja.
    Acicula::DesplazarHoja(10.0f, 80.0f, 10.0f, 1.0f, 0.0f, 1.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    float d = std::sqrt(dx*dx + dy*dy + dz*dz);
    INFO("por debajo -> ", d);
    CHECK(d < Acicula::BRISA * 3.0f);   // solo brisa

    // Jugador 10 bloques por encima.
    Acicula::DesplazarHoja(10.0f, 60.0f, 10.0f, 1.0f, 0.0f, 1.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    d = std::sqrt(dx*dx + dy*dy + dz*dz);
    INFO("por encima -> ", d);
    CHECK(d < Acicula::BRISA * 3.0f);
}

TEST_CASE("Fisica de hoja: es continua, no da saltos en el borde del radio") {
    // El empuje cae con un smoothstep. Si fuera un corte duro, la hoja pasaria
    // de apartada a normal en un frame y se veria un PARPADEO al caminar. Se
    // comprueba que dos puntos casi iguales dan desplazamientos casi iguales,
    // justo en el borde del radio, que es donde un escalon se notaria.
    const float R = Acicula::RADIO_EMPUJE;
    float a[3], b[3];

    Acicula::DesplazarHoja(10.0f + R - 0.01f, 70.0f, 10.0f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, a[0], a[1], a[2]);
    Acicula::DesplazarHoja(10.0f + R + 0.01f, 70.0f, 10.0f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, b[0], b[1], b[2]);

    const float salto = std::sqrt((a[0]-b[0])*(a[0]-b[0]) +
                                  (a[1]-b[1])*(a[1]-b[1]) +
                                  (a[2]-b[2])*(a[2]-b[2]));
    INFO("salto en el borde ", salto);
    CHECK(salto < 0.02f);
}

TEST_CASE("Fisica de hoja: dos mechones con distinta fase no van al unisono") {
    // Si todos compartieran fase, la copa entera latiria como un solo objeto
    // --se veria respirar-- en vez de ondular por zonas.
    float a[3], b[3];

    Acicula::DesplazarHoja(30.0f, 70.0f, 30.0f, 1.0f, /*fase=*/0.0f, 2.0f,
                           0.0f, 0.0f, 0.0f, a[0], a[1], a[2]);
    Acicula::DesplazarHoja(30.0f, 70.0f, 30.0f, 1.0f, /*fase=*/3.1f, 2.0f,
                           0.0f, 0.0f, 0.0f, b[0], b[1], b[2]);

    CHECK(a[0] != b[0]);
}

TEST_CASE("Fisica de hoja: es determinista") {
    // Sin estado ni memoria: el mismo instante da siempre el mismo resultado.
    // Es lo que permite que el mesher la aplique sin guardar nada por vertice.
    float a[3], b[3];
    Acicula::DesplazarHoja(12.3f, 71.5f, 9.7f, 0.8f, 1.1f, 4.25f,
                           11.0f, 70.0f, 10.0f, a[0], a[1], a[2]);
    Acicula::DesplazarHoja(12.3f, 71.5f, 9.7f, 0.8f, 1.1f, 4.25f,
                           11.0f, 70.0f, 10.0f, b[0], b[1], b[2]);
    CHECK(a[0] == b[0]);
    CHECK(a[1] == b[1]);
    CHECK(a[2] == b[2]);
}

TEST_CASE("Fisica de hoja: la hoja apartada cede tambien hacia abajo") {
    // Una rama que algo aparta no se desplaza solo en horizontal: se vence.
    // Sin la componente vertical el movimiento se lee como si la hoja
    // resbalara sobre un plano.
    float dx, dy, dz;
    Acicula::DesplazarHoja(10.5f, 70.0f, 10.0f, 1.0f, 0.0f, 0.0f,
                           10.0f, 70.0f, 10.0f, dx, dy, dz);
    CHECK(dy < 0.0f);
}

// ============================================================================
// LA CONEXION CON LA MADERA: LO QUE SE AGARRA NO SE MUEVE
// ============================================================================
// Es el comportamiento pedido: en una rama con hojas encima, se mueven las que
// quedan libres --las esquinas, lo que no llego a conectar-- y NO las del punto
// de union con la rama.

TEST_CASE("Sujecion: sin madera alrededor, el follaje cuelga suelto") {
    // El caso de referencia: una celda de hoja en el aire. Ningun mechon esta
    // sujeto, asi que todos se vencen y todos ondean.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];
    const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 5, 70, 5,
                                      buf, Acicula::MAX_MECHONES,
                                      /*maderaAlrededor=*/0);
    REQUIRE(n > 0);
    for (int i = 0; i < n; ++i) {
        INFO("mechon ", i, " sujecion ", buf[i].sujecion);
        CHECK(buf[i].sujecion == doctest::Approx(0.0f));
    }
}

TEST_CASE("Sujecion: con una rama debajo, algun mechon se agarra") {
    // Con madera en un lado, los mechones que apuntan HACIA ella quedan
    // sujetos. No todos: los que salen al lado contrario siguen libres, que es
    // justo lo que produce el contraste entre zona tensa y zona blanda.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];
    const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 5, 70, 5,
                                      buf, Acicula::MAX_MECHONES,
                                      Acicula::MADERA_ABAJO);
    REQUIRE(n > 0);

    int sujetos = 0, libres = 0;
    for (int i = 0; i < n; ++i) {
        if (buf[i].sujecion > 0.3f) ++sujetos;
        if (buf[i].sujecion < 0.05f) ++libres;
        // Nunca fuera de rango: es un coseno entre direcciones unitarias.
        CHECK(buf[i].sujecion >= 0.0f);
        CHECK(buf[i].sujecion <= 1.0f);
    }

    INFO("sujetos ", sujetos, " libres ", libres, " de ", n);
    CHECK(sujetos > 0);   // los que miran a la rama
    CHECK(libres > 0);    // los que miran al aire
}

TEST_CASE("Sujecion: el mechon agarrado NO se vence, el libre si") {
    // La FORMA. La caida se anula donde la acicula nace de la madera: sale
    // recta desde la vaina. Se compara la misma celda con y sin rama debajo.
    Acicula::Mechon libre[Acicula::MAX_MECHONES];
    Acicula::Mechon pegado[Acicula::MAX_MECHONES];

    const int nl = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 9, 72, 9,
                                       libre, Acicula::MAX_MECHONES, 0);
    const int np = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 9, 72, 9,
                                       pegado, Acicula::MAX_MECHONES,
                                       Acicula::MADERA_ABAJO);
    REQUIRE(nl == np);

    // El mechon que MAS se agarra en la version con rama tiene que apuntar
    // mas hacia abajo (hacia la rama) que su equivalente suelto, porque a este
    // ultimo se le resta la caida y al otro no.
    int mejor = 0;
    for (int i = 1; i < np; ++i)
        if (pegado[i].sujecion > pegado[mejor].sujecion) mejor = i;

    INFO("sujecion del mejor ", pegado[mejor].sujecion);
    CHECK(pegado[mejor].sujecion > 0.5f);
    // Su direccion difiere de la del mismo mechon sin sujetar: la caida ya no
    // le resta lo mismo.
    CHECK(pegado[mejor].dy != doctest::Approx(libre[mejor].dy));
}

TEST_CASE("Sujecion: la hoja pegada a la rama apenas se mueve") {
    // ⭐ EL COMPORTAMIENTO PEDIDO, MEDIDO.
    //
    // Mismo punto, mismo instante, mismo jugador encima: lo unico que cambia
    // es si el mechon esta agarrado a la madera. El sujeto tiene que moverse
    // MUCHO menos que el libre.
    float suelto[3], anclado[3];

    Acicula::DesplazarHoja(10.4f, 70.0f, 10.0f, 1.0f, 0.5f, 3.0f,
                           10.0f, 70.0f, 10.0f,
                           suelto[0], suelto[1], suelto[2],
                           /*sujecion=*/0.0f);
    Acicula::DesplazarHoja(10.4f, 70.0f, 10.0f, 1.0f, 0.5f, 3.0f,
                           10.0f, 70.0f, 10.0f,
                           anclado[0], anclado[1], anclado[2],
                           /*sujecion=*/1.0f);

    const float dSuelto = std::sqrt(suelto[0]*suelto[0] + suelto[1]*suelto[1] +
                                    suelto[2]*suelto[2]);
    const float dAnclado = std::sqrt(anclado[0]*anclado[0] +
                                     anclado[1]*anclado[1] +
                                     anclado[2]*anclado[2]);

    INFO("suelto ", dSuelto, " anclado ", dAnclado);
    REQUIRE(dSuelto > 1e-4f);
    // El anclado conserva como mucho el 20% del movimiento.
    CHECK(dAnclado < dSuelto * 0.2f);
    // Pero NO se congela del todo: una aguja sujeta aun vibra.
    CHECK(dAnclado > 0.0f);
}

TEST_CASE("Sujecion: el efecto es gradual, no un interruptor") {
    // Entre agarrado y suelto hay todos los grados intermedios. Si fuera
    // binario se veria una frontera dura en la copa, justo el corte que el
    // resto del sistema evita con smoothstep.
    float ant = 1e9f;
    for (float s = 0.0f; s <= 1.0f; s += 0.2f) {
        float d[3];
        Acicula::DesplazarHoja(10.4f, 70.0f, 10.0f, 1.0f, 0.5f, 3.0f,
                               10.0f, 70.0f, 10.0f, d[0], d[1], d[2], s);
        const float mag = std::sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
        INFO("sujecion ", s, " -> ", mag);
        CHECK(mag <= ant + 1e-5f);   // monotono decreciente
        ant = mag;
    }
}

TEST_CASE("Sujecion: la base sigue clavada aunque el mechon este suelto") {
    // La regla del voladizo manda por encima de todo: pase lo que pase con la
    // sujecion, el vertice pegado a la rama no se mueve.
    for (float s = 0.0f; s <= 1.0f; s += 0.25f) {
        float d[3];
        Acicula::DesplazarHoja(10.2f, 70.0f, 10.0f, /*t01=*/0.0f, 0.5f, 3.0f,
                               10.0f, 70.0f, 10.0f, d[0], d[1], d[2], s);
        INFO("sujecion ", s);
        CHECK(d[0] == 0.0f);
        CHECK(d[1] == 0.0f);
        CHECK(d[2] == 0.0f);
    }
}

TEST_CASE("Combinaciones: los 64 vecindarios dan resultados validos") {
    // Las 64 combinaciones de madera alrededor (2^6) son la fuente de la
    // variedad de siluetas. Ninguna puede producir geometria invalida.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];

    int distintas = 0;
    float firmaAnterior = -1e9f;

    for (int mapa = 0; mapa < 64; ++mapa) {
        const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE, 3, 66, 3,
                                          buf, Acicula::MAX_MECHONES,
                                          (uint8_t)mapa);
        REQUIRE(n >= 3);
        REQUIRE(n <= Acicula::MAX_MECHONES);

        float firma = 0.0f;
        for (int i = 0; i < n; ++i) {
            // Direccion unitaria y finita, siempre.
            const float len = std::sqrt(buf[i].dx*buf[i].dx +
                                        buf[i].dy*buf[i].dy +
                                        buf[i].dz*buf[i].dz);
            INFO("mapa ", mapa, " mechon ", i);
            CHECK(std::isfinite(len));
            CHECK(len == doctest::Approx(1.0f).epsilon(0.01f));
            CHECK(buf[i].sujecion >= 0.0f);
            CHECK(buf[i].sujecion <= 1.0f);
            firma += buf[i].dy * (float)(i + 1) + buf[i].sujecion;
        }
        if (firma != firmaAnterior) ++distintas;
        firmaAnterior = firma;
    }

    // La mayoria de los 64 vecindarios producen una copa distinta. Si todos
    // dieran lo mismo, el sistema de conexion no estaria haciendo nada.
    INFO("vecindarios con silueta distinta: ", distintas);
    CHECK(distintas > 30);
}

TEST_CASE("Combinaciones: la celda con la rama dentro se comporta como sujeta") {
    // BLOCK_LEAVES_OCOTE_RAMA lleva su rama en el mismo voxel, asi que su
    // follaje nace de madera aunque no tenga vecinos leñosos. El mesher le
    // pone MADERA_ABAJO por eso; aqui se comprueba que el efecto llega.
    Acicula::Mechon buf[Acicula::MAX_MECHONES];
    const int n = Acicula::MechonesDe(BLOCK_LEAVES_OCOTE_RAMA, 4, 68, 4,
                                      buf, Acicula::MAX_MECHONES,
                                      Acicula::MADERA_ABAJO);
    int sujetos = 0;
    for (int i = 0; i < n; ++i) if (buf[i].sujecion > 0.3f) ++sujetos;
    INFO("sujetos ", sujetos, " de ", n);
    CHECK(sujetos > 0);
}

// ============================================================================
// CON QUIEN COMPARTE CELDA LA ACICULA
// ============================================================================

TEST_CASE("Celda compartida: la acicula solo admite raices") {
    // Lo pedido: en el espacio de una hoja de ocote solo cabe ademas una raiz.
    const BlockType raices[4] = {
        BLOCK_RAIZ_PEQUENA, BLOCK_RAIZ_MEDIANA,
        BLOCK_RAIZ_GRANDE,  BLOCK_RAIZ_ENORME
    };

    for (BlockType r : raices) {
        INFO("raiz ", (int)r);
        // En los dos ordenes: da igual quien llegue primero a la celda.
        CHECK(combinar(BLOCK_LEAVES_OCOTE, r) != BLOCK_AIR);
        CHECK(combinar(r, BLOCK_LEAVES_OCOTE) != BLOCK_AIR);
        // Y lo que queda en la celda es la HOJA, que es lo que se ve.
        CHECK(combinar(BLOCK_LEAVES_OCOTE, r) == BLOCK_LEAVES_OCOTE);
    }
}

TEST_CASE("Celda compartida: la acicula NO admite nada mas") {
    // Todo lo demas queda fuera. Sin este corte, cualquier sprite que cayera
    // en la celda podria fundirse con el follaje.
    const BlockType prohibidos[6] = {
        BLOCK_TALLGRASS, BLOCK_ORANGE_FLOWER, BLOCK_IXTLE_HOJA,
        BLOCK_STONE, BLOCK_WATER, BLOCK_LEAVES
    };

    for (BlockType p : prohibidos) {
        INFO("con ", (int)p);
        CHECK(combinar(BLOCK_LEAVES_OCOTE, p) == BLOCK_AIR);
        CHECK(combinar(p, BLOCK_LEAVES_OCOTE) == BLOCK_AIR);
    }

    // Las cuatro variantes de follaje siguen la misma regla.
    const BlockType follaje[4] = {
        BLOCK_LEAVES_OCOTE, BLOCK_LEAVES_OCOTE_RAMA,
        BLOCK_LEAVES_OCOTE_CHINO, BLOCK_LEAVES_OCOTE_CHINO_RAMA
    };
    for (BlockType f : follaje) {
        INFO("follaje ", (int)f);
        CHECK(combinar(f, BLOCK_TALLGRASS) == BLOCK_AIR);
        CHECK(combinar(f, BLOCK_RAIZ_MEDIANA) != BLOCK_AIR);
    }
}

TEST_CASE("Celda compartida: el ixtle conserva sus parejas de siempre") {
    // La regla nueva del ocote se resuelve ANTES que la del ixtle. Hay que
    // comprobar que no se ha llevado por delante lo que ya funcionaba.
    CHECK(combinar(BLOCK_IXTLE_HOJA, BLOCK_TALLGRASS) == BLOCK_IXTLE_CON_HIERBA);
    CHECK(combinar(BLOCK_IXTLE_HOJA, BLOCK_ORANGE_FLOWER) == BLOCK_IXTLE_CON_FLOR);
    CHECK(combinar(BLOCK_IXTLE_HOJA, BLOCK_IXTLE_HOJA) == BLOCK_IXTLE_DOBLE);
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
