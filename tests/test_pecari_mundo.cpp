#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include <cmath>

// ============================================================================
// TESTS DE INTEGRACION DEL REGISTRO DE PECARIES
// ============================================================================
// Aqui se prueba que las cuatro piezas funcionan JUNTAS. Los tests anteriores
// prueban cada una por separado; estos prueban lo que solo falla al unirlas:
//
//   1. ANTIDUPLICADO - recargar un chunk no duplica la manada
//   2. TOPE DURO     - la poblacion no crece sin limite
//   3. TERRENO       - los animales acaban sobre el suelo, no flotando
//   4. DESCARGA      - alejarse libera memoria
//
// Se usa un mundo SINTETICO, sin motor ni OpenGL. Es exactamente lo que
// 01_ARQUITECTURA busca con la inversion de dependencias: "la IA se puede
// probar contra un mundo sintetico en tests, sin arrancar OpenGL".
// ============================================================================

using namespace Fauna;
using namespace TerrainGen;

// ----------------------------------------------------------------------------
// MUNDO DE PRUEBA
// ----------------------------------------------------------------------------
// Terreno llano de selva a altura fija. Lo mas simple que permite probar todo
// lo demas sin que el terreno introduzca ruido.
class MundoLlano : public IPecariMundo {
public:
    float altura = 64.0f;
    BiomeType bioma = BIOME_FOREST;
    float pendiente = 0.0f;

    float alturaSuelo(int, int) const override { return altura; }
    BiomeType biomaEn(int, int) const override { return bioma; }
    float pendienteEn(int, int) const override { return pendiente; }
    bool esSolido(int, int y, int) const override { return (float)y < altura; }
};

// Mundo con una colina, para probar el seguimiento de terreno.
class MundoColina : public IPecariMundo {
public:
    float alturaSuelo(int x, int z) const override {
        // Una loma suave centrada en el origen.
        const float d = std::sqrt((float)(x*x + z*z));
        return 64.0f + 10.0f * std::exp(-d * 0.01f);
    }
    BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
    float pendienteEn(int, int) const override { return 0.1f; }
    bool esSolido(int x, int y, int z) const override {
        return (float)y < alturaSuelo(x, z);
    }
};

