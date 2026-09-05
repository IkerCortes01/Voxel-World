#include <doctest/doctest.h>
#include "fauna/PecariRepoblacion.h"
#include <vector>
#include <cmath>

// ============================================================================
// TESTS DE REPOBLACION DINAMICA
// ============================================================================
// La repoblacion es mas peligrosa que la generacion estatica: se dispara con
// una accion que el JUGADOR controla y puede repetir a voluntad (cargar un
// chunk). Un fallo aqui no produce un mundo raro, produce un mundo ROTO,
// y de las dos formas opuestas:
//
//   - Demasiado permisiva  -> el jugador farmea pecaries recargando chunks
//   - Demasiado restrictiva-> el mundo se vacia y no se recupera (Ultima
//                             Online)
//
// Estos tests cubren las dos.
// ============================================================================

using namespace Fauna;
using namespace TerrainGen;

// Contexto de una zona VACIA y lejos del jugador: el caso mas favorable.
static ContextoRepoblacion CtxVacio(double t = 1000.0) {
    ContextoRepoblacion c;
    c.pecariesVivosCerca = 0;
    c.radioConteoBloques = 128.0f;
    c.distanciaAlJugadorBloques = 200.0f;   // muy lejos
    c.tiempoJuegoSegundos = t;
    return c;
}

TEST_CASE("Repoblacion: NO aparece a la vista del jugador") {
    PecariRepoblacion rep(12345);

    // Regla 1 de 01_ARQUITECTURA. El motor carga chunks hasta 80 bloques y la
    // niebla es opaca a 78.4, asi que aparecer dentro de ese radio significa
    // que el jugador ve el "pop" de la nada.
    //
    // Se barre todo el rango prohibido y se exige CERO apariciones.
    int aparecidos = 0;
    for (float dist = 0.0f; dist < PecariRepoblacion::RADIO_MIN_BLOQUES; dist += 2.0f) {
        for (int cx = 0; cx < 200; ++cx) {
            for (int cz = 0; cz < 5; ++cz) {
                ContextoRepoblacion c = CtxVacio();
                c.distanciaAlJugadorBloques = dist;
                GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, c);
                if (g.aparece) ++aparecidos;
            }
        }
    }
    CHECK(aparecidos == 0);
}

TEST_CASE("Repoblacion: SI aparece lejos del jugador") {
    PecariRepoblacion rep(12345);

    // El complemento del test anterior. Si el filtro de distancia fuera
    // demasiado agresivo no aparecerian nunca, y el test de arriba pasaria
    // igual: el mundo se vaciaria en silencio.
    int aparecidos = 0;
    for (int cx = 0; cx < 300; ++cx) {
        for (int cz = 0; cz < 10; ++cz) {
            GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, CtxVacio());
            if (g.aparece) ++aparecidos;
        }
    }
    CHECK(aparecidos > 0);
}

TEST_CASE("Repoblacion: recargar el mismo chunk NO da pecaries infinitos") {
    PecariRepoblacion rep(999);

    // ESTE ES EL TEST QUE MAS IMPORTA.
    //
    // Cargar un chunk es una accion que el jugador controla y puede repetir
    // indefinidamente (alejarse y volver). Si cada carga tirase el dado de
    // nuevo, el jugador podria farmear pecaries sin limite.
    //
    // La defensa es cuantizar el tiempo en ventanas: todas las cargas dentro
    // de la misma ventana deben dar EXACTAMENTE el mismo resultado.

    // Mil recargas del mismo chunk en el mismo instante.
    GrupoRepoblacion primera = rep.ConsultarChunk(42, 17, BIOME_FOREST, 50.0f, 0.1f, CtxVacio(1000.0));
    for (int i = 0; i < 1000; ++i) {
        GrupoRepoblacion g = rep.ConsultarChunk(42, 17, BIOME_FOREST, 50.0f, 0.1f, CtxVacio(1000.0));
        CHECK(g.aparece  == primera.aparece);
        CHECK(g.miembros == primera.miembros);
        CHECK(g.x        == primera.x);
        CHECK(g.z        == primera.z);
    }

    // Y dentro de la MISMA ventana, aunque el tiempo avance un poco.
    // Ventana = 300 s, asi que 1000.0 y 1200.0 caen en la misma (la 3a).
    GrupoRepoblacion mismaVentana = rep.ConsultarChunk(42, 17, BIOME_FOREST, 50.0f, 0.1f, CtxVacio(1200.0));
    CHECK(mismaVentana.aparece  == primera.aparece);
    CHECK(mismaVentana.miembros == primera.miembros);
}

