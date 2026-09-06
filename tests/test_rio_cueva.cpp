#include <doctest/doctest.h>
#include "terrain/CaveGenerator.h"
#include "terrain/TerrainGenerator.h"

// ============================================================================
// RIOS Y CUEVAS: QUE EL AGUA NO SE QUEDE FLOTANDO
// ============================================================================
// EL BUG, medido con el generador real (seed 12345, 160.000 columnas):
//
//     columnas de cauce                          7.412
//     con una BOCA que abre el propio lecho        725   ->  9.78%
//
// Una de cada diez columnas de rio tenia un pozo de entrada perforandola. El
// agua del cauce se rellena desde surfaceY+1, asi que al abrirse el suelo bajo
// ella quedaba FLOTANDO sobre el agujero.
//
// La causa: IsCave respeta SURFACE_MARGIN (por eso el hueco minimo bajo un
// cauce eran 5 bloques, nunca menos), pero IsCaveEntrance NO -- baja hasta 56
// bloques sin mirar si perfora tierra firme o el fondo de un rio.
//
// Lo que NO se queria: prohibir las cuevas bajo los rios en general. Eso
// vaciaria el subsuelo y ademas impide pasar por debajo de un cauce. Se
// protege solo una LOSA fina bajo el lecho.

using namespace TerrainGen;

// ============================================================================
// LA LOSA BAJO EL LECHO
// ============================================================================

TEST_CASE("Rio-cueva: la losa protege justo bajo el lecho, no mas") {
    const int SUP = 70;

    // Dentro de la losa: protegido.
    for (int y = SUP; y > SUP - CaveGenerator::LOSA_LECHO; --y) {
        INFO("y=", y, " superficie=", SUP);
        CHECK(CaveGenerator::BajoLechoDeRio(y, SUP, true));
    }

    // Justo por debajo de la losa: ya NO. Es lo que deja pasar una galeria por
    // debajo del cauce.
    CHECK_FALSE(CaveGenerator::BajoLechoDeRio(
        SUP - CaveGenerator::LOSA_LECHO, SUP, true));
    CHECK_FALSE(CaveGenerator::BajoLechoDeRio(
        SUP - CaveGenerator::LOSA_LECHO - 10, SUP, true));
    CHECK_FALSE(CaveGenerator::BajoLechoDeRio(20, SUP, true));
}

TEST_CASE("Rio-cueva: donde no hay cauce, la losa no protege nada") {
    // La proteccion es SOLO para los rios. Si se activara en tierra firme
    // estariamos tapando cuevas normales sin motivo.
    const int SUP = 70;
    for (int y = 0; y <= SUP; y += 5) {
        INFO("y=", y);
        CHECK_FALSE(CaveGenerator::BajoLechoDeRio(y, SUP, false));
    }
}

TEST_CASE("Rio-cueva: la losa es mas gruesa que el margen de superficie") {
    // Si LOSA_LECHO fuera <= SURFACE_MARGIN, esta proteccion no añadiria nada:
    // IsCave ya corta ahi por su cuenta. Tiene que ser estrictamente mayor
    // para que sirva de algo.
    CHECK(CaveGenerator::LOSA_LECHO > CaveGenerator::SURFACE_MARGIN);
}

// ============================================================================
// EL ARREGLO, CONTRA EL GENERADOR REAL
// ============================================================================

TEST_CASE("Rio-cueva: ninguna boca abre el lecho de un rio") {
    // ⭐ EL TEST QUE FIJA EL ARREGLO.
    //
    // Antes: 725 de 7.412 columnas de cauce (9.78%). Ahora tiene que ser CERO.
    // Se barre un area grande para que salgan bastantes rios de verdad.
    const int SEED = 12345;
    TerrainGenerator terreno(SEED);
    CaveGenerator cuevas(SEED);

    int columnasRio = 0;
    int bocasEnLecho = 0;

    for (int x = -300; x < 300; x += 7) {
        for (int z = -300; z < 300; z += 7) {
            const ColumnData col = terreno.GetColumnData((float)x, (float)z);
            if (!col.isRiverBed) continue;
            ++columnasRio;

            const int sy = col.surfaceHeight;
            for (int y = sy; y >= sy - CaveGenerator::ENTRADA_MAX_HONDURA; --y) {
                if (y < 1) break;
                if (cuevas.IsCaveEntrance((float)x, y, (float)z, sy,
                                          col.isRiverBed)) {
                    ++bocasEnLecho;
                    break;
                }
            }
        }
    }

    INFO("columnas de rio barridas: ", columnasRio);
    REQUIRE(columnasRio > 50);        // que el barrido haya visto rios
    CHECK(bocasEnLecho == 0);
}