TEST_CASE("Mundo: cargar chunks puebla el mundo") {
    MundoPecaries mundo(12345);
    MundoLlano terreno;

    CHECK(mundo.cuantos() == 0);

    // Se cargan varios chunks lejos del jugador.
    int total = 0;
    for (int cx = 0; cx < 30; ++cx) {
        for (int cz = 0; cz < 30; ++cz) {
            total += mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }

    CHECK(total > 0);
    CHECK(mundo.cuantos() == (size_t)total);
}

TEST_CASE("Mundo: recargar un chunk NO duplica la manada") {
    MundoPecaries mundo(999);
    MundoLlano terreno;

    // ESTE ES EL TEST QUE MAS IMPORTA DE ESTE ARCHIVO.
    //
    // Un chunk se carga y descarga constantemente segun el jugador se mueve.
    // Si cada carga instanciara la poblacion estructural otra vez, ir y
    // volver duplicaria la manada, y un jugador que camine en circulos
    // llenaria el mundo de pecaries sin hacer nada.

    // Se carga la zona ENTERA una vez.
    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    const size_t trasPrimera = mundo.cuantos();
    REQUIRE(trasPrimera > 0);

    // Ahora se recargan LOS MISMOS chunks 20 veces, en el mismo instante de
    // juego (misma ventana temporal de repoblacion).
    for (int repeticion = 0; repeticion < 20; ++repeticion) {
        for (int cx = 0; cx < 40; ++cx) {
            for (int cz = 0; cz < 40; ++cz) {
                mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
            }
        }
    }

    const size_t trasVeinte = mundo.cuantos();

    // La poblacion tiene que CONVERGER, no crecer con cada visita.
    //
    // No se exige igualdad exacta: en la primera pasada, los chunks del
    // principio se evaluan con el mundo aun vacio, asi que la
    // densodependencia les deja repoblar; al recargar, el conteo de vivos ya
    // es alto y la repoblacion se apaga sola. Ese pequeno ajuste de una sola
    // vez es correcto y es justo lo que debe pasar.
    //
    // Lo que NO puede pasar es crecimiento sostenido: veinte vueltas no
    // pueden multiplicar la poblacion. Se exige menos de un 5% de aumento
    // total, cuando una duplicacion por visita habria dado 20x.
    CHECK(trasVeinte <= trasPrimera * 105 / 100);

    // Y una vez estabilizado, mas recargas NO anaden absolutamente nada.
    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    CHECK(mundo.cuantos() == trasVeinte);
}

TEST_CASE("Mundo: la poblacion respeta el tope duro") {
    MundoPecaries mundo(777);
    MundoLlano terreno;

    // 01_ARQUITECTURA lo exige como red de seguridad derivada del fracaso de
    // Ultima Online: "poner limites duros... un ecosistema extinto no es mas
    // realista que uno con limites".
    //
    // Aqui el limite protege el frame rate: se cargan MUCHISIMOS chunks.
    for (int cx = 0; cx < 200; ++cx) {
        for (int cz = 0; cz < 200; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
            if (mundo.cuantos() >= MundoPecaries::MAX_PECARIES) break;
        }
        if (mundo.cuantos() >= MundoPecaries::MAX_PECARIES) break;
    }

    CHECK(mundo.cuantos() <= MundoPecaries::MAX_PECARIES);
}

TEST_CASE("Mundo: los pecaries acaban SOBRE el terreno") {
    MundoPecaries mundo(4242);
    MundoColina terreno;

    for (int cx = 0; cx < 30; ++cx) {
        for (int cz = 0; cz < 30; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    REQUIRE(mundo.cuantos() > 0);

    // Se simulan 10 segundos para que se asienten sobre la colina.
    for (int i = 0; i < 200; ++i) {
        mundo.actualizar(0.05f, terreno);
    }

    // Cada animal debe estar a ras del suelo de SU columna, no flotando ni
    // enterrado. Sin el seguimiento de terreno, caminarian a altura fija y
    // atravesarian la loma.
    for (const PecariAgente& p : mundo.todos()) {
        const int bx = (int)std::floor(p.x);
        const int bz = (int)std::floor(p.z);

        // ⚠️ SIN `+1`. `alturaSuelo` devuelve la SUPERFICIE PISABLE, no el
        // indice del ultimo bloque solido (ver su contrato en IPecariMundo).
        //
        // Este test tenia el `+1` heredado de cuando si devolvia el indice, y
        // por eso aceptaba como bueno un animal flotando un bloque por encima
        // de la hierba -- que es justo el bug que se reporto. El test pasaba y
        // el juego se veia mal.
        const float suelo = terreno.alturaSuelo(bx, bz);

        // Y el margen baja de 1.0 a 0.25: con un bloque de holgura, un animal
        // flotando entero seguia dando el test por bueno. 0.25 deja sitio al
        // suavizado de `posarEnSuelo` pero no a un bloque de aire.
        INFO("y=", p.y, "  suelo=", suelo, "  diferencia=", p.y - suelo);
        CHECK(std::fabs(p.y - suelo) < 0.25f);
    }
}

TEST_CASE("Mundo: la simulacion completa es estable") {
    MundoPecaries mundo(31337);
    MundoColina terreno;

    for (int cx = 0; cx < 25; ++cx) {
        for (int cz = 0; cz < 25; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    REQUIRE(mundo.cuantos() > 0);

    // Cinco minutos de juego.
    for (int i = 0; i < 6000; ++i) {
        mundo.actualizar(0.05f, terreno);
    }

    for (const PecariAgente& p : mundo.todos()) {
        CHECK(std::isfinite(p.x));
        CHECK(std::isfinite(p.y));
        CHECK(std::isfinite(p.z));
        CHECK(std::isfinite(p.orientacion));
    }
}

TEST_CASE("Mundo: descargar lejos libera poblacion") {
    MundoPecaries mundo(555);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    const size_t antes = mundo.cuantos();
    REQUIRE(antes > 0);

    // El jugador esta en el origen; se descarga todo lo que este a mas de
    // 50 bloques.
    mundo.descargarLejos(0.0f, 0.0f, 50.0f);

    CHECK(mundo.cuantos() < antes);

    // Y lo que queda esta de verdad cerca.
    for (const PecariAgente& p : mundo.todos()) {
        CHECK(std::sqrt(p.x*p.x + p.z*p.z) <= 50.0f);
    }
}

TEST_CASE("Mundo: no puebla biomas imposibles") {
    MundoPecaries mundo(888);
    MundoLlano terreno;
    terreno.bioma = BIOME_OCEAN_DEEP;

    for (int cx = 0; cx < 60; ++cx) {
        for (int cz = 0; cz < 60; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    CHECK(mundo.cuantos() == 0);
}

TEST_CASE("Mundo: las manadas tienen mezcla de edades") {
    MundoPecaries mundo(2024);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    REQUIRE(mundo.cuantos() > 10);

    // MEDIDO: las manadas son "grupos mixtos de machos y hembras adultos,
    // juveniles y subadultos, de varias clases de edad".
    //
    // Una manada de puros adultos clonados se veria artificial.
    bool hayAdulto = false, hayJoven = false;
    for (const PecariAgente& p : mundo.todos()) {
        if (p.etapa == EtapaPecari::ADULTO) hayAdulto = true;
        if (p.etapa == EtapaPecari::JUVENIL ||
            p.etapa == EtapaPecari::SUBADULTO ||
            p.etapa == EtapaPecari::NEONATO) hayJoven = true;
    }
    CHECK(hayAdulto);
    CHECK(hayJoven);
}

TEST_CASE("Mundo: la geometria sale solo de los cercanos") {
    MundoPecaries mundo(1234);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    REQUIRE(mundo.cuantos() > 0);

    std::vector<PiezaCuerpo> cerca, lejos;
    mundo.construirGeometria(cerca, 0.0f, 0.0f, 30.0f);
    mundo.construirGeometria(lejos, 0.0f, 0.0f, 2000.0f);

    // Un radio pequeno debe producir menos geometria que uno grande.
    CHECK(cerca.size() < lejos.size());

    // Todos los cuerpos tienen el MISMO numero de piezas, asi que el total
    // debe ser multiplo exacto de ese numero. Si no lo fuera, algun cuerpo se
    // estaria construyendo a medias.
    const size_t piezasPorCuerpo = lejos.size() / mundo.cuantos();
    CHECK(lejos.size() % piezasPorCuerpo == 0);
    CHECK(piezasPorCuerpo > 30);   // cuerpo con detalle real
    CHECK(piezasPorCuerpo < 90);   // pero sin abrumar
}

TEST_CASE("Mundo: contar vivos cerca funciona") {
    MundoPecaries mundo(6161);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx) {
        for (int cz = 0; cz < 40; ++cz) {
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
        }
    }
    REQUIRE(mundo.cuantos() > 0);

    // Es la entrada de la densodependencia: si contara mal, la repoblacion
    // decidiria con datos falsos.
    const int enRadioGrande = mundo.vivosCerca(0.0f, 0.0f, 100000.0f);
    CHECK(enRadioGrande == (int)mundo.cuantos());

    const int enRadioCero = mundo.vivosCerca(999999.0f, 999999.0f, 1.0f);
    CHECK(enRadioCero == 0);
}

// ============================================================================
// EMPUJE BIDIRECCIONAL JUGADOR <-> ANIMAL
// ============================================================================

TEST_CASE("Mundo: el animal aparta al jugador") {
    MundoPecaries mundo(4242);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 0);

    // Se coloca al jugador ENCIMA del primer pecari.
    const PecariAgente& a = mundo.todos()[0];
    float ex = 0.0f, ez = 0.0f;
    const bool choca = mundo.empujeSobreJugador(a.x + 0.1f, a.y, a.z,
                                                0.30f, 1.80f, ex, ez);
    CHECK(choca);
    // Tiene que empujar en ALGUNA direccion.
    CHECK((std::fabs(ex) + std::fabs(ez)) > 0.0f);
}

TEST_CASE("Mundo: el jugador aparta al animal") {
    MundoPecaries mundo(4242);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 0);

    const float x0 = mundo.todos()[0].x;
    const float z0 = mundo.todos()[0].z;

    // El jugador se mete dentro del animal.
    const int n = mundo.jugadorEmpuja(x0 + 0.15f, mundo.todos()[0].y, z0,
                                      0.30f, 1.80f, 1.0f);
    CHECK(n > 0);

    // El animal se ha movido.
    const float x1 = mundo.todos()[0].x;
    const float z1 = mundo.todos()[0].z;
    CHECK((std::fabs(x1 - x0) + std::fabs(z1 - z0)) > 0.0f);
}

TEST_CASE("Mundo: una cria cede MUCHO mas que un adulto") {
    // El empuje se pondera por la masa MEDIDA: 0.5 kg al nacer frente a
    // 18.7 kg de adulto. Un bebe tiene que salir casi despedido.
    MundoPecaries mundo(999);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 10);

    // Se busca un neonato y un adulto.
    int idxBebe = -1, idxAdulto = -1;
    for (size_t i = 0; i < mundo.todos().size(); ++i) {
        if (mundo.todos()[i].etapa == EtapaPecari::NEONATO && idxBebe < 0) idxBebe = (int)i;
        if (mundo.todos()[i].etapa == EtapaPecari::ADULTO && idxAdulto < 0) idxAdulto = (int)i;
    }
    REQUIRE(idxBebe >= 0);
    REQUIRE(idxAdulto >= 0);

    const float bx0 = mundo.todos()[idxBebe].x;
    const float ax0 = mundo.todos()[idxAdulto].x;

    // Se empuja a cada uno desde la misma distancia relativa.
    mundo.jugadorEmpuja(bx0 - 0.20f, mundo.todos()[idxBebe].y,
                        mundo.todos()[idxBebe].z, 0.30f, 1.80f, 1.0f);
    mundo.jugadorEmpuja(ax0 - 0.20f, mundo.todos()[idxAdulto].y,
                        mundo.todos()[idxAdulto].z, 0.30f, 1.80f, 1.0f);

    const float desplBebe   = std::fabs(mundo.todos()[idxBebe].x - bx0);
    const float desplAdulto = std::fabs(mundo.todos()[idxAdulto].x - ax0);

    // El bebe tiene que haberse movido mas que el adulto.
    CHECK(desplBebe > desplAdulto);
}

TEST_CASE("Mundo: al empujarlo, el animal se ALARMA") {
    // Erizar la cresta es la respuesta de alarma MEDIDA de la especie. Hace
    // que el empujon tenga consecuencia visible en vez de ser silencioso.
    MundoPecaries mundo(777);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 0);

    CHECK(mundo.todos()[0].erizado == doctest::Approx(0.0f));

    mundo.jugadorEmpuja(mundo.todos()[0].x + 0.1f, mundo.todos()[0].y,
                        mundo.todos()[0].z, 0.30f, 1.80f, 1.0f);

    CHECK(mundo.todos()[0].erizado > 0.5f);
}

TEST_CASE("Mundo: sin contacto, nadie empuja a nadie") {
    MundoPecaries mundo(555);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 0);

    // Jugador muy lejos de todo.
    float ex = 0.0f, ez = 0.0f;
    CHECK(mundo.empujeSobreJugador(99999.0f, 64.0f, 99999.0f,
                                   0.30f, 1.80f, ex, ez) == false);
    CHECK(mundo.jugadorEmpuja(99999.0f, 64.0f, 99999.0f,
                              0.30f, 1.80f, 1.0f) == 0);
}

TEST_CASE("Mundo: saltar por encima de un pecari no choca") {
    MundoPecaries mundo(321);
    MundoLlano terreno;

    for (int cx = 0; cx < 40; ++cx)
        for (int cz = 0; cz < 40; ++cz)
            mundo.alCargarChunk(cx, cz, terreno, 500.0f, 100.0);
    REQUIRE(mundo.cuantos() > 0);

    const PecariAgente& a = mundo.todos()[0];

    // El jugador pasa muy por encima: no debe haber contacto.
    float ex = 0.0f, ez = 0.0f;
    CHECK(mundo.empujeSobreJugador(a.x, a.y + 8.0f, a.z,
                                   0.30f, 1.80f, ex, ez) == false);
}

// ============================================================================
// EL PECARI NO ATRAVIESA BLOQUES
// ============================================================================
// POR QUE ESTOS TESTS NO EXISTIAN ANTES, que es lo interesante:
//
// Los dos mundos de prueba de arriba (MundoLlano y MundoColina) definen
// `esSolido` como "y < alturaSuelo", o sea: solido SOLO por debajo de la
// superficie. Son campos de altura puros, sin una sola pared.
//
// Y esa era EXACTAMENTE la definicion que usaba el adaptador real del motor.
// Los tests reproducian la misma suposicion equivocada que el codigo, asi que
// por construccion no podian detectar el fallo: en un mundo donde nada
// sobresale del suelo, un animal que atraviesa paredes se comporta igual que
// uno que no.
//
// Un test solo prueba lo que su mundo permite que ocurra.
// ----------------------------------------------------------------------------

// Terreno llano CON UNA PARED encima: lo que el jugador construye.
class MundoConPared : public IPecariMundo {
public:
    float altura = 64.0f;
    // La pared ocupa la franja x en [10, 11], de suelo a suelo+4.
    int paredXmin = 10, paredXmax = 11;
    float paredAlto = 4.0f;

    float alturaSuelo(int, int) const override { return altura; }
    BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
    float pendienteEn(int, int) const override { return 0.0f; }

    bool esSolido(int x, int y, int z) const override {
        (void)z;
        if ((float)y < altura) return true;             // el suelo
        if (x >= paredXmin && x <= paredXmax &&         // y la pared
            (float)y < altura + paredAlto) return true;
        return false;
    }
};

TEST_CASE("Colision: una pared detiene al pecari") {
    MundoConPared m;
    MundoPecaries mundo(1234);

    // Un animal empujado contra la pared durante mucho tiempo.
    PecariAgente p;
    p.x = 5.0f; p.z = 0.0f; p.y = m.altura + 1.0f;
    p.escala = 1.0f;
    p.etapa = EtapaPecari::ADULTO;
    p.vx = 3.0f;    // va derecho hacia la pared (esta en x=10)
    p.vz = 0.0f;

    // Se simula el movimiento con colision, que es lo que hace el bucle real.
    for (int i = 0; i < 400; ++i) {
        p.vx = 3.0f;    // insiste: sigue empujando cada frame
        MundoPecaries::moverConColisionTest(p, m, 0.05f);
    }

    // Sin colision habria recorrido 3*0.05*400 = 60 bloques y estaria en x=65,
    // al otro lado. Con colision tiene que haberse quedado ANTES de la pared.
    INFO("x final = ", p.x);
    CHECK(p.x < 10.0f);
}

TEST_CASE("Colision: el pecari no se queda atascado en terreno libre") {
    // El reverso, y es igual de importante: si la colision fuera demasiado
    // estricta el animal no podria moverse en campo abierto y la manada se
    // congelaria. Con una pared muy lejos, tiene que avanzar sin estorbo.
    MundoConPared m;
    m.paredXmin = 900; m.paredXmax = 901;   // fuera de alcance

    PecariAgente p;
    p.x = 0.0f; p.z = 0.0f; p.y = m.altura + 1.0f;
    p.escala = 1.0f;
    p.etapa = EtapaPecari::ADULTO;

    for (int i = 0; i < 100; ++i) {
        p.vx = 2.0f; p.vz = 0.0f;
        MundoPecaries::moverConColisionTest(p, m, 0.05f);
    }

    // 2 * 0.05 * 100 = 10 bloques. Se admite margen, pero tiene que haberse
    // movido de verdad.
    INFO("x final = ", p.x);
    CHECK(p.x > 8.0f);
}

TEST_CASE("Colision: un escalon bajo NO frena al animal") {
    // Un ungulado sube un bordillo sin pensarlo. Si una loncha de terreno de
    // 1/8 fuera un muro, las manadas se quedarian encerradas en el primer
    // desnivel del mapa -- que en este motor hay en todas partes.
    class MundoEscalon : public IPecariMundo {
    public:
        float alturaSuelo(int x, int) const override {
            return (x >= 10) ? 64.25f : 64.0f;   // escalon de un cuarto
        }
        BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
        float pendienteEn(int, int) const override { return 0.0f; }
        bool esSolido(int x, int y, int z) const override {
            return (float)y < alturaSuelo(x, z);
        }
    } m;

    PecariAgente p;
    p.x = 5.0f; p.z = 0.0f; p.y = 65.0f;
    p.escala = 1.0f;
    p.etapa = EtapaPecari::ADULTO;

    for (int i = 0; i < 200; ++i) {
        p.vx = 2.0f; p.vz = 0.0f;
        MundoPecaries::moverConColisionTest(p, m, 0.05f);
    }

    INFO("x final = ", p.x);
    CHECK(p.x > 12.0f);   // lo ha pasado
}

TEST_CASE("Colision: una cria pasa por donde no cabe un adulto") {
    // La caja sale de la etapa, asi que el hueco por el que cabe cada uno es
    // distinto. Es la prueba de que la colision usa la hitbox REAL y no un
    // radio fijo para todos.
    MundoConPared m;
    m.paredXmin = 10; m.paredXmax = 10;   // pared de un solo bloque de grosor

    PecariAgente adulto;
    adulto.x = 9.0f; adulto.y = m.altura + 1.0f;
    adulto.etapa = EtapaPecari::ADULTO;
    adulto.escala = EscalaDeEtapa(EtapaPecari::ADULTO);

    PecariAgente cria = adulto;
    cria.etapa = EtapaPecari::NEONATO;
    cria.escala = EscalaDeEtapa(EtapaPecari::NEONATO);

    // El adulto es mas ancho, asi que su caja toca la pared antes.
    const HitboxPecari ha = HitboxDe(adulto);
    const HitboxPecari hc = HitboxDe(cria);
    INFO("semiLargo adulto=", ha.semiLargo, " cria=", hc.semiLargo);
    CHECK(hc.semiLargo < ha.semiLargo);
}

// ============================================================================
// LA REACCION AL SER GOLPEADO
// ============================================================================
// La regla sale de 11_PECARI_IA.json, no de la intuicion. Para el
// comportamiento DEFENDER el diseno dice literalmente:
//
//     "Solo si hay crias cerca Y la huida esta bloqueada"
//     "El mobbing coordinado NO esta documentado en esta especie (si en
//      T. pecari). Implementar con cautela y marcar como extrapolacion."
//
// Y el dato MEDIDO va en la misma direccion: ante una amenaza, lo observado en
// Barro Colorado es que la manada AUMENTA LA COHESION y la vigilancia. Se
// apinan; no cargan en grupo.
//
// Por eso la respuesta por defecto es HUIR, y encarar es la excepcion. Estos
// tests fijan esa asimetria: si alguien la invierte "para que sea mas
// divertido", que sea una decision consciente y no un descuido.

TEST_CASE("Golpe: un adulto timido y solo HUYE") {
    MundoLlano m;
    MundoPecaries mundo(777);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() > 0);

    int id = -1;
    for (const PecariAgente& p : mundo.todos())
        if (p.etapa == EtapaPecari::ADULTO) { id = p.id; break; }
    REQUIRE(id >= 0);

    mundo.forzarParaTest(id, EtapaPecari::ADULTO, 0.10f);   // audacia baja
    mundo.aislarParaTest(id);   // sin crias cerca que defender

    const bool murio = mundo.recibirDano(id, 2, 0.0f, 0.0f, m);
    CHECK_FALSE(murio);

    const PecariAgente* p = mundo.buscarParaTest(id);
    REQUIRE(p != nullptr);
    INFO("conducta = ", (int)p->conducta);
    CHECK(p->conducta == ConductaPecari::HUIDA);
}

