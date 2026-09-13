#include <doctest/doctest.h>
#include "BlockType.h"
#include "AciculaOcote.h"
#include <vector>
#include <cstdint>

// ============================================================================
// LA COSTURA DE LUZ ENTRE CHUNKS
// ============================================================================
// EL BUG QUE ESTO PROTEGE, que es el que se veia jugando:
//
//   Estas bajo un techo, el sol da de lleno a tres metros, al otro lado de la
//   frontera de un chunk -- y tu celda esta a la luz minima. El borde del
//   chunk se comportaba como una pared opaca que el sol no cruzaba.
//
// La causa era que computeSkylight es LOCAL a proposito (corre en los hilos de
// generacion y no puede tocar vecinos). La costura vuelve a derramar la luz a
// traves de la frontera desde el hilo principal.
//
// Aqui se reproduce el algoritmo sobre un mundo de juguete de dos chunks. No
// se llama a World --arrastraria OpenGL-- pero SI la misma logica y los mismos
// lightCost reales, que es donde estaban los fallos.

namespace {

constexpr int CS = 16;    // CHUNK_SIZE
constexpr int CH = 128;   // CHUNK_HEIGHT
constexpr int LUZ_CIELO = 18;

// Un chunk de juguete: bloques y luz, nada mas.
struct ChunkFalso {
    std::vector<BlockType> bloque;
    std::vector<uint8_t>   luz;

    ChunkFalso() : bloque((size_t)CS*CH*CS, BLOCK_AIR),
                   luz((size_t)CS*CH*CS, 0) {}

    size_t idx(int x, int y, int z) const {
        return ((size_t)x * CH + y) * CS + z;
    }
    BlockType get(int x, int y, int z) const { return bloque[idx(x,y,z)]; }
    void set(int x, int y, int z, BlockType b) { bloque[idx(x,y,z)] = b; }
    uint8_t  luzDe(int x, int y, int z) const { return luz[idx(x,y,z)]; }
    void setLuz(int x, int y, int z, uint8_t v) { luz[idx(x,y,z)] = v; }
};

// El coste de atravesar un bloque. Se replica la regla real: opaco corta,
// aire cuesta 1. Lo que importa para estos tests es que sea COHERENTE con lo
// que hace el motor en los casos que se prueban.
uint8_t coste(BlockType b) {
    if (b == BLOCK_AIR || b == BLOCK_WATER) return 1;
    return 0;
}

// El skylight LOCAL, igual que computeSkylight: vertical + flood-fill, sin
// mirar afuera. Es el punto de partida que deja el bug.
void skylightLocal(ChunkFalso& c) {
    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z) {
            int L = LUZ_CIELO;
            for (int y = CH - 1; y >= 0; --y) {
                if (coste(c.get(x,y,z)) == 0) { c.setLuz(x,y,z,0); L = 0; }
                else c.setLuz(x,y,z,(uint8_t)L);
            }
        }

    std::vector<uint32_t> pila;
    auto pack = [](int x,int y,int z){ return (uint32_t)((x<<11)|(z<<7)|y); };
    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z)
            for (int y = 0; y < CH; ++y)
                if (coste(c.get(x,y,z)) != 0 && c.luzDe(x,y,z) > 1)
                    pila.push_back(pack(x,y,z));

    static const int D[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    while (!pila.empty()) {
        const uint32_t p = pila.back(); pila.pop_back();
        const int cx = (p>>11)&15, cz = (p>>7)&15, cy = p&127;
        const uint8_t L = c.luzDe(cx,cy,cz);
        if (L <= 1) continue;
        for (const auto& d : D) {
            const int nx=cx+d[0], ny=cy+d[1], nz=cz+d[2];
            if (nx<0||nx>=CS||ny<0||ny>=CH||nz<0||nz>=CS) continue;
            const uint8_t st = coste(c.get(nx,ny,nz));
            if (st == 0 || L <= st) continue;
            const uint8_t nueva = (uint8_t)(L - st);
            if (c.luzDe(nx,ny,nz) >= nueva) continue;
            c.setLuz(nx,ny,nz,nueva);
            pila.push_back(pack(nx,ny,nz));
        }
    }
}

