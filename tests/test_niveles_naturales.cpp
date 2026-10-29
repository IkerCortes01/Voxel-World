#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <iterator>
#include <set>

// ============================================================================
// UN NIVEL NATURAL SUELTA UNO. SOLO LO APILADO MULTIPLICA.
// ============================================================================
// EL BUG, Y POR QUE ERA GRAVE.
//
// La regla "un nivel suelta tantos items como octavos tiene" se escribio para
// el APILADO: pones tierra tres veces, queda una capa de nivel 3, la picas y
// recuperas tus 3 tierras. El material se conserva en los dos sentidos.
//
// Pero los niveles NO son solo del jugador. El generador los usa en TODA la
// superficie del mundo: la altura del terreno no se redondea a bloques
// enteros, y la parte decimal se convierte en el nivel de la capa de arriba.
// Una loma que sube 2,4 bloques son dos bloques enteros y una capa de nivel 4.
//
// Con la regla vieja, picar una loma INTACTA de nivel 7 daba SIETE tierras de
// un golpe. Y como casi cada bloque de suelo visible tiene nivel parcial, era
// material infinito a ras de suelo: piedra, tierra, pasto, arena y grava.
//
// LA REGLA CORRECTA: la multiplicacion se aplica SOLO a lo que el jugador
// apilo. Un nivel que puso el mundo suelta uno, como cualquier otro bloque.
//
// COMO SE DISTINGUE. El nivel viaja dentro del ID del bloque y no quedan bits
// libres, asi que no se puede marcar ahi sin romper el formato de guardado.
// Se lleva un registro aparte (`g_nivelesDelJugador`) con las celdas que el
// jugador apilo.
// ============================================================================

namespace {

std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}

// Replica del registro del motor, para probar su semantica sin enlazar
// main.cpp. La clave de ordenacion es la misma.
struct Celda {
    int x, y, z;
    bool operator<(const Celda& o) const {
        if (x != o.x) return x < o.x;
        if (y != o.y) return y < o.y;
        return z < o.z;
    }
};

}  // namespace

TEST_CASE("Niveles: el terreno genera niveles parciales en su superficie") {
    // Es la premisa del bug: si el generador NO usara niveles, no habria
    // nada que arreglar. Esta aqui para que quede constancia de por que la
    // regla vieja era peligrosa.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    // ⚠️ La frase aparece DOS veces: una en el comentario del registro
    // (que la cita) y otra en el generador. Interesa la del GENERADOR, que
    // es la ultima -- de ahi rfind.
    const size_t m = src.rfind("EL NIVEL SALE DE LA ALTURA REAL DEL TERRENO");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "el generador ya no pone niveles por la altura; "
                    "revisar si este arreglo sigue teniendo sentido");

    // Y los pone con conNivel, que es lo que los hace indistinguibles de los
    // apilados mirando solo el ID.
    // Margen amplio: entre el comentario y la llamada hay toda la tabla que
    // reparte los ocho escalones por altura en pixeles.
    const std::string cuerpo = src.substr(m, 3000);
    CHECK(cuerpo.find("conNivel(sup, nivel)") != std::string::npos);
}

TEST_CASE("Niveles: existe el registro de lo que apilo el jugador") {
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    CHECK(src.find("std::set<CeldaApilada> g_nivelesDelJugador;") != std::string::npos);
    CHECK(src.find("inline void marcarNivelDelJugador(") != std::string::npos);
    CHECK(src.find("inline bool consumirNivelDelJugador(") != std::string::npos);
}

TEST_CASE("Niveles: la multiplicacion esta condicionada al registro") {
    // ⭐ EL ARREGLO. Sin la condicion, vuelve el material gratis.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("SOLO SE MULTIPLICA LO QUE EL JUGADOR APILO");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "se quito la regla: picar el suelo intacto volveria a "
                    "dar varios items");

    const std::string cuerpo = src.substr(m, 1800);
    CHECK_MESSAGE(
        cuerpo.find("if (consumirNivelDelJugador(bx, by, bz))") != std::string::npos,
        "la multiplicacion debe depender del registro");

    // Y el valor por defecto es 1: lo natural suelta uno.
    CHECK(cuerpo.find("int octavosDelNivel = 1;") != std::string::npos);
}