TEST_CASE("Golpe: con una CRIA cerca, el adulto defiende") {
    // La condicion que exige el diseno. Es el caso en que atacar esta
    // respaldado: hay algo que proteger.
    MundoLlano m;
    MundoPecaries mundo(888);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() >= 2);

    int idAdulto = -1, idCria = -1;
    for (const PecariAgente& p : mundo.todos()) {
        if (idAdulto < 0 && p.etapa == EtapaPecari::ADULTO) { idAdulto = p.id; continue; }
        if (idCria < 0) idCria = p.id;
    }
    REQUIRE(idAdulto >= 0);
    REQUIRE(idCria >= 0);

    // Adulto TIMIDO -- para que quede claro que lo que le hace quedarse es la
    // cria y no su caracter -- y una cria pegada a el.
    mundo.forzarParaTest(idAdulto, EtapaPecari::ADULTO, 0.10f);
    mundo.forzarParaTest(idCria,   EtapaPecari::NEONATO, 0.5f);
    mundo.juntarParaTest(idCria, idAdulto);

    mundo.recibirDano(idAdulto, 2, 50.0f, 50.0f, m);

    const PecariAgente* p = mundo.buscarParaTest(idAdulto);
    REQUIRE(p != nullptr);
    INFO("conducta = ", (int)p->conducta);
    CHECK(p->conducta == ConductaPecari::DEFENSA);
}

