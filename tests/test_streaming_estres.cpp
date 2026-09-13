#include <doctest/doctest.h>
#include "render/EstadoChunk.h"
#include "render/PrioridadChunk.h"
#include "render/VigilanteChunk.h"

#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

// ============================================================================
// PRUEBAS EXTREMAS DEL STREAMING (§63, §64)
// ============================================================================
// Los tests de test_streaming.cpp fijan la logica pieza a pieza. Estos simulan
// el PIPELINE COMPLETO a lo largo del tiempo, con un jugador que se mueve, y
// comprueban la propiedad que de verdad importa:
//
//     NINGUN CHUNK SE QUEDA ATASCADO, PASE LO QUE PASE.
//
// Se simula sin OpenGL y sin World: un mapa de chunks falsos que avanzan de
// estado a un ritmo controlado, mas el mismo vigilante que corre en el juego.
// Asi se pueden reproducir en milisegundos escenarios que en el juego real
// tardarian minutos y no serian deterministas -- como "un worker se muere y
// nunca devuelve el chunk".
// ============================================================================

using namespace Streaming;

namespace {

// Un chunk de mentira: solo lo que el streaming necesita saber.
struct ChunkFalso {
    Estado estado = Estado::DESCARGADO;
    double tiempoEstado = 0.0;
    int    rescates = 0;
    bool   trabajadorPerdido = false;   // simula un worker que nunca contesta

    void cambiar(Estado e, double ahora) {
        if (estado == e) return;
        // Toda transicion del simulador tiene que ser legal: si el propio
        // simulador hiciera trampas, el test no probaria nada.
        REQUIRE(transicionValida(estado, e));
        estado = e;
        tiempoEstado = ahora;
    }
};

// El mundo simulado.
struct MundoFalso {
    std::map<std::pair<int,int>, ChunkFalso> chunks;
    double ahora = 0.0;
    int rescatesTotales = 0;
    int fallidosTotales = 0;

    ChunkFalso& en(int x, int z) { return chunks[{x, z}]; }

    // Un paso de simulacion: el pipeline avanza y el vigilante vigila.
    void paso(float dt, const ContextoJugador& j, int chunksPorPaso = 4) {
        ahora += dt;

        // 1. Pedir lo que falte, por orden de prioridad (como updateChunks).
        std::vector<EntradaPrioridad> candidatos;
        const int r = radioBarrido(j);
        for (int dx = -r; dx <= r; ++dx)
            for (int dz = -r; dz <= r; ++dz) {
                if (dx*dx + dz*dz > r*r) continue;
                const int cx = j.chunkX + dx, cz = j.chunkZ + dz;
                auto it = chunks.find({cx, cz});
                if (it != chunks.end() && it->second.estado != Estado::DESCARGADO)
                    continue;
                candidatos.push_back(calcularPrioridad(cx, cz, j));
            }
        std::sort(candidatos.begin(), candidatos.end());

        int pedidos = 0;
        for (const auto& c : candidatos) {
            if (pedidos >= chunksPorPaso) break;
            en(c.cx, c.cz).cambiar(Estado::SOLICITADO, ahora);
            pedidos++;
        }

        // 2. Avanzar el pipeline (los que no tengan el trabajador perdido).
        int trabajo = chunksPorPaso;
        for (auto& par : chunks) {
            if (trabajo <= 0) break;
            ChunkFalso& c = par.second;
            if (c.trabajadorPerdido) continue;   // este no avanza nunca

            switch (c.estado) {
                case Estado::SOLICITADO:    c.cambiar(Estado::GENERANDO, ahora);   trabajo--; break;
                case Estado::GENERANDO:     c.cambiar(Estado::GENERADO, ahora);    trabajo--; break;
                case Estado::GENERADO:      c.cambiar(Estado::MALLA_EN_COLA, ahora); trabajo--; break;
                case Estado::MALLA_EN_COLA: c.cambiar(Estado::MALLANDO, ahora);    trabajo--; break;
                case Estado::MALLANDO:      c.cambiar(Estado::MALLA_LISTA, ahora); trabajo--; break;
                case Estado::MALLA_LISTA:   c.cambiar(Estado::SUBIENDO, ahora);    trabajo--; break;
                case Estado::SUBIENDO:      c.cambiar(Estado::LISTO, ahora);       trabajo--; break;
                default: break;
            }
        }

        // 3. El vigilante, igual que en el juego.
        for (auto& par : chunks) {
            ChunkFalso& c = par.second;
            const float enEstado = (float)(ahora - c.tiempoEstado);
            const Accion a = decidir(c.estado, enEstado, c.rescates);
            if (a == Accion::NINGUNA) continue;

            switch (a) {
                case Accion::REENCOLAR:
                    c.rescates++;
                    c.trabajadorPerdido = false;   // el rescate reengancha
                    c.cambiar(Estado::DESCARGADO, ahora);
                    rescatesTotales++;
                    break;
                case Accion::FORZAR_MALLADO:
                    c.rescates++;
                    c.trabajadorPerdido = false;
                    c.cambiar(Estado::MALLA_EN_COLA, ahora);
                    rescatesTotales++;
                    break;
                case Accion::MARCAR_FALLIDO:
                    c.cambiar(Estado::FALLIDO, ahora);
                    fallidosTotales++;
                    break;
                default: break;
            }
        }
    }

