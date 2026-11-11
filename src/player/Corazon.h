#pragma once

#include <cmath>

// ============================================================================
// EL CORAZON DEL JUGADOR: SU PULSO TE AVISA
// ============================================================================
// PEDIDO: "un corazon arriba de la casilla 5 de la hotbar [...] su funcion es
// avisarte de algo, tendra animacion de palpitar":
//
//   QUIETO / ANDANDO   normal (~70 / ~78 latidos por minuto)
//   CORRIENDO          sube, y sube mas cuanto mas rato se corre (hasta ~150)
//   DURMIENDO / DESCANSANDO  lento (~52) y suave
//   A GRAN ALTURA      desde 10 bloques de caida: fuerte, pero no tan rapido
//   FRIO               rapido (desde "frio"; mas cuanto mas frio)
//   CALOR              rapido, solo en los extremos ("mucho" y "demasiado")
//   MIEDO              con un animal cerca: mas fuerte y mas rapido cuanto
//                      mas grande y amenazante sea y mas cerca este
//
// Dos numeros salen de aqui: el PULSO (latidos por minuto) y la FUERZA
// (0..1, lo que se hincha el corazon en cada latido). Los dos siguen a su meta
// con inercia, como un corazon de verdad: sube en segundos y baja despacio.
// La forma del latido es la doble de verdad, "lub-dub" (ver latido()).
// ============================================================================

namespace CorazonJugador {

struct Entradas {
    float rapidez = 0.0f;          // bloques/s en horizontal
    bool corriendo = false;
    bool durmiendo = false;
    float quieto = 0.0f;           // segundos sin moverse
    float caida = 0.0f;            // bloques hasta el suelo de alrededor (el borde mas alto)
    float temperatura = 0.0f;      // nivel -5 (demasiado frio) .. +5 (demasiado calor)
    float amenaza = 0.0f;          // 0..2: animales cerca, por tamano y cercania
};

struct Corazon {
    float pulso = 70.0f;           // latidos por minuto
    float fuerza = 0.45f;          // 0..1
    float fase = 0.0f;             // 0..1 dentro del latido
    float corriendoSeg = 0.0f;     // lo que lleva corriendo
    // Para el HUD: la causa que mas pesa ahora mismo.
    enum Causa { CALMA, ESFUERZO, DESCANSO, ALTURA, FRIO, CALOR, MIEDO } causa = CALMA;

    // Las metas para estas entradas.
    void metas(const Entradas& e, float& pulsoMeta, float& fuerzaMeta, Causa& c) const {
        pulsoMeta = 70.0f;
        fuerzaMeta = 0.45f;
        c = CALMA;
        if (e.durmiendo || e.quieto > 25.0f) {
            pulsoMeta = e.durmiendo ? 52.0f : 60.0f;
            fuerzaMeta = 0.30f;
            c = DESCANSO;
        }
        if (e.rapidez > 0.5f && !e.corriendo) { pulsoMeta = 78.0f; fuerzaMeta = 0.5f; c = CALMA; }
        if (e.corriendo && e.rapidez > 2.0f) {
            // Sube con el rato corriendo: 95 al empezar, 150 al minuto.
            const float t = corriendoSeg / 60.0f;
            pulsoMeta = 95.0f + 55.0f * (t > 1.0f ? 1.0f : t);
            fuerzaMeta = 0.65f + 0.20f * (t > 1.0f ? 1.0f : t);
            c = ESFUERZO;
        }
        // La altura: fuerte, poco mas rapido.
        if (e.caida >= 10.0f) {
            const float h = std::fmin(1.0f, (e.caida - 10.0f) / 40.0f);
            pulsoMeta += 8.0f + 12.0f * h;
            fuerzaMeta = std::fmax(fuerzaMeta, 0.80f + 0.20f * h);
            c = ALTURA;
        }
        // El frio acelera desde "frio" (-3); el calor solo en los extremos (+4).
        if (e.temperatura <= -3.0f) {
            pulsoMeta += 12.0f * (-e.temperatura - 2.0f);
            fuerzaMeta = std::fmax(fuerzaMeta, 0.55f);
            c = FRIO;
        } else if (e.temperatura >= 4.0f) {
            pulsoMeta += 18.0f * (e.temperatura - 3.0f);
            fuerzaMeta = std::fmax(fuerzaMeta, 0.55f);
            c = CALOR;
        }
        // El miedo manda sobre todo.
        if (e.amenaza > 0.05f) {
            const float a = std::fmin(2.0f, e.amenaza);
            pulsoMeta += 30.0f * a;
            fuerzaMeta = std::fmax(fuerzaMeta, 0.6f + 0.2f * a);
            c = MIEDO;
        }
        if (pulsoMeta > 185.0f) pulsoMeta = 185.0f;
        if (fuerzaMeta > 1.0f) fuerzaMeta = 1.0f;
    }

    void avanzar(float dt, const Entradas& e) {
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.25f) dt = 0.25f;
        corriendoSeg = e.corriendo && e.rapidez > 2.0f ? corriendoSeg + dt : std::fmax(0.0f, corriendoSeg - 2.0f * dt);
        float pm, fm;
        metas(e, pm, fm, causa);
        // Sube en unos 4 s; baja en unos 12.
        const float tau = pm > pulso ? 4.0f : 12.0f;
        pulso += (pm - pulso) * (1.0f - std::exp(-dt / tau));
        fuerza += (fm - fuerza) * (1.0f - std::exp(-dt / 3.0f));
        fase += dt * pulso / 60.0f;
        fase -= std::floor(fase);
    }

    // Lo que se hincha el corazon ahora (0..1): el "lub" (fuerte) y el "dub"
    // (mas flojo) un poco despues, como suena con un estetoscopio.
    float latido() const {
        auto campana = [](float x, float c, float a) { const float d = (x - c) / a; return std::exp(-d * d); };
        return campana(fase, 0.06f, 0.045f) + 0.55f * campana(fase, 0.24f, 0.04f);
    }
};

} // namespace CorazonJugador