TEST_CASE("Repoblacion: en ventanas distintas el resultado puede cambiar") {
    PecariRepoblacion rep(999);

    // El complemento: si el resultado NUNCA cambiara con el tiempo, la
    // repoblacion no existiria (seria generacion estatica con otro nombre).
    //
    // Se cuenta cuantos chunks cambian de resultado entre dos ventanas muy
    // separadas.
    int cambios = 0;
    for (int cx = 0; cx < 400; ++cx) {
        GrupoRepoblacion a = rep.ConsultarChunk(cx, 3, BIOME_FOREST, 50.0f, 0.1f, CtxVacio(1000.0));
        GrupoRepoblacion b = rep.ConsultarChunk(cx, 3, BIOME_FOREST, 50.0f, 0.1f, CtxVacio(99000.0));
        if (a.aparece != b.aparece) ++cambios;
    }
    CHECK(cambios > 0);
}

TEST_CASE("Repoblacion: son POCOS (2-4), no manadas enteras") {
    PecariRepoblacion rep(555);

    // El usuario pidio explicitamente "pocos". Un grupo de repoblacion es un
    // fragmento recolonizador, no una manada completa (que son 5-15).
    //
    // Base biologica: MEDIDO que el 37-38% de machos se dispersan entre
    // manadas; la recolonizacion real ocurre por grupitos, no por manadas
    // teletransportadas.
    int comprobados = 0;
    for (int cx = 0; cx < 2000 && comprobados < 100; ++cx) {
        for (int cz = 0; cz < 20; ++cz) {
            GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, CtxVacio());
            if (g.aparece) {
                CHECK(g.miembros >= 2);
                CHECK(g.miembros <= 4);
                ++comprobados;
            }
        }
    }
    CHECK(comprobados > 0);

    // Y deben ser ESTRICTAMENTE menos que una manada estructural minima (5).
    CHECK(PecariRepoblacion::GRUPO_MAX < 5);
}

TEST_CASE("Repoblacion: nunca aparece un individuo suelto") {
    PecariRepoblacion rep(321);

    // Regla 3 de 01_ARQUITECTURA: "Aparecer como grupo si la especie es
    // social, no como individuos sueltos".
    //
    // El pecari es una especie social obligada: un pecari solo no es un
    // pecari, es un error de simulacion.
    for (int cx = 0; cx < 3000; ++cx) {
        GrupoRepoblacion g = rep.ConsultarChunk(cx, 7, BIOME_DESERT, 50.0f, 0.1f, CtxVacio());
        if (g.aparece) CHECK(g.miembros >= 2);
    }
}