TEST_CASE("Rio-cueva: ninguna cueva vacia la losa bajo el cauce") {
    const int SEED = 12345;
    TerrainGenerator terreno(SEED);
    CaveGenerator cuevas(SEED);

    int columnasRio = 0;
    int cuevasEnLosa = 0;

    for (int x = -300; x < 300; x += 7) {
        for (int z = -300; z < 300; z += 7) {
            const ColumnData col = terreno.GetColumnData((float)x, (float)z);
            if (!col.isRiverBed) continue;
            ++columnasRio;

            const int sy = col.surfaceHeight;
            for (int y = sy; y > sy - CaveGenerator::LOSA_LECHO; --y) {
                if (y < 1) break;
                if (cuevas.IsCave((float)x, y, (float)z, sy, col.isRiverBed))
                    ++cuevasEnLosa;
            }
        }
    }

    REQUIRE(columnasRio > 50);
    CHECK(cuevasEnLosa == 0);
}

// ============================================================================
// Y QUE NO SE HAYAN CARGADO LAS CUEVAS PARA CONSEGUIRLO
// ============================================================================
// Estos son los tests que impiden el "arreglo" facil: prohibir cuevas bajo los
// rios sin mas. Eso pondria los dos tests de arriba en verde y arruinaria el
// mundo.

TEST_CASE("Rio-cueva: SE PUEDE pasar por debajo de un rio") {
    // ⭐ Lo pedido explicitamente: la cueva puede estar al lado o por debajo,
    // conectada de lejos. Si la proteccion se hubiera aplicado a la columna
    // entera, esto daria cero y el subsuelo quedaria partido por cada cauce.
    const int SEED = 12345;
    TerrainGenerator terreno(SEED);
    CaveGenerator cuevas(SEED);

    int columnasRio = 0;
    int conCuevaDebajo = 0;

    for (int x = -300; x < 300; x += 7) {
        for (int z = -300; z < 300; z += 7) {
            const ColumnData col = terreno.GetColumnData((float)x, (float)z);
            if (!col.isRiverBed) continue;
            ++columnasRio;

            const int sy = col.surfaceHeight;
            for (int y = sy - CaveGenerator::LOSA_LECHO; y >= 1; --y) {
                if (cuevas.IsCave((float)x, y, (float)z, sy, col.isRiverBed)) {
                    ++conCuevaDebajo;
                    break;
                }
            }
        }
    }

    REQUIRE(columnasRio > 50);
    // La inmensa mayoria de los cauces tiene galeria por debajo.
    INFO("con cueva debajo: ", conCuevaDebajo, " de ", columnasRio);
    CHECK(conCuevaDebajo > columnasRio / 2);
}