    int contar(Estado e) const {
        int n = 0;
        for (const auto& p : chunks) if (p.second.estado == e) n++;
        return n;
    }

    // ⭐ LA PROPIEDAD CENTRAL: nadie lleva demasiado tiempo sin avanzar.
    int atascados(float umbralSegundos) const {
        int n = 0;
        for (const auto& p : chunks) {
            if (!esTransitorio(p.second.estado)) continue;
            if ((float)(ahora - p.second.tiempoEstado) > umbralSegundos) n++;
        }
        return n;
    }
};

} // namespace

// ----------------------------------------------------------------------------
TEST_CASE("Estres: el jugador quieto acaba con todo LISTO") {
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 4;

    for (int i = 0; i < 400; ++i) m.paso(0.016f, j);

    CHECK(m.contar(Estado::LISTO) > 0);
    CHECK(m.atascados(20.0f) == 0);
    CHECK(m.fallidosTotales == 0);
}

TEST_CASE("Estres: corriendo en linea recta no deja atascados") {
    // El caso "el jugador llega antes que los chunks".
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 4;
    j.velX = 18.0f;                  // corriendo fuerte
    j.miradaX = 1.0f; j.miradaZ = 0.0f;

    float posX = 0.0f;
    for (int i = 0; i < 600; ++i) {
        posX += j.velX * 0.016f;
        j.chunkX = (int)std::floor(posX / 16.0f);
        m.paso(0.016f, j);
    }

    CHECK(m.atascados(20.0f) == 0);
    // Y el suelo bajo los pies tiene que estar listo, que es el objetivo real.
    CHECK(esDibujable(m.en(j.chunkX, j.chunkZ).estado));
}

TEST_CASE("Estres: cambiar de direccion constantemente") {
    // El caso "el jugador gira la camara / cambia de rumbo sin parar".
    // Es el que mas castiga a una cola FIFO sin cancelacion.
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 3;

    float posX = 0.0f, posZ = 0.0f;
    for (int i = 0; i < 800; ++i) {
        const float ang = (float)i * 0.05f;
        j.velX = std::cos(ang) * 15.0f;
        j.velZ = std::sin(ang) * 15.0f;
        j.miradaX = std::cos(ang);
        j.miradaZ = std::sin(ang);
        posX += j.velX * 0.016f;
        posZ += j.velZ * 0.016f;
        j.chunkX = (int)std::floor(posX / 16.0f);
        j.chunkZ = (int)std::floor(posZ / 16.0f);
        m.paso(0.016f, j);
    }

    CHECK(m.atascados(20.0f) == 0);
}

TEST_CASE("Estres: teletransporte repetido") {
    // §55. Con el sistema anterior, cada salto dejaba 24 chunks encolados del
    // sitio viejo que se generaban enteros antes de pedir uno solo del nuevo.
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 3;

    for (int salto = 0; salto < 6; ++salto) {
        j.chunkX = salto * 500;       // lejisimos, sin solape
        j.chunkZ = salto * -300;
        j.velX = j.velZ = 0.0f;
        for (int i = 0; i < 150; ++i) m.paso(0.016f, j);
    }

    CHECK(m.atascados(20.0f) == 0);
    // Tras el ultimo salto, el chunk del jugador tiene que estar listo.
    CHECK(esDibujable(m.en(j.chunkX, j.chunkZ).estado));
}

TEST_CASE("Estres: coordenadas negativas") {
    // El floor de enteros negativos es una fuente clasica de bugs: -1/16 == 0
    // con truncamiento, que manda al chunk equivocado.
    MundoFalso m;
    ContextoJugador j;
    j.chunkX = -1234;
    j.chunkZ = -5678;
    j.distanciaRender = 3;

    for (int i = 0; i < 300; ++i) m.paso(0.016f, j);

    CHECK(m.atascados(20.0f) == 0);
    CHECK(esDibujable(m.en(j.chunkX, j.chunkZ).estado));
    // Y las diagonales negativas siguen siendo emergencia.
    CHECK(anilloDe(j.chunkX - 1, j.chunkZ - 1, j) == Anillo::R0_EMERGENCIA);
}