TEST_CASE("Repoblacion: la respuesta densodependiente apaga el sistema") {
    // El corazon del sistema, probado en aislamiento.

    SUBCASE("zona vacia: repoblacion maxima") {
        CHECK(PecariRepoblacion::FactorDensodependiente(0, 128.0f) == doctest::Approx(1.0f));
    }

    SUBCASE("zona en el objetivo: repoblacion APAGADA") {
        // Densidad objetivo = 0.000877 ind/bloque2
        // Area de radio 128 = pi*128^2 = 51471.9 bloques2
        // Individuos para llegar al objetivo = 51471.9 * 0.000877 = 45.1
        CHECK(PecariRepoblacion::FactorDensodependiente(46, 128.0f) == doctest::Approx(0.0f));
        CHECK(PecariRepoblacion::FactorDensodependiente(100, 128.0f) == doctest::Approx(0.0f));
        CHECK(PecariRepoblacion::FactorDensodependiente(10000, 128.0f) == doctest::Approx(0.0f));
    }

    SUBCASE("es monotona decreciente: mas poblacion, menos repoblacion") {
        float anterior = 2.0f;
        for (int vivos = 0; vivos <= 50; ++vivos) {
            const float f = PecariRepoblacion::FactorDensodependiente(vivos, 128.0f);
            CHECK(f <= anterior);
            CHECK(f >= 0.0f);
            CHECK(f <= 1.0f);
            anterior = f;
        }
    }

    SUBCASE("el decaimiento es cuadratico, no lineal") {
        // A media ocupacion (~23 individuos), el factor lineal daria 0.5.
        // El cuadratico da ~0.25. Se comprueba que esta claramente por
        // debajo de 0.5, que es lo que distingue una recuperacion ecologica
        // (rapida cuando esta vacio, lenta cerca del objetivo) de un goteo
        // constante.
        const float f = PecariRepoblacion::FactorDensodependiente(23, 128.0f);
        CHECK(f < 0.40f);
        CHECK(f > 0.15f);
    }
}

TEST_CASE("Repoblacion: una zona poblada no recibe mas pecaries") {
    PecariRepoblacion rep(777);

    // Test de integracion de la densodependencia: con la zona llena, ningun
    // chunk debe repoblar, por muchos que se carguen.
    ContextoRepoblacion lleno = CtxVacio();
    lleno.pecariesVivosCerca = 60;   // por encima del objetivo (~45)

    int aparecidos = 0;
    for (int cx = 0; cx < 2000; ++cx) {
        for (int cz = 0; cz < 5; ++cz) {
            GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, lleno);
            if (g.aparece) ++aparecidos;
            if (!g.aparece && g.motivo != GrupoRepoblacion::MOTIVO_ZONA_YA_POBLADA) {
                // No es un fallo, pero el motivo dominante debe ser este.
            }
        }
    }
    CHECK(aparecidos == 0);
}

TEST_CASE("Repoblacion: una zona vaciada por el jugador SI se recupera") {
    PecariRepoblacion rep(777);

    // El contrapunto del test anterior, y la razon de ser del archivo.
    //
    // Es literalmente el escenario que hundio a Ultima Online: el jugador
    // arrasa una zona. Si el sistema no repuebla, el mundo queda muerto para
    // siempre.
    ContextoRepoblacion vaciado = CtxVacio();
    vaciado.pecariesVivosCerca = 0;

    int aparecidos = 0;
    for (int cx = 0; cx < 2000; ++cx) {
        for (int cz = 0; cz < 5; ++cz) {
            if (rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, vaciado).aparece) {
                ++aparecidos;
            }
        }
    }
    CHECK(aparecidos > 0);
}

TEST_CASE("Repoblacion: la recuperacion es mas rapida cuanto mas vacia la zona") {
    PecariRepoblacion rep(4242);

    // La respuesta densodependiente debe notarse en el AGREGADO, no solo en
    // la formula: una zona muy vaciada recibe mas grupos que una medio llena.
    auto contar = [&](int vivos) {
        ContextoRepoblacion c = CtxVacio();
        c.pecariesVivosCerca = vivos;
        int n = 0;
        for (int cx = 0; cx < 3000; ++cx) {
            for (int cz = 0; cz < 5; ++cz) {
                if (rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, c).aparece) ++n;
            }
        }
        return n;
    };

    const int vacia     = contar(0);
    const int mediaLlena= contar(25);
    const int casiLlena = contar(40);

    CHECK(vacia > mediaLlena);
    CHECK(mediaLlena > casiLlena);
}