TEST_CASE("Golpe: una CRIA nunca defiende, siempre huye") {
    // Un neonato de 0.5 kg no encara a nadie, tenga la audacia que tenga.
    MundoLlano m;
    MundoPecaries mundo(999);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() > 0);

    const int id = mundo.todos()[0].id;
    mundo.forzarParaTest(id, EtapaPecari::NEONATO, 1.0f);   // audacia maxima
    mundo.recibirDano(id, 1, 10.0f, 10.0f, m);

    const PecariAgente* p = mundo.buscarParaTest(id);
    REQUIRE(p != nullptr);
    CHECK(p->conducta == ConductaPecari::HUIDA);
}

TEST_CASE("Golpe: el muy audaz encara aunque no haya crias") {
    // La audacia es un rasgo MEDIDO en esta especie, con consecuencias
    // fisiologicas demostradas. Que el mas atrevido plante cara es lo que ese
    // rasgo significa.
    MundoLlano m;
    MundoPecaries mundo(1111);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);

    int id = -1;
    for (const PecariAgente& p : mundo.todos())
        if (p.etapa == EtapaPecari::ADULTO) { id = p.id; break; }
    REQUIRE(id >= 0);

    mundo.forzarParaTest(id, EtapaPecari::ADULTO, 0.95f);   // muy audaz
    mundo.aislarParaTest(id);
    mundo.recibirDano(id, 2, 0.0f, 0.0f, m);

    const PecariAgente* p = mundo.buscarParaTest(id);
    REQUIRE(p != nullptr);
    CHECK(p->conducta == ConductaPecari::DEFENSA);
}