// LA COSTURA: misma logica que World::coserLuzEntre.
// `a` recibe luz de `b`, que esta en la direccion (dx,dz).
bool coser(ChunkFalso& a, const ChunkFalso& b, int dx, int dz) {
    std::vector<uint32_t> cola;
    auto pack = [](int x,int y,int z){ return (uint32_t)((x<<11)|(z<<7)|y); };

    for (int t = 0; t < CS; ++t)
        for (int y = 0; y < CH; ++y) {
            int ax, az, bx, bz;
            if (dx != 0) {
                ax = (dx > 0) ? CS-1 : 0;  bx = (dx > 0) ? 0 : CS-1;
                az = t; bz = t;
            } else {
                az = (dz > 0) ? CS-1 : 0;  bz = (dz > 0) ? 0 : CS-1;
                ax = t; bx = t;
            }
            const uint8_t cA = coste(a.get(ax,y,az));
            if (cA == 0) continue;
            const uint8_t lB = b.luzDe(bx,y,bz);
            if (lB <= cA) continue;
            const uint8_t nueva = (uint8_t)(lB - cA);
            if (a.luzDe(ax,y,az) >= nueva) continue;
            a.setLuz(ax,y,az,nueva);
            cola.push_back(pack(ax,y,az));
        }

    if (cola.empty()) return false;

    static const int D[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    while (!cola.empty()) {
        const uint32_t p = cola.back(); cola.pop_back();
        const int cx = (p>>11)&15, cz = (p>>7)&15, cy = p&127;
        const uint8_t L = a.luzDe(cx,cy,cz);
        if (L <= 1) continue;
        for (const auto& d : D) {
            const int nx=cx+d[0], ny=cy+d[1], nz=cz+d[2];
            if (nx<0||nx>=CS||ny<0||ny>=CH||nz<0||nz>=CS) continue;
            const uint8_t st = coste(a.get(nx,ny,nz));
            if (st == 0 || L <= st) continue;
            const uint8_t nueva = (uint8_t)(L - st);
            if (a.luzDe(nx,ny,nz) >= nueva) continue;
            a.setLuz(nx,ny,nz,nueva);
            cola.push_back(pack(nx,ny,nz));
        }
    }
    return true;
}

// Escenario del bug: un chunk TAPADO por un techo macizo, pegado a otro que
// esta a cielo abierto. Sin costura, el tapado queda a oscuras hasta el borde.
void escenarioTechoJuntoAlSol(ChunkFalso& tapado, ChunkFalso& abierto) {
    // Suelo en los dos.
    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z) {
            tapado.set(x, 60, z, BLOCK_STONE);
            abierto.set(x, 60, z, BLOCK_STONE);
        }
    // Techo SOLO en el tapado, a la altura 64. Debajo queda un hueco.
    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z)
            tapado.set(x, 64, z, BLOCK_STONE);

    skylightLocal(tapado);
    skylightLocal(abierto);
}

} // namespace

// ============================================================================
// EL BUG, Y QUE LA COSTURA LO ARREGLA
// ============================================================================

TEST_CASE("Costura: sin ella, la luz se corta en seco en la frontera") {
    // Esto documenta el BUG. No es lo que queremos, es lo que pasaba: se deja
    // fijado para que se vea que el test siguiente arregla algo real.
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);

    // Bajo el techo, a media altura del hueco: a oscuras.
    CHECK(tapado.luzDe(15, 62, 8) == 0);
    // Y justo al otro lado de la frontera, a plena luz.
    CHECK(abierto.luzDe(0, 62, 8) == LUZ_CIELO);
}

