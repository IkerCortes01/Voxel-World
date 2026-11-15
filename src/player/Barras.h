#pragma once

#include <algorithm>
#include <cmath>

// ============================================================================
// ⭐ LAS TRES BARRAS DEL JUGADOR: VIDA, RESISTENCIA Y ENERGIA
// ============================================================================
// PEDIDO: "elimina el corazon y [...] pon 3 barras del lado izquierdo hasta
// arriba, una es de vida con 100 puntos, otra es de resistencia que es de 100,
// y la otra es de energia que es de 100".
//
//   VIDA (100)         "se gasta cuando el personaje recibe dano, como caer de
//                      gran altura o que un animal le haga dano etc, tu inventa
//                      las demas formas de perder vida":
//                        CAIDA   mas de 3,5 bloques: 9 puntos por bloque de mas
//                                (14,6 bloques la quitan entera).
//                        ANIMAL  un animal que se defiende y lo alcanza.
//                        AHOGO   20 s de aire bajo el agua; luego 6 por segundo.
//                        LAVA    dentro de ella, 25 por segundo.
//                        FUEGO   pegado a lava o a un horno encendido.
//                        FRIO / CALOR  en "demasiado frio" o "demasiado calor".
//                      Vuelve sola, despacio, si lleva 8 s sin recibir dano y
//                      le queda energia; durmiendo, mas deprisa. A cero, se
//                      desmaya (el motor lo devuelve a su punto de partida).
//
//   RESISTENCIA (100)  "se gana si hace ejercicio el personaje, por ahora no
//                      funcionara, luego si". Existe y se dibuja; nada la mueve.
//
//   ENERGIA (100)      "se gasta al correr, saltar, etc y se recarga sola al
//                      estar parado, dormido y caminando con sigilo":
//                        gasta   correr 7/s, nadar 3/s, golpear o picar 1,2/s,
//                                cada salto 5.
//                        recarga parado 9/s, dormido 16/s, con sigilo 5/s.
//                        Andar normal ni gasta ni recarga.
//                      A cero queda AGOTADO: no puede correr hasta recuperar 20.
//
// En creativo nada baja. Cabecera sola, sin motor: tests/test_barras.cpp.
// ============================================================================

namespace BarrasJugador {

constexpr float MAXIMO = 100.0f;

constexpr float CAIDA_LIBRE = 3.5f;            // bloques que no duelen
constexpr float DANO_POR_BLOQUE = 9.0f;
constexpr float AIRE_SEG = 20.0f;
constexpr float DANO_AHOGO = 6.0f;             // por segundo, sin aire
constexpr float DANO_LAVA = 25.0f;
constexpr float DANO_FUEGO = 4.0f;             // por segundo, pegado a el
constexpr float FUEGO_CERCA = 0.6f;            // desde que cercania quema (0..1)
constexpr float DANO_TEMPERATURA = 1.5f;       // por segundo, en los extremos
constexpr float TEMPERATURA_EXTREMA = 4.5f;
constexpr float SIN_DANO_PARA_CURAR = 8.0f;
constexpr float CURA = 0.8f, CURA_DORMIDO = 2.5f;
constexpr float ENERGIA_PARA_CURAR = 30.0f;

constexpr float GASTO_CORRER = 7.0f, GASTO_NADAR = 3.0f, GASTO_GOLPEAR = 1.2f, GASTO_SALTO = 5.0f;
constexpr float RECARGA_PARADO = 9.0f, RECARGA_DORMIDO = 16.0f, RECARGA_SIGILO = 5.0f;
constexpr float ENERGIA_PARA_CORRER = 20.0f;   // lo que hay que recuperar tras agotarse

enum class Causa { NINGUNA, CAIDA, ANIMAL, AHOGO, LAVA, FUEGO, FRIO, CALOR };

struct Entradas {
    float rapidez = 0.0f;          // bloques/s en horizontal
    bool corriendo = false;
    bool sigilo = false;
    bool durmiendo = false;
    bool enSuelo = true;
    bool nadando = false;          // el cuerpo en el agua
    bool bajoAgua = false;         // la cabeza bajo el agua
    bool enLava = false;
    bool golpeando = false;        // picando, talando o pegando
    bool volando = false;
    float fuego = 0.0f;            // 0..1: lo cerca que hay lava o un horno encendido
    float temperatura = 0.0f;      // -5 (demasiado frio) .. +5 (demasiado calor)
    bool creativo = false;
};

struct Estado {
    float vida = MAXIMO, resistencia = MAXIMO, energia = MAXIMO;
    float aire = AIRE_SEG;
    float sinDano = 99.0f;         // segundos desde el ultimo dano
    float golpe = 0.0f;            // 1 al recibir dano, se apaga en ~0,5 s (para el HUD)
    bool agotado = false;
    Causa ultimaCausa = Causa::NINGUNA;