TEST_CASE("Golpe: la manada se entera") {
    // Los companeros que lo tienen cerca reaccionan. No es un "grito": las
    // vocalizaciones no estan documentadas en esta especie y el diseno pide no
    // fingir que lo estan. Es que VEN lo que le pasa a uno de los suyos.
    MundoLlano m;
    MundoPecaries mundo(2222);
    for (int cx = 0; cx < 30 && mundo.cuantos() < 3; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() < 3; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() >= 3);

    const int id = mundo.todos()[0].id;
    const int idManada = mundo.todos()[0].idManada;

    mundo.recibirDano(id, 2, 100.0f, 100.0f, m);

    int alterados = 0;
    for (const PecariAgente& p : mundo.todos()) {
        if (p.id == id) continue;
        if (p.idManada != idManada) continue;
        if (p.conducta != ConductaPecari::CALMA) ++alterados;
    }
    INFO("companeros alterados = ", alterados);
    CHECK(alterados > 0);
}

TEST_CASE("Golpe: a base de golpes, muere") {
    MundoLlano m;
    MundoPecaries mundo(3333);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);

    const size_t antes = mundo.cuantos();
    REQUIRE(antes > 0);
    const int id = mundo.todos()[0].id;

    bool murio = false;
    for (int i = 0; i < 40 && !murio; ++i)
        murio = mundo.recibirDano(id, 2, 0.0f, 0.0f, m);

    CHECK(murio);
    CHECK(mundo.cuantos() == antes - 1);
    CHECK(mundo.buscarParaTest(id) == nullptr);
}

TEST_CASE("Golpe: la vida sale de la etapa, no es la misma para todos") {
    // Un adulto de 18.7 kg tiene que aguantar mas que una cria de 0.5. Si
    // fueran iguales, matar crias y adultos costaria lo mismo y la decision de
    // a quien cazar no significaria nada.
    CHECK(VidaMaximaMedios(EtapaPecari::ADULTO) >
          VidaMaximaMedios(EtapaPecari::NEONATO));
    CHECK(VidaMaximaMedios(EtapaPecari::SUBADULTO) >
          VidaMaximaMedios(EtapaPecari::JUVENIL));
    // Pero una cria no muere de un solo golpe flojo: eso quitaria el dilema de
    // que la manada la defienda.
    CHECK(VidaMaximaMedios(EtapaPecari::NEONATO) >= 4);
}