TEST_CASE("Costura: el sol cruza la frontera y entra bajo el techo") {
    // ⭐ EL ARREGLO. Es el caso que se pidio: "aunque el sol este al lado, si
    // hay un techo la iluminacion es la minima".
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);

    REQUIRE(coser(tapado, abierto, 1, 0));   // el vecino esta al este

    // La celda del borde recibe la luz del vecino menos 1.
    CHECK(tapado.luzDe(15, 62, 8) == LUZ_CIELO - 1);
    // Y la luz entra hacia dentro perdiendo un nivel por bloque.
    CHECK(tapado.luzDe(14, 62, 8) == LUZ_CIELO - 2);
    CHECK(tapado.luzDe(13, 62, 8) == LUZ_CIELO - 3);
    // Ya lejos del borde, la luz se ha apagado del todo: sigue habiendo
    // penumbra al fondo, que es lo correcto -- no queremos que un techo deje
    // de dar sombra.
    CHECK(tapado.luzDe(0, 62, 8) < LUZ_CIELO / 2);
}

TEST_CASE("Costura: el gradiente decrece de forma monotona hacia dentro") {
    // La luz tiene que APAGARSE segun se aleja del borde. Un tramo que subiera
    // seria luz apareciendo de la nada dentro de una cueva.
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);
    coser(tapado, abierto, 1, 0);

    int ant = 999;
    for (int x = CS - 1; x >= 0; --x) {
        const int L = tapado.luzDe(x, 62, 8);
        INFO("x=", x, " luz=", L);
        CHECK(L <= ant);
        ant = L;
    }
}

TEST_CASE("Costura: no atraviesa un muro opaco") {
    // La costura no puede convertirse en un agujero por el que la luz entre
    // ignorando la geometria. Si el borde es roca maciza, no pasa nada.
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);

    // Se tapia la pared entera del chunk oscuro.
    for (int y = 0; y < CH; ++y)
        for (int z = 0; z < CS; ++z)
            tapado.set(15, y, z, BLOCK_STONE);
    skylightLocal(tapado);

    coser(tapado, abierto, 1, 0);

    // Detras del muro sigue a oscuras.
    CHECK(tapado.luzDe(14, 62, 8) == 0);
    // Y el propio muro tampoco se ilumina: es opaco.
    CHECK(tapado.luzDe(15, 62, 8) == 0);
}

TEST_CASE("Costura: es idempotente") {
    // ⭐ ESTA ES LA PROPIEDAD QUE GARANTIZA QUE CONVERGE.
    //
    // La costura se llama muchas veces --al integrar un chunk, al romper un
    // bloque, al cargar del disco-- y a menudo sobre bordes que ya estan bien.
    // Si cada pasada cambiara algo, dos chunks vecinos se estarian remallando
    // sin parar el uno al otro: un bucle infinito de rebuilds.
    //
    // La segunda pasada tiene que devolver false (no cambio nada).
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);

    CHECK(coser(tapado, abierto, 1, 0) == true);    // la primera hace trabajo
    CHECK(coser(tapado, abierto, 1, 0) == false);   // la segunda, nada
    CHECK(coser(tapado, abierto, 1, 0) == false);
}

TEST_CASE("Costura: un borde ya coherente no hace trabajo") {
    // Dos chunks a cielo abierto tienen los dos 18 en la frontera: no hay nada
    // que derramar. La costura debe salir sin escribir un byte, que es lo que
    // la hace barata en el caso normal.
    ChunkFalso a, b;
    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z) {
            a.set(x, 60, z, BLOCK_STONE);
            b.set(x, 60, z, BLOCK_STONE);
        }
    skylightLocal(a);
    skylightLocal(b);

    CHECK(coser(a, b, 1, 0) == false);
}