TEST_CASE("Repoblacion: respeta el habitat igual que la poblacion estructural") {
    PecariRepoblacion rep(888);

    // Regla 2. Se reutiliza PecariSpawn::IdoneidadBioma, asi que esto
    // verifica ademas que la reutilizacion funciona y no se duplico la tabla
    // con valores distintos.
    const BiomeType prohibidos[] = {
        BIOME_OCEAN_DEEP, BIOME_OCEAN, BIOME_BEACH, BIOME_MOUNTAIN_PEAKS
    };
    for (BiomeType b : prohibidos) {
        int aparecidos = 0;
        for (int cx = 0; cx < 1000; ++cx) {
            for (int cz = 0; cz < 5; ++cz) {
                if (rep.ConsultarChunk(cx, cz, b, 50.0f, 0.1f, CtxVacio()).aparece) ++aparecidos;
            }
        }
        CHECK(aparecidos == 0);
    }
}

TEST_CASE("Repoblacion: respeta pendiente y altitud") {
    PecariRepoblacion rep(6161);

    SUBCASE("pendiente fuerte") {
        int n = 0;
        for (int cx = 0; cx < 1000; ++cx) {
            if (rep.ConsultarChunk(cx, 3, BIOME_FOREST, 50.0f, 0.9f, CtxVacio()).aparece) ++n;
        }
        CHECK(n == 0);
    }

    SUBCASE("por encima de 2335 m no aparece") {
        // 4000 bloques * 0.60 = 2400 m
        int n = 0;
        for (int cx = 0; cx < 1000; ++cx) {
            if (rep.ConsultarChunk(cx, 3, BIOME_MOUNTAINS, 4000.0f, 0.1f, CtxVacio()).aparece) ++n;
        }
        CHECK(n == 0);
    }
}

TEST_CASE("Repoblacion: los miembros salen cohesionados") {
    PecariRepoblacion rep(31337);

    GrupoRepoblacion g;
    bool encontrado = false;
    for (int cx = 0; cx < 3000 && !encontrado; ++cx) {
        g = rep.ConsultarChunk(cx, 5, BIOME_FOREST, 50.0f, 0.1f, CtxVacio());
        if (g.aparece) encontrado = true;
    }
    REQUIRE(encontrado);

    for (int i = 0; i < g.miembros; ++i) {
        int px = 0, pz = 0;
        rep.PosicionMiembro(g, i, px, pz);
        const float dx = (float)(px - g.x);
        const float dz = (float)(pz - g.z);
        CHECK(std::sqrt(dx*dx + dz*dz) <= g.dispersionBloques + 1.5f);
    }
}

TEST_CASE("Repoblacion: es MUCHO menos frecuente que la generacion estructural") {
    PecariRepoblacion rep(2024);

    // La repoblacion NO debe competir con PecariSpawn: es un mecanismo de
    // rescate, no una segunda fuente de poblacion. Si repoblara mas que la
    // generacion base, el mundo se llenaria de grupos pequenos y la
    // estructura de manadas se perderia.
    //
    // Se mide sobre una zona VACIA (el caso mas favorable para repoblar) y
    // se comprueba que la fraccion de chunks que repuebla es baja.
    int chunksConGrupo = 0;
    const int TOTAL = 5000;
    for (int cx = 0; cx < TOTAL; ++cx) {
        if (rep.ConsultarChunk(cx, 11, BIOME_FOREST, 50.0f, 0.1f, CtxVacio()).aparece) {
            ++chunksConGrupo;
        }
    }
    const double fraccion = (double)chunksConGrupo / TOTAL;

    // Con la cuota por vecindad activa, solo 4 de cada 256 chunks pueden
    // repoblar por ventana (1.6%), y de esos solo el 35% acierta la tirada.
    // La fraccion esperada es por tanto muy pequena: del orden del 0.5%.
    //
    // Es exactamente lo que se busca: la repoblacion es un goteo de rescate,
    // no una segunda fuente de poblacion que compita con PecariSpawn.
    CHECK(fraccion > 0.0);
    CHECK(fraccion < 0.05);
}