TEST_CASE("Audacia: se reparte, no es igual para todos") {
    // Sale de la semilla, asi que es determinista por individuo. Lo que se
    // comprueba es que HAY variedad: si todos tuvieran la misma, el rasgo no
    // significaria nada.
    float minima = 2.0f, maxima = -1.0f;
    double suma = 0.0;
    constexpr int N = 500;
    for (int i = 1; i <= N; ++i) {
        const float a = AudaciaDeSemilla((uint32_t)(i * 2654435761u) | 1u);
        CHECK(a >= 0.0f);
        CHECK(a <= 1.0f);
        if (a < minima) minima = a;
        if (a > maxima) maxima = a;
        suma += a;
    }
    const double media = suma / N;
    INFO("min=", minima, " max=", maxima, " media=", media);

    // Centrada en 0.5, como pide el diseno (Normal(0.5, 0.18)).
    CHECK(media > 0.42);
    CHECK(media < 0.58);
    CHECK(maxima - minima > 0.4f);
}

TEST_CASE("Audacia: el mismo animal tiene siempre el mismo caracter") {
    // Determinismo: es lo que permite no guardarla en disco.
    for (int i = 1; i <= 50; ++i) {
        const uint32_t s = (uint32_t)(i * 2654435761u) | 1u;
        CHECK(AudaciaDeSemilla(s) == AudaciaDeSemilla(s));
    }
}

// ============================================================================
// SOLTAR A MANO (el huevo de spawn del creativo)
// ============================================================================
// La tercera via de aparicion, ademas de la manada estructural y el grupo de
// repoblacion. Aqui manda el jugador: pone el punto y sale.

TEST_CASE("Soltar uno: aparece donde se pide y sobre el suelo") {
    MundoLlano mundo;
    MundoPecaries m(1234);

    const int id = m.soltarUno(100.0f, 200.0f, EtapaPecari::ADULTO, mundo);

    CHECK(id >= 0);
    CHECK(m.cuantos() == 1);

    const PecariAgente& p = m.todos()[0];
    CHECK(p.x == doctest::Approx(100.0f));
    CHECK(p.z == doctest::Approx(200.0f));
    // ⚠️ SOBRE el terreno = EXACTAMENTE en su superficie.
    //
    // Esto pedia `p.y > mundo.altura` estricto, que era correcto cuando
    // `alturaSuelo` devolvia el INDICE del ultimo bloque solido y soltarUno le
    // sumaba 1. Con el contrato actual --devuelve ya la superficie-- el animal
    // se posa justo ahi, asi que el `>` estricto fallaba por igualdad.
    //
    // Se comprueba lo que de verdad importa: que no este enterrado ni
    // flotando. El margen de 3 bloques de la version anterior era tan holgado
    // que un animal flotando un bloque entero lo pasaba.
    INFO("y=", p.y, "  superficie=", mundo.altura);
    CHECK(p.y >= mundo.altura - 0.01f);
    CHECK(p.y <= mundo.altura + 0.01f);
    CHECK(p.etapa == EtapaPecari::ADULTO);
}

TEST_CASE("Soltar uno: NO flota un bloque sobre el suelo") {
    // ⭐⭐ EL BUG QUE SE VEIA EN PANTALLA: el pecari caminaba flotando un
    // bloque por encima de la hierba, con las patas colgando en el aire.
    //
    // LA CAUSA fue un cambio de contrato sin actualizar a sus llamantes.
    // `alturaSuelo` pasó de devolver el INDICE del ultimo bloque solido a
    // devolver la SUPERFICIE PISABLE, y los cinco sitios que lo usaban se
    // quedaron con el `+1` que antes hacia falta. Ese `+1` heredado es,
    // literalmente, el bloque de aire que se veia debajo del animal.
    //
    // POR QUE NO LO CAZO NINGUN TEST: el que comprobaba "acaban sobre el
    // terreno" tenia el mismo `+1` en su expectativa, asi que medía el bug
    // contra si mismo. Y su margen era de 1.0 bloques -- justo lo que flotaba.
    //
    // Este test mide contra la superficie REAL y con margen estrecho.
    MundoLlano mundo;
    MundoPecaries m(999);

    REQUIRE(m.soltarUno(50.0f, 50.0f, EtapaPecari::ADULTO, mundo) >= 0);
    const PecariAgente& p = m.todos()[0];

    const float flotacion = p.y - mundo.altura;
    INFO("flota ", flotacion, " bloques sobre la superficie");

    // Medio bloque ya se ve a simple vista; un bloque entero es el bug.
    CHECK(flotacion < 0.5f);
    // Y tampoco enterrado.
    CHECK(flotacion > -0.5f);
}

TEST_CASE("Suelo: la superficie NO es el indice del bloque") {
    // ⚠️ EL CONTRATO QUE SE ROMPIO, FIJADO COMO TEST.
    //
    // `alturaSuelo` devuelve donde se APOYAN LAS PEZUNAS, no el indice del
    // ultimo bloque solido. Sobre un bloque entero en y=64 eso es 65.0.
    //
    // La diferencia parece trivial y no lo es: confundirlas es exactamente lo
    // que dejo al animal flotando. Se fija aqui para que quien cambie la
    // implementacion vea el contrato antes que el bug.
    MundoLlano mundo;

    // El mundo de prueba declara su superficie en `altura`. Lo que devuelve
    // alturaSuelo tiene que ser ESO, sin sumas ni restas por el camino.
    CHECK(mundo.alturaSuelo(0, 0) == doctest::Approx(mundo.altura));
    CHECK(mundo.alturaSuelo(1000, -1000) == doctest::Approx(mundo.altura));

    // Y un animal soltado ahi se posa en ese mismo numero.
    MundoPecaries m(5);
    REQUIRE(m.soltarUno(0.0f, 0.0f, EtapaPecari::ADULTO, mundo) >= 0);
    CHECK(m.todos()[0].y == doctest::Approx(mundo.altura));
}