    void reponer() { *this = Estado(); }
    bool sinVida() const { return vida <= 0.0f; }
    bool puedeCorrer() const { return !agotado; }

    void danar(float puntos, Causa causa) {
        if (!(puntos > 0.0f) || vida <= 0.0f) return;
        vida = (std::max)(0.0f, vida - puntos);
        sinDano = 0.0f;
        golpe = 1.0f;
        ultimaCausa = causa;
    }
    // Lo que quita caer `bloques` de alto.
    static float danoDeCaida(float bloques) {
        return bloques > CAIDA_LIBRE ? (bloques - CAIDA_LIBRE) * DANO_POR_BLOQUE : 0.0f;
    }
    void aterrizar(float bloques) { danar(danoDeCaida(bloques), Causa::CAIDA); }
    void saltar() { gastar(GASTO_SALTO); }
    void gastar(float puntos) {
        energia = (std::max)(0.0f, energia - puntos);
        if (energia <= 0.0f) agotado = true;
    }

    void avanzar(float dt, const Entradas& e) {
        if (!(dt > 0.0f)) return;
        if (dt > 0.25f) dt = 0.25f;
        golpe = (std::max)(0.0f, golpe - dt * 2.0f);
        if (e.creativo) {
            vida = energia = MAXIMO;
            aire = AIRE_SEG;
            agotado = false;
            sinDano = 99.0f;
            return;
        }
        sinDano += dt;

        // --- La vida ---
        if (e.bajoAgua) {
            aire = (std::max)(0.0f, aire - dt);
            if (aire <= 0.0f) danar(DANO_AHOGO * dt, Causa::AHOGO);
        } else {
            aire = (std::min)(AIRE_SEG, aire + dt * 5.0f);
        }
        if (e.enLava) danar(DANO_LAVA * dt, Causa::LAVA);
        else if (e.fuego > FUEGO_CERCA) danar(DANO_FUEGO * e.fuego * dt, Causa::FUEGO);
        if (e.temperatura <= -TEMPERATURA_EXTREMA) danar(DANO_TEMPERATURA * dt, Causa::FRIO);
        if (e.temperatura >= TEMPERATURA_EXTREMA) danar(DANO_TEMPERATURA * dt, Causa::CALOR);
        if (vida > 0.0f && sinDano >= SIN_DANO_PARA_CURAR) {
            if (e.durmiendo) vida += CURA_DORMIDO * dt;
            else if (energia >= ENERGIA_PARA_CURAR) vida += CURA * dt;
            vida = (std::min)(MAXIMO, vida);
        }

        // --- La energia ---
        const bool quieto = e.rapidez < 0.3f;
        float cambio = 0.0f;
        if (e.durmiendo) cambio = RECARGA_DORMIDO;
        else if (e.volando) cambio = 0.0f;
        else if (e.nadando) cambio = quieto ? 0.0f : -GASTO_NADAR;
        else if (e.corriendo && !quieto) cambio = -GASTO_CORRER;
        else if (quieto && e.enSuelo) cambio = RECARGA_PARADO;
        else if (e.sigilo && e.enSuelo) cambio = RECARGA_SIGILO;
        if (e.golpeando && !e.durmiendo) cambio = (std::min)(cambio, 0.0f) - GASTO_GOLPEAR;
        if (cambio < 0.0f) gastar(-cambio * dt);
        else energia = (std::min)(MAXIMO, energia + cambio * dt);
        if (agotado && energia >= ENERGIA_PARA_CORRER) agotado = false;
    }
};

inline const char* nombre(Causa c) {
    switch (c) {
        case Causa::CAIDA:  return "CAIDA";
        case Causa::ANIMAL: return "ANIMAL";
        case Causa::AHOGO:  return "AHOGO";
        case Causa::LAVA:   return "LAVA";
        case Causa::FUEGO:  return "FUEGO";
        case Causa::FRIO:   return "FRIO";
        case Causa::CALOR:  return "CALOR";
        default:            return "";
    }
}

}  // namespace BarrasJugador