TEST_CASE("Niveles: todas las rutas de apilado marcan la celda") {
    // Si una ruta de colocado no marca, el jugador PIERDE material al picar
    // lo que apilo por ahi. Son cinco sitios distintos, y es facil anadir
    // una sexta y olvidarla.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    // Se cuentan las llamadas a marcarNivelDelJugador en el codigo (no en
    // los comentarios, que no llevan parentesis de llamada).
    int llamadas = 0;
    size_t i = 0;
    while ((i = src.find("marcarNivelDelJugador(", i)) != std::string::npos) {
        ++llamadas;
        i += 20;
    }
    // 5 rutas de apilado + la definicion de la funcion.
    CHECK_MESSAGE(llamadas >= 6,
                  "falta alguna ruta de apilado por marcar: el jugador "
                  "perderia material al picar lo que puso");
}

TEST_CASE("Niveles: el registro se limpia al cambiar de mundo") {
    // Son COORDENADAS: las del mundo anterior señalan sitios distintos aqui.
    // Sin limpiarlo, una loma natural en las mismas coordenadas donde el
    // jugador apilo en otra partida soltaria de mas. Mismo fallo que el
    // inventario heredado.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("EL REGISTRO DE CAPAS APILADAS ES DE ESTE MUNDO");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "el registro no se limpia entre mundos");

    const std::string cuerpo = src.substr(m, 900);
    CHECK(cuerpo.find("g_nivelesDelJugador.clear();") != std::string::npos);
}

TEST_CASE("Niveles: el registro tiene tope y no crece sin limite") {
    // La leccion de `lavaLevels` (ver PENDIENTES.md): un mapa por coordenada
    // que nadie purga crece toda la sesion. Aqui hay dos defensas: se borra
    // al consumirlo y hay un tope duro.
    const std::string src = fuenteMotor();
    REQUIRE(!src.empty());

    CHECK(src.find("MAX_NIVELES_JUGADOR") != std::string::npos);
    CHECK(src.find("if (g_nivelesDelJugador.size() >= MAX_NIVELES_JUGADOR) return;")
          != std::string::npos);
    // Y consumir BORRA la entrada.
    CHECK(src.find("g_nivelesDelJugador.erase(it);") != std::string::npos);
}

TEST_CASE("Niveles: consultar y olvidar es idempotente en el lado seguro") {
    // Semantica del registro: la primera consulta dice "si", la segunda "no".
    // Importa porque si dos rutas consultaran la misma celda, la segunda no
    // debe volver a multiplicar.
    std::set<Celda> reg;
    auto marcar = [&](int x, int y, int z) { reg.insert({x, y, z}); };
    auto consumir = [&](int x, int y, int z) {
        auto it = reg.find({x, y, z});
        if (it == reg.end()) return false;
        reg.erase(it);
        return true;
    };

    marcar(10, 64, -3);
    CHECK(consumir(10, 64, -3));        // apilada: multiplica
    CHECK_FALSE(consumir(10, 64, -3));  // ya consumida: no repite
    CHECK(reg.empty());                 // y no deja basura

    // Una celda que nadie apilo nunca da falso: es el suelo natural.
    CHECK_FALSE(consumir(0, 64, 0));
}

TEST_CASE("Niveles: perder el registro degrada al lado seguro") {
    // Si se alcanza el tope y se deja de registrar, lo peor que pasa es que
    // una capa apilada suelte 1 item en vez de N. Nunca al reves: jamas se
    // multiplica algo que no este en el registro, que es lo que crearia
    // material de la nada.
    std::set<Celda> reg;   // vacio, como si se hubiera perdido el registro
    auto consumir = [&](int x, int y, int z) {
        auto it = reg.find({x, y, z});
        if (it == reg.end()) return false;
        reg.erase(it);
        return true;
    };

    // Sin registro, el multiplicador se queda en 1 pase lo que pase.
    const int octavos = consumir(5, 70, 5) ? 7 : 1;
    CHECK(octavos == 1);
}

TEST_CASE("Niveles: los materiales del suelo admiten niveles") {
    // Los que nombro el pedido: piedra, tierra, pasto, arena, grava. Son los
    // que cubren la superficie del mundo y por tanto los que daban material
    // gratis.
    CHECK(admiteNiveles(BLOCK_STONE));
    CHECK(admiteNiveles(BLOCK_DIRT));
    CHECK(admiteNiveles(BLOCK_GRASS));
    CHECK(admiteNiveles(BLOCK_SAND));
    CHECK(admiteNiveles(BLOCK_GRAVEL));

    // Y un nivel suyo sigue siendo su material: lo que cambia es cuanto
    // suelta, no que suelta.
    for (BlockType b : { BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL }) {
        const BlockType n3 = conNivel(b, 3);
        INFO("material ", (int)b);
        CHECK(esNivelParcial(n3));
        CHECK(nivelDe(n3) == 3);
        CHECK(bloqueBaseDe(n3) == b);
    }
}