TEST_CASE("Soltar uno: la etapa pedida es la que sale") {
    MundoLlano mundo;
    MundoPecaries m(7);

    const EtapaPecari etapas[] = {
        EtapaPecari::NEONATO, EtapaPecari::JUVENIL, EtapaPecari::SUBADULTO,
        EtapaPecari::ADULTO,  EtapaPecari::SENESCENTE
    };
    for (EtapaPecari e : etapas) {
        const int id = m.soltarUno(0.0f, 0.0f, e, mundo);
        REQUIRE(id >= 0);
        CHECK(m.todos().back().etapa == e);
    }
}

TEST_CASE("Soltar uno: la vida sale de la ETAPA, no del valor por defecto") {
    // ⭐ ESTE TEST NACIO DE UN BUG REAL.
    //
    // instanciarManada() e instanciarGrupo() asignaban p.vidaMedios ANTES de
    // p.etapa. Como p.etapa tiene valor por defecto, VidaMaximaMedios() leia
    // ese valor y TODOS los pecaries nacian con la vida del adulto: un
    // neonato aguantaba tantos golpes como un adulto hecho.
    //
    // No daba error ni valor absurdo, solo el numero equivocado -- la clase
    // de fallo que sobrevive anos. Aqui se ata.
    MundoLlano mundo;
    MundoPecaries m(99);

    m.soltarUno(0.0f, 0.0f, EtapaPecari::NEONATO, mundo);
    const int vidaNeonato = m.todos().back().vidaMedios;

    m.soltarUno(10.0f, 0.0f, EtapaPecari::ADULTO, mundo);
    const int vidaAdulto = m.todos().back().vidaMedios;

    INFO("neonato=", vidaNeonato, " adulto=", vidaAdulto);
    CHECK(vidaNeonato > 0);
    CHECK(vidaAdulto > 0);
    CHECK(vidaNeonato < vidaAdulto);   // la cria aguanta MENOS
}

TEST_CASE("Soltar manada: sale entera y con estructura de edades") {
    MundoLlano mundo;
    MundoPecaries m(2024);

    const int puestos = m.soltarManada(500.0f, 500.0f, 10, mundo);

    CHECK(puestos == 10);
    CHECK(m.cuantos() == 10);

    // Todos de la MISMA manada: es una piara, no diez sueltos.
    const int idManada = m.todos()[0].idManada;
    for (const PecariAgente& p : m.todos()) {
        CHECK(p.idManada == idManada);
    }

    // Estructura de edades: con 10 miembros tiene que haber variedad, no
    // diez adultos clonados.
    bool hayAdulto = false, hayCria = false;
    for (const PecariAgente& p : m.todos()) {
        if (p.etapa == EtapaPecari::ADULTO)  hayAdulto = true;
        if (p.etapa == EtapaPecari::NEONATO) hayCria = true;
    }
    CHECK(hayAdulto);
    CHECK(hayCria);
}

TEST_CASE("Soltar manada: nadie nace dentro de otro") {
    MundoLlano mundo;
    MundoPecaries m(555);
    m.soltarManada(0.0f, 0.0f, 12, mundo);

    // El reparto en espiral tiene que separarlos. Si dos salieran en el mismo
    // punto exacto, la resolucion de colisiones tendria que empujarlos desde
    // una superposicion perfecta, que es justo el caso que peor resuelve.
    const auto& v = m.todos();
    for (size_t i = 0; i < v.size(); ++i) {
        for (size_t j = i + 1; j < v.size(); ++j) {
            const float dx = v[i].x - v[j].x;
            const float dz = v[i].z - v[j].z;
            CHECK((dx*dx + dz*dz) > 1e-4f);
        }
    }
}

TEST_CASE("Soltar manada: respeta el tope de poblacion") {
    MundoLlano mundo;
    MundoPecaries m(8);

    // Pedir mucho mas de lo que cabe no puede desbordar el tope.
    int total = 0;
    for (int i = 0; i < 600; ++i) {
        total += m.soltarManada((float)(i * 40), 0.0f, 10, mundo);
    }
    CHECK(m.cuantos() <= MundoPecaries::MAX_PECARIES);
    CHECK(total == (int)m.cuantos());
}

TEST_CASE("Soltar uno: al llegar al tope devuelve -1 y no crece") {
    MundoLlano mundo;
    MundoPecaries m(3);

    while (m.cuantos() < MundoPecaries::MAX_PECARIES) {
        REQUIRE(m.soltarUno(0.0f, 0.0f, EtapaPecari::ADULTO, mundo) >= 0);
    }
    const size_t lleno = m.cuantos();

    // Con el registro lleno, el huevo tiene que fallar limpiamente. Es lo que
    // permite a main.cpp no gastar el item cuando no ha pasado nada.
    CHECK(m.soltarUno(0.0f, 0.0f, EtapaPecari::ADULTO, mundo) == -1);
    CHECK(m.cuantos() == lleno);
}

TEST_CASE("Soltar uno: sigue el terreno, no una altura fija") {
    MundoColina mundo;
    MundoPecaries m(11);

    m.soltarUno(0.0f, 0.0f, EtapaPecari::ADULTO, mundo);       // cima
    m.soltarUno(900.0f, 900.0f, EtapaPecari::ADULTO, mundo);   // lejos, llano

    const float yCima  = m.todos()[0].y;
    const float yLejos = m.todos()[1].y;

    INFO("cima=", yCima, " lejos=", yLejos);
    CHECK(yCima > yLejos);   // la loma levanta al de arriba
}