TEST_CASE("Estres: un worker que nunca contesta se rescata") {
    // ⭐ LA REGLA ABSOLUTA (§65) contra el peor caso real.
    //
    // Se simula lo que en el juego seria un trabajo perdido: un chunk cuyo
    // worker murio, o cuyo resultado se descarto sin soltar el estado. Sin
    // vigilante esto es un chunk invisible para siempre.
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 2;

    for (int i = 0; i < 60; ++i) m.paso(0.016f, j);

    // Se rompen a mano: dejan de avanzar.
    int rotos = 0;
    for (auto& p : m.chunks) {
        if (esTransitorio(p.second.estado)) { p.second.trabajadorPerdido = true; rotos++; }
        if (rotos >= 5) break;
    }
    // Y uno mas, forzado a un estado transitorio concreto.
    m.en(j.chunkX, j.chunkZ).cambiar(Estado::DESCARGADO, m.ahora);
    m.en(j.chunkX, j.chunkZ).cambiar(Estado::SOLICITADO, m.ahora);
    m.en(j.chunkX, j.chunkZ).trabajadorPerdido = true;

    // Tiempo de sobra para que el vigilante actue.
    for (int i = 0; i < 3000; ++i) m.paso(0.016f, j);

    CHECK(m.rescatesTotales > 0);       // el vigilante intervino
    CHECK(m.atascados(30.0f) == 0);     // y nadie quedo colgado
}

TEST_CASE("Estres: 1000 chunks no bloquean el sistema") {
    // §64. El sistema tiene que REPARTIR el trabajo, no intentarlo todo.
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 16;   // radio enorme: ~800 chunks

    // Con un presupuesto pequeño por paso, esto es exactamente el escenario
    // "muchas mas peticiones que capacidad".
    for (int i = 0; i < 1500; ++i) m.paso(0.016f, j, 4);

    CHECK(m.chunks.size() > 500);
    CHECK(m.atascados(30.0f) == 0);
    CHECK(m.fallidosTotales == 0);
    // El nucleo tiene que estar resuelto aunque la periferia siga en camino:
    // es la prueba de que la prioridad manda sobre el orden de llegada.
    CHECK(esDibujable(m.en(j.chunkX, j.chunkZ).estado));
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz) {
            INFO("vecino R0 ", dx, ",", dz);
            CHECK(esDibujable(m.en(j.chunkX + dx, j.chunkZ + dz).estado));
        }
}

TEST_CASE("Estres: velocidad maxima sostenida") {
    // Volar a tope, que es donde el jugador adelanta al streaming.
    MundoFalso m;
    ContextoJugador j;
    j.distanciaRender = 4;
    j.velX = 60.0f;                   // muy por encima de correr
    j.miradaX = 1.0f; j.miradaZ = 0.0f;

    float posX = 0.0f;
    for (int i = 0; i < 500; ++i) {
        posX += j.velX * 0.016f;
        j.chunkX = (int)std::floor(posX / 16.0f);
        m.paso(0.016f, j, 6);
    }

    CHECK(m.atascados(20.0f) == 0);
    // A esta velocidad el corredor de anticipacion tiene que estar activo.
    CHECK(j.chunksAnticipacion() > 0);
    CHECK(j.movimientoRapido());
}

TEST_CASE("Estres: cruzar fronteras de chunk en diagonal no cicla") {
    // §34. Sin histeresis real, oscilar sobre una esquina produce
    // carga/descarga en bucle. Se comprueba que la banda existe: un chunk
    // dentro del radio de carga NUNCA esta a la vez fuera del de descarga.
    ContextoJugador j;
    j.distanciaRender = 4;

    const int rc = radioCarga(j);
    const int rd = radioDescarga(j);
    CHECK(rd > rc);

    // Todo lo que se carga sigue estando dentro del radio de descarga con
    // margen de sobra: hay que alejarse de verdad para perderlo.
    for (int dx = -rc; dx <= rc; ++dx)
        for (int dz = -rc; dz <= rc; ++dz) {
            if (dx*dx + dz*dz > rc*rc) continue;
            INFO("chunk cargado ", dx, ",", dz);
            CHECK(dx*dx + dz*dz <= rd*rd);
        }
}