TEST_CASE("Repoblacion: cargar MUCHOS chunks a la vez no inunda la zona") {
    PecariRepoblacion rep(12345);

    // TEST DE REGRESION DE UN BUG REAL.
    //
    // Este fallo existio y NINGUN test unitario lo detecto, porque solo
    // aparece al cargar muchos chunks a la vez:
    //
    //   La densodependencia se evalua por chunk, y al cargar una zona
    //   arrasada los 400 chunks reciben el MISMO contexto ("hay 0 vivos").
    //   Los 400 deciden en paralelo, cada uno con su probabilidad, y cada uno
    //   acierta por separado. Medido: 131 grupos y 389 individuos de golpe,
    //   con un objetivo de ~45. Sobredisparo de 8.6x.
    //
    // Es el problema clasico de N agentes decidiendo a la vez sobre un
    // recurso compartido sin verse entre ellos.
    //
    // La defensa es la cuota por vecindad. Este test la vigila.
    // Se comprueba en VARIAS ventanas, porque en una sola puede salir cero
    // legitimamente: con la cuota activa solo 4 chunks de la vecindad tienen
    // derecho, y cada uno aun tiene que superar su tirada del 35%.
    //
    // Lo que este test vigila NO es que aparezca algo en cada ventana, sino
    // que NINGUNA ventana produzca la avalancha de 389 individuos del bug.
    int peorVentana = 0;
    int totalAcumulado = 0;

    for (int v = 0; v < 20; ++v) {
        ContextoRepoblacion arrasada = CtxVacio(v * PecariRepoblacion::VENTANA_SEGUNDOS + 10.0);
        arrasada.pecariesVivosCerca = 0;   // el peor caso: zona siempre vacia

        int individuos = 0;
        for (int cx = 0; cx < 20; ++cx) {
            for (int cz = 0; cz < 20; ++cz) {
                GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, arrasada);
                if (g.aparece) individuos += g.miembros;
            }
        }

        // NINGUNA ventana puede inundar la zona. Antes del arreglo, la
        // primera daba 389 individuos con un objetivo de ~45.
        CHECK(individuos <= 45);

        if (individuos > peorVentana) peorVentana = individuos;
        totalAcumulado += individuos;
    }

    // Y a lo largo de 20 ventanas debe aparecer ALGO: si diera 0 siempre, el
    // arreglo habria roto la recuperacion en vez de acotarla, y el mundo se
    // quedaria muerto (que es el fallo de Ultima Online que esto previene).
    CHECK(totalAcumulado > 0);
}

TEST_CASE("Repoblacion: la cuota por vecindad se reparte con el tiempo") {
    PecariRepoblacion rep(555);

    // La cuota limita cuantos chunks repueblan por ventana, pero NO debe
    // congelar siempre los mismos: eso concentraria toda la repoblacion en
    // cuatro esquinas fijas del mapa.
    //
    // Se comprueba que a lo largo de varias ventanas participan chunks
    // distintos.
    std::vector<int> chunksQueRepoblaron;
    for (int v = 0; v < 30; ++v) {
        ContextoRepoblacion c = CtxVacio(v * PecariRepoblacion::VENTANA_SEGUNDOS + 10.0);
        for (int cx = 0; cx < 16; ++cx) {
            if (rep.ConsultarChunk(cx, 0, BIOME_FOREST, 50.0f, 0.1f, c).aparece) {
                chunksQueRepoblaron.push_back(cx);
            }
        }
    }

    REQUIRE(chunksQueRepoblaron.size() >= 2);

    // Debe haber al menos dos chunks DISTINTOS entre los que repoblaron.
    bool hayVariedad = false;
    for (size_t i = 1; i < chunksQueRepoblaron.size(); ++i) {
        if (chunksQueRepoblaron[i] != chunksQueRepoblaron[0]) { hayVariedad = true; break; }
    }
    CHECK(hayVariedad);
}