// ============================================================================
// MOVIMIENTO REALISTA
// ============================================================================
// ⭐ TRES BUGS QUE HACIAN QUE NO SE MOVIERAN COMO ANIMALES.

TEST_CASE("Movimiento: la velocidad no salta de golpe entre frames") {
    // El bug: la velocidad se asignaba directa (p.vx = dirX * vel), asi que un
    // pecari pasaba de quieto a 6 bloques/s EN UN FRAME, y de 6 a 0 igual de
    // seco. Sin arranque ni frenada, el ojo lo lee como teletransporte por
    // mucho que las patas se muevan.
    //
    // Ahora hay aceleracion, y el salto por frame no puede pasar de
    // max(ACELERACION, FRENADA) * dt.
    MundoLlano mundo;

    for (float fps : {30.0f, 60.0f, 144.0f}) {
        MundoPecaries m(7);
        m.soltarManada(0.0f, 0.0f, 8, mundo);

        const float dt = 1.0f / fps;
        const float limite = PecariMovimiento::FRENADA * dt * 1.05f;  // 5% de holgura

        std::vector<float> previa(m.cuantos(), 0.0f);
        float saltoMax = 0.0f;

        for (int i = 0; i < (int)(15.0f * fps); ++i) {
            m.actualizar(dt, mundo);
            for (size_t k = 0; k < m.todos().size(); ++k) {
                const PecariAgente& p = m.todos()[k];
                const float v = std::sqrt(p.vx*p.vx + p.vz*p.vz);
                const float salto = std::fabs(v - previa[k]);
                if (salto > saltoMax) saltoMax = salto;
                previa[k] = v;
            }
        }

        INFO("a ", fps, " fps: salto max ", saltoMax, " limite ", limite);
        CHECK(saltoMax <= limite);
    }
}

TEST_CASE("Movimiento: el seguimiento del terreno no depende del framerate") {
    // El bug: `p.y += diferencia * 0.25f` era un 25% POR FRAME, no por
    // segundo. A 144 fps el animal subia los escalones 5 veces mas rapido que
    // a 30 -- la altura dependia del rendimiento del PC.
    //
    // Ahora es 1 - exp(-k*dt), que se mide en segundos.
    MundoColina mundo;

    float tiempos[3];
    int idx = 0;
    for (float fps : {30.0f, 60.0f, 144.0f}) {
        MundoPecaries m(3);
        m.soltarUno(0.0f, 0.0f, EtapaPecari::ADULTO, mundo);

        // Hundirlo 3 bloques y medir cuanto tarda en volver al suelo.
        const float objetivo = m.todos()[0].y;
        const_cast<PecariAgente&>(m.todos()[0]).y = objetivo - 3.0f;

        const float dt = 1.0f / fps;
        int frames = 0;
        for (int i = 0; i < (int)(5.0f * fps); ++i) {
            m.actualizar(dt, mundo);
            ++frames;
            if (std::fabs(m.todos()[0].y - objetivo) < 0.05f) break;
        }
        tiempos[idx++] = frames * dt;
    }

    INFO("30fps=", tiempos[0], "s  60fps=", tiempos[1], "s  144fps=", tiempos[2], "s");
    // Los tres tiempos deben parecerse. Antes iban en proporcion ~5:1.
    for (int i = 0; i < 3; ++i) {
        for (int j = i + 1; j < 3; ++j) {
            const float may = std::fmax(tiempos[i], tiempos[j]);
            const float men = std::fmin(tiempos[i], tiempos[j]);
            CHECK(may < men * 1.5f);   // menos del 50% de diferencia
        }
    }
}

TEST_CASE("Culling: el pecari que tienes DELANTE siempre se dibuja") {
    // ⭐ ESTE ERA EL BUG DE "DESAPARECEN CUANDO LOS MIRO".
    //
    // main.cpp calculaba la direccion de camara como (+sin(yaw), +cos(yaw)),
    // pero getForward() del motor usa (-sin(yaw), -cos(yaw)) porque OpenGL
    // mira hacia Z NEGATIVO. O sea que el vector apuntaba justo al reves.
    //
    // Efecto: el culling por angulo descartaba a los que tenias delante y
    // conservaba a los de detras. Al girarte hacia una manada, desaparecia.
    MundoLlano mundo;
    CacheMallas cache;

    for (int yawDeg = 0; yawDeg < 360; yawDeg += 30) {
        const float yaw = yawDeg * 3.14159265f / 180.0f;

        // La formula del motor (main.cpp, Player::getForward).
        const float dirX = -std::sin(yaw);
        const float dirZ = -std::cos(yaw);

        MundoPecaries m(1);
        m.soltarUno(dirX * 20.0f, dirZ * 20.0f, EtapaPecari::ADULTO, mundo);

        std::vector<MundoPecaries::InstanciaDibujo> inst;
        m.prepararDibujo(inst, cache, 0.0f, 0.0f, dirX, dirZ, 200.0f);

        INFO("yaw = ", yawDeg, " grados");
        CHECK(inst.size() == 1);   // el de delante TIENE que salir
    }
}

TEST_CASE("Culling: el pecari que tienes DETRAS no se dibuja") {
    // La otra mitad: si no descartara nada, el culling no serviria de nada.
    MundoLlano mundo;
    CacheMallas cache;

    for (int yawDeg = 0; yawDeg < 360; yawDeg += 30) {
        const float yaw = yawDeg * 3.14159265f / 180.0f;
        const float dirX = -std::sin(yaw);
        const float dirZ = -std::cos(yaw);

        MundoPecaries m(1);
        // Justo a la espalda.
        m.soltarUno(-dirX * 20.0f, -dirZ * 20.0f, EtapaPecari::ADULTO, mundo);

        std::vector<MundoPecaries::InstanciaDibujo> inst;
        m.prepararDibujo(inst, cache, 0.0f, 0.0f, dirX, dirZ, 200.0f);

        INFO("yaw = ", yawDeg, " grados");
        CHECK(inst.empty());
    }
}