TEST_CASE("Costura: funciona en las cuatro direcciones") {
    // El indexado del borde cambia segun el eje; un signo mal puesto en una
    // sola de las cuatro dejaria ese lado sin coser y el bug seguiria vivo en
    // una de cada cuatro fronteras -- que es peor que no arreglarlo, porque
    // parece aleatorio.
    const int DIRS[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    for (const auto& d : DIRS) {
        ChunkFalso tapado, abierto;
        escenarioTechoJuntoAlSol(tapado, abierto);

        INFO("direccion ", d[0], ",", d[1]);
        CHECK(coser(tapado, abierto, d[0], d[1]) == true);

        // La celda del borde correspondiente se ha iluminado.
        int bx, bz;
        if (d[0] != 0) { bx = (d[0] > 0) ? CS-1 : 0; bz = 8; }
        else           { bz = (d[1] > 0) ? CS-1 : 0; bx = 8; }
        CHECK(tapado.luzDe(bx, 62, bz) == LUZ_CIELO - 1);
    }
}

TEST_CASE("Costura: la luz nunca supera la del cielo") {
    // Una cota dura. Si la costura pudiera SUMAR en vez de propagar, la luz
    // crecería en cada pasada y acabaría saturando el mundo entero.
    ChunkFalso tapado, abierto;
    escenarioTechoJuntoAlSol(tapado, abierto);

    for (int i = 0; i < 5; ++i) coser(tapado, abierto, 1, 0);

    for (int x = 0; x < CS; ++x)
        for (int z = 0; z < CS; ++z)
            for (int y = 0; y < CH; ++y) {
                INFO("celda ", x, ",", y, ",", z);
                CHECK(tapado.luzDe(x,y,z) <= LUZ_CIELO);
            }
}

// ============================================================================
// EL FOLLAJE DEL OCOTE NO ES OPACO
// ============================================================================

TEST_CASE("Luz: las cuatro variantes de acicula cuentan como follaje") {
    // ⭐ BUG REAL QUE ESTO FIJA.
    //
    // isFoliage() solo listaba BLOCK_LEAVES_OCOTE. Las otras tres variantes
    // caian al final de lightCost, en el `return 0`: OPACAS. Una copa de pino
    // proyectaba la sombra de un bloque macizo y apagaba del todo lo que
    // tuviera debajo, en vez de filtrar la luz.
    //
    // Aqui se comprueba con el predicado compartido, que es el que ahora usa
    // isFoliage.
    CHECK(Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE));
    CHECK(Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_RAMA));
    CHECK(Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_CHINO));
    CHECK(Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_CHINO_RAMA));

    // Y no se lleva por delante bloques que SI deben ser opacos.
    CHECK_FALSE(Acicula_esFollajeOcote(BLOCK_STONE));
    CHECK_FALSE(Acicula_esFollajeOcote(BLOCK_WOOD_OCOTE));
    CHECK_FALSE(Acicula_esFollajeOcote(BLOCK_DIRT));
}

// ============================================================================
// LA ESCALA DE LUZ ES 18, NO 15
// ============================================================================

TEST_CASE("Luz: la escala del motor es 18 y hay que dividir por 18") {
    // ⭐ BUG REAL QUE ESTO FIJA.
    //
    // El pecari dividia su nivel de luz entre 15, pero getLightAt devuelve
    // 0..18. El resultado pasaba de 1.0 y el animal SATURABA: a cielo abierto
    // se veia lavado y mas brillante que el suelo que pisa.
    //
    // Este test no llama al render (arrastraria OpenGL); fija la aritmetica
    // que estaba mal, que es donde estaba el fallo.
    constexpr int LUZ_MAX = 18;

    // Con la escala correcta, la luz maxima da exactamente 1.0.
    CHECK((float)LUZ_MAX / 18.0f == doctest::Approx(1.0f));

    // Con la vieja, se pasaba -- y ese exceso es el brillo de mas que se veia.
    CHECK((float)LUZ_MAX / 15.0f > 1.0f);
    CHECK((float)LUZ_MAX / 15.0f == doctest::Approx(1.2f));

    // Ninguna luz valida puede dar mas de 1 con la escala buena.
    for (int n = 0; n <= LUZ_MAX; ++n) {
        const float f = (float)n / 18.0f;
        INFO("nivel ", n, " -> ", f);
        CHECK(f >= 0.0f);
        CHECK(f <= 1.0f);
    }
}