TEST_CASE("Repoblacion: el motivo explica siempre la decision") {
    PecariRepoblacion rep(1234);

    // 01_ARQUITECTURA exige tooling de depuracion ANTES que contenido, y la
    // causa 4 del fracaso de Ultima Online fue "no hubo deteccion de fallo en
    // produccion". Un motivo vacio o incoherente hace invisible un bug.

    // Bioma malo -> motivo correcto
    GrupoRepoblacion g1 = rep.ConsultarChunk(10, 10, BIOME_OCEAN, 50.0f, 0.1f, CtxVacio());
    CHECK(g1.aparece == false);
    CHECK(g1.motivo == GrupoRepoblacion::MOTIVO_BIOMA_NO_APTO);

    // Demasiado cerca -> motivo correcto
    ContextoRepoblacion cerca = CtxVacio();
    cerca.distanciaAlJugadorBloques = 10.0f;
    GrupoRepoblacion g2 = rep.ConsultarChunk(10, 10, BIOME_FOREST, 50.0f, 0.1f, cerca);
    CHECK(g2.aparece == false);
    CHECK(g2.motivo == GrupoRepoblacion::MOTIVO_DEMASIADO_CERCA_DEL_JUGADOR);

    // Zona llena -> motivo correcto
    ContextoRepoblacion lleno = CtxVacio();
    lleno.pecariesVivosCerca = 200;
    GrupoRepoblacion g3 = rep.ConsultarChunk(10, 10, BIOME_FOREST, 50.0f, 0.1f, lleno);
    CHECK(g3.aparece == false);
    CHECK(g3.motivo == GrupoRepoblacion::MOTIVO_ZONA_YA_POBLADA);

    // Pendiente -> motivo correcto
    GrupoRepoblacion g4 = rep.ConsultarChunk(10, 10, BIOME_FOREST, 50.0f, 0.95f, CtxVacio());
    CHECK(g4.aparece == false);
    CHECK(g4.motivo == GrupoRepoblacion::MOTIVO_PENDIENTE_O_ALTITUD);

    // Y el texto nunca es nulo
    CHECK(PecariRepoblacion::MotivoTexto(g1.motivo) != nullptr);
    CHECK(PecariRepoblacion::MotivoTexto(GrupoRepoblacion::MOTIVO_APARECE) != nullptr);
}

TEST_CASE("Repoblacion: funciona con coordenadas de chunk negativas") {
    PecariRepoblacion rep(2024);

    // Igual que en PecariSpawn: el mundo se extiende en las cuatro
    // direcciones y el hash debe comportarse en territorio negativo.
    int aparecidos = 0;
    for (int cx = -2000; cx < 0; ++cx) {
        for (int cz = -10; cz < 0; ++cz) {
            GrupoRepoblacion g = rep.ConsultarChunk(cx, cz, BIOME_FOREST, 50.0f, 0.1f, CtxVacio());
            if (g.aparece) {
                ++aparecidos;
                CHECK(g.miembros >= 2);
                CHECK(g.miembros <= 4);
            }
        }
    }
    CHECK(aparecidos > 0);
}

TEST_CASE("Repoblacion: seeds distintas dan resultados distintos") {
    PecariRepoblacion a(1000);
    PecariRepoblacion b(2000);

    int diferencias = 0;
    for (int cx = 0; cx < 1000; ++cx) {
        const bool ga = a.ConsultarChunk(cx, 4, BIOME_FOREST, 50.0f, 0.1f, CtxVacio()).aparece;
        const bool gb = b.ConsultarChunk(cx, 4, BIOME_FOREST, 50.0f, 0.1f, CtxVacio()).aparece;
        if (ga != gb) ++diferencias;
    }
    CHECK(diferencias > 0);
}