TEST_CASE("Rio-cueva: las cuevas de AL LADO del rio no se tocan") {
    // La proteccion depende de `esLechoDeRio`, que solo es cierto en el cauce
    // (strength > 0.35). Una cueva a dos bloques del agua sigue existiendo.
    //
    // Se comprueba de la forma mas directa: el MISMO voxel, preguntado como
    // lecho y como no-lecho, a una hondura dentro de la losa.
    const int SEED = 12345;
    CaveGenerator cuevas(SEED);

    // ⚠️ LA COTA IMPORTA, Y LA PRIMERA VERSION DE ESTE TEST LA TENIA MAL.
    //
    // Se probaba a `sy - 2`, pero IsCave corta por su cuenta todo lo que este
    // a menos de SURFACE_MARGIN (5) de la superficie: a esa hondura no hay
    // cueva NUNCA, con rio o sin el. El test daba cero y no probaba nada.
    //
    // Hay que mirar en la franja donde las dos cosas pueden pasar a la vez:
    // mas honda que SURFACE_MARGIN (para que IsCave pueda decir que si) pero
    // aun dentro de LOSA_LECHO (para que la proteccion del rio aplique).
    const int Y_PRUEBA = -(CaveGenerator::LOSA_LECHO - 1);   // -7 con losa 8
    static_assert(CaveGenerator::LOSA_LECHO - 1 > CaveGenerator::SURFACE_MARGIN,
                  "la franja de prueba tiene que caer bajo SURFACE_MARGIN, o "
                  "este test no comprueba nada");

    int huboAlguna = 0;
    for (int x = -200; x < 200; x += 3) {
        for (int z = -200; z < 200; z += 3) {
            const int sy = 70;
            const int y = sy + Y_PRUEBA;   // dentro de la losa y bajo el margen
            // Fuera del cauce el voxel se decide solo por el ruido...
            const bool fuera = cuevas.IsCave((float)x, y, (float)z, sy, false);
            // ...y dentro del cauce esta protegido siempre.
            const bool dentro = cuevas.IsCave((float)x, y, (float)z, sy, true);

            CHECK_FALSE(dentro);              // en el cauce: nunca
            if (fuera) ++huboAlguna;          // al lado: a veces si
        }
    }
    // Tiene que haber ALGUNA cueva fuera del cauce a esa cota, o el test no
    // estaria probando nada (y significaria que se han perdido las cuevas).
    INFO("cuevas fuera del cauce a esa cota: ", huboAlguna);
    CHECK(huboAlguna > 0);
}

TEST_CASE("Rio-cueva: sin marcar lecho, el generador se comporta como antes") {
    // COMPATIBILIDAD. El parametro nuevo tiene valor por omision false, asi
    // que todo el codigo que no sepa de rios --tests viejos, herramientas--
    // sigue obteniendo exactamente el mismo mundo. Si esto fallara, cambiaria
    // el terreno de los mundos ya guardados.
    const int SEED = 999;
    CaveGenerator cuevas(SEED);

    for (int x = -100; x < 100; x += 11) {
        for (int z = -100; z < 100; z += 11) {
            for (int y = 12; y < 60; y += 7) {
                const bool porDefecto = cuevas.IsCave((float)x, y, (float)z, 70);
                const bool explicito  = cuevas.IsCave((float)x, y, (float)z, 70, false);
                INFO("voxel ", x, ",", y, ",", z);
                CHECK(porDefecto == explicito);
            }
        }
    }
}

TEST_CASE("Rio-cueva: el mundo sigue teniendo bocas de sobra") {
    // La correccion quita bocas (las que caian sobre agua), pero no puede
    // desplomar el numero total: son la forma de entrar a las cuevas.
    const int SEED = 12345;
    TerrainGenerator terreno(SEED);
    CaveGenerator cuevas(SEED);

    int columnas = 0, conBoca = 0;

    for (int x = -300; x < 300; x += 7) {
        for (int z = -300; z < 300; z += 7) {
            ++columnas;
            const ColumnData col = terreno.GetColumnData((float)x, (float)z);
            const int sy = col.surfaceHeight;
            for (int y = sy; y >= sy - CaveGenerator::ENTRADA_MAX_HONDURA; --y) {
                if (y < 1) break;
                if (cuevas.IsCaveEntrance((float)x, y, (float)z, sy,
                                          col.isRiverBed)) {
                    ++conBoca;
                    break;
                }
            }
        }
    }

    const double pct = 100.0 * conBoca / columnas;
    INFO("columnas con boca: ", conBoca, " de ", columnas, " (", pct, "%)");
    // La medicion previa daba ~9% de columnas con boca; tras quitar las de
    // los cauces baja poco. Un 4% seria ya una perdida grave.
    CHECK(pct > 4.0);
}