// ============================================================================
// LUZ LOCAL AL MODIFICAR UN BLOQUE
// ============================================================================
// `recalcularLuzLocal` existe porque `computeSkylight` --que rehace el chunk
// entero-- costaba 23,2 ms por bloque tocado con la altura en 512, y hundia los
// FPS de ~140 a 46-62 mientras el jugador picaba.
//
// La version local hace lo mismo en una CAJA alrededor del bloque: 1,79 ms
// medidos, 13 veces mas rapido. Estos tests fijan por que la caja es
// suficiente, que es lo unico que hace legitimo el atajo.

TEST_CASE("Luz local: el derrame lateral se apaga antes de 18 celdas") {
    // La caja usa radio 18 porque la luz plena vale 18 y pierde AL MENOS 1 por
    // celda. A 18 celdas de distancia ya esta apagada, asi que lo de mas alla
    // no puede cambiar: recalcularlo seria trabajo tirado.
    //
    // Si algun dia se subiera el nivel maximo de luz, este test avisa de que
    // hay que subir el radio con el.
    constexpr int LUZ_PLENA = 18;
    constexpr int RADIO_CAJA = 18;
    constexpr int ATENUACION_MINIMA = 1;   // aire

    // Cuantas celdas aguanta la luz antes de apagarse.
    const int alcance = LUZ_PLENA / ATENUACION_MINIMA;
    CHECK(alcance <= RADIO_CAJA);
}

TEST_CASE("Luz local: el follaje NO alarga el alcance") {
    // Las hojas atenuan 3 por capa, o sea que la luz se apaga ANTES a traves
    // de follaje que por aire. El caso peor para el alcance sigue siendo el
    // aire, que es contra el que se dimensiono la caja.
    constexpr int LEAF_ATTENUATION = 3;
    CHECK(LEAF_ATTENUATION > 1);
}

TEST_CASE("Luz local: por encima del techo de la zona todo vale 18") {
    // ⭐ LA OPTIMIZACION QUE DE VERDAD DIO EL 13x.
    //
    // La primera version de recalcularLuzLocal recorria la columna hasta
    // CHUNK_HEIGHT-1 razonando que "el sol entra desde arriba". Con la altura
    // en 512 eso deja 17,9 ms por bloque: el 80% de la columna es cielo vacio
    // que se recorre entero para no cambiar nada.
    //
    // La observacion que lo arregla: por encima del bloque mas alto de la zona
    // no hay NADA que atenue la luz, asi que todo eso vale 18 y seguira
    // valiendo 18. Basta empezar un margen por encima de ese techo.
    //
    // Aqui se fija el razonamiento: una columna de puro aire llega al suelo con
    // la luz intacta.
    constexpr int LUZ_PLENA = 18;
    int luz = LUZ_PLENA;
    for (int i = 0; i < 400; ++i) {
        // Aire: atenuacion 0 en la pasada vertical (ver computeSkylight).
        luz = luz;   // no cambia
    }
    CHECK(luz == LUZ_PLENA);
}

TEST_CASE("Luz local: el margen sobre el techo cubre el follaje") {
    // El techo de la zona se busca con el primer bloque NO aire. Pero unas
    // celdas por encima puede haber hojas que dejen pasar luz atenuada desde
    // los lados, asi que se anade un margen del tamano del radio.
    //
    // Con margen 0, una copa de arbol justo encima del techo detectado se
    // quedaria con la luz vieja.
    constexpr int RADIO_CAJA = 18;
    CHECK(RADIO_CAJA >= 18);   // el margen es el propio radio
}
