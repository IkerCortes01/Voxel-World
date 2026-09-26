#ifndef AMBIENTE_MUNDO_H
#define AMBIENTE_MUNDO_H

#include "AmbienteSynth.h"
#include "../../src/terrain/BiomeTypes.h"

#include <cstdint>
#include <vector>

// ============================================================================
// CUANDO SUENA CADA AMBIENTE
// ============================================================================
// RESPONSABILIDAD UNICA: decidir QUE se oye y CUANDO, segun donde este el
// jugador. No sintetiza (eso es AmbienteSynth) ni reproduce (eso es el mixer).
//
// La separacion importa: asi la decision se puede probar sin tarjeta de sonido
// -- un test le da un bioma y una hora y comprueba que pide lo correcto.
//
// ----------------------------------------------------------------------------
// DE QUE DEPENDE LO QUE SE OYE
// ----------------------------------------------------------------------------
//   BIOMA   un pinar no suena como un desierto
//   HORA    los grillos son de noche y las chicharras de mediodia
//   CUEVA   bajo tierra no hay viento ni pajaros: hay goteo y retumbo
//
// La regla de la cueva MANDA sobre las otras dos: estar bajo tierra cambia el
// ambiente entero, no lo matiza.
//
// ----------------------------------------------------------------------------
// DOS CAPAS
// ----------------------------------------------------------------------------
//   FONDO      un bucle continuo que no para nunca (viento, retumbo). Se
//              renueva al terminar, con otra semilla, para que no se oiga la
//              repeticion.
//
//   PUNTUALES  sonidos sueltos que aparecen de vez en cuando (un pajaro, una
//              gota, un coyote). Son los que dan vida: un fondo solo se
//              vuelve invisible en unos segundos.
//
// El intervalo entre puntuales es ALEATORIO dentro de un rango. Con un
// intervalo fijo el oido detecta el patron enseguida y deja de sonar a
// naturaleza para sonar a maquina.
// ============================================================================

namespace Audio {

// Lo que el mundo le cuenta al ambiente. Todo lo que hace falta para decidir.
struct ContextoAmbiente {
    TerrainGen::BiomeType bioma = TerrainGen::BIOME_PLAINS;

    // 0..1 a lo largo del dia. 0 = medianoche, 0.5 = mediodia.
    float horaDelDia = 0.5f;

    // Bajo tierra y sin cielo encima.
    bool enCueva = false;

    // Altura del jugador, para el viento de los picos.
    float altura = 64.0f;
};

// Una peticion de sonido. La resuelve quien tenga el mixer.
struct PeticionSonido {
    Ambiente tipo;
    float    volumen;
    float    tono;      // multiplicador de pitch
};

class AmbienteMundo {
public:
    // ------------------------------------------------------------------------
    // ¿ES DE NOCHE?
    // ------------------------------------------------------------------------
    // El corte no esta en 0.25/0.75 exactos sino algo dentro, para que el
    // cambio no caiga justo en el amanecer visual: los grillos empiezan antes
    // de que anochezca del todo y siguen un rato despues de clarear, que es lo
    // que pasa de verdad.
    static bool esNoche(float hora) {
        return hora < 0.22f || hora > 0.78f;
    }

    static bool esMediodia(float hora) {
        return hora > 0.38f && hora < 0.62f;
    }

    // ------------------------------------------------------------------------
    // EL FONDO CONTINUO
    // ------------------------------------------------------------------------
    // Siempre hay uno. Devuelve que ambiente toca y a que volumen.
    static PeticionSonido fondo(const ContextoAmbiente& c) {
        // La cueva manda: bajo tierra no llega ni el viento ni el follaje.
        if (c.enCueva) {
            return { Ambiente::RetumboCueva, 0.55f, 1.0f };
        }

        // En un bosque lo que se oye de fondo son las hojas, no el aire.
        if (TerrainGen::EsBoscoso(c.bioma)) {
            return { Ambiente::Hojas, 0.40f, 1.0f };
        }

        // En alto el viento arrecia. Es el unico sitio donde el fondo sube de
        // volumen de verdad, y es lo que hace que subir una montana se NOTE
        // sin decirselo al jugador.
        const bool enAlto = (c.altura > 110.0f) ||
                            c.bioma == TerrainGen::BIOME_MOUNTAINS ||
                            c.bioma == TerrainGen::BIOME_MOUNTAIN_PEAKS;
        if (enAlto) {
            return { Ambiente::VientoFuerte, 0.50f, 1.0f };
        }

        return { Ambiente::VientoSuave, 0.30f, 1.0f };
    }

    // ------------------------------------------------------------------------
    // LOS PUNTUALES
    // ------------------------------------------------------------------------
    // Devuelve la lista de lo que PUEDE sonar aqui y ahora. Quien llama elige
    // uno al azar cuando toca.
    //
    // Se devuelve la lista entera --en vez de elegir aqui-- para que la
    // decision sea determinista y comprobable: un test puede mirar que en una
    // cueva no aparece ningun pajaro sin depender de ningun sorteo.
    static void puntuales(const ContextoAmbiente& c,
                          std::vector<PeticionSonido>& salida) {
        salida.clear();

        // --- BAJO TIERRA ---
        if (c.enCueva) {
            // El goteo es el sonido de cueva por excelencia. Se repite con
            // tono variable para que no suene la misma gota dos veces.
            salida.push_back({ Ambiente::GotaCueva, 0.45f, 1.0f });
            salida.push_back({ Ambiente::GotaCueva, 0.30f, 0.82f });
            salida.push_back({ Ambiente::GotaCueva, 0.38f, 1.24f });
            return;   // ⭐ y nada mas: no hay pajaros bajo tierra
        }

        const bool noche = esNoche(c.horaDelDia);

        // --- DE NOCHE ---
        if (noche) {
            // Grillos en cualquier sitio con algo de vegetacion.
            if (c.bioma != TerrainGen::BIOME_DESERT &&
                c.bioma != TerrainGen::BIOME_MOUNTAIN_PEAKS) {
                salida.push_back({ Ambiente::Grillos, 0.35f, 1.0f });
            }

            // El buho es de bosque: necesita arboles donde posarse.
            if (TerrainGen::EsBoscoso(c.bioma)) {
                salida.push_back({ Ambiente::Buho, 0.30f, 1.0f });
            }

            // El coyote, de zona abierta y seca.
            if (c.bioma == TerrainGen::BIOME_DESERT ||
                c.bioma == TerrainGen::BIOME_PLAINS) {
                salida.push_back({ Ambiente::Coyote, 0.26f, 1.0f });
            }
            return;
        }

        // --- DE DIA ---

        // La chicharra es del calor seco del mediodia. Fuera de esa franja
        // calla, que es lo que hace que el mediodia SUENE a mediodia.
        if (esMediodia(c.horaDelDia) &&
            (c.bioma == TerrainGen::BIOME_DESERT ||
             c.bioma == TerrainGen::BIOME_PLAINS)) {
            salida.push_back({ Ambiente::Chicharra, 0.28f, 1.0f });
        }

        // Pajaros donde hay arboles. Uno cerca y otro lejos: la distancia es
        // lo que da profundidad al bosque.
        if (TerrainGen::EsBoscoso(c.bioma)) {
            salida.push_back({ Ambiente::PajaroDia,    0.32f, 1.0f  });
            salida.push_back({ Ambiente::PajaroLejano, 0.20f, 0.92f });
            salida.push_back({ Ambiente::PajaroDia,    0.28f, 1.15f });
        }

        // En la playa y la llanura, alguno suelto.
        if (c.bioma == TerrainGen::BIOME_PLAINS ||
            c.bioma == TerrainGen::BIOME_BEACH) {
            salida.push_back({ Ambiente::PajaroLejano, 0.22f, 1.0f });
        }
    }

    // ------------------------------------------------------------------------
    // CADA CUANTO SUENA UN PUNTUAL
    // ------------------------------------------------------------------------
    // En segundos, minimo y maximo. Quien llama sortea dentro del rango.
    //
    // POR QUE UN RANGO Y NO UN VALOR FIJO: con un intervalo constante el oido
    // detecta el patron en tres repeticiones y el ambiente deja de sonar a
    // naturaleza. La variacion es lo que lo hace creible.
    static void intervalo(const ContextoAmbiente& c,
                          float& minSeg, float& maxSeg) {
        if (c.enCueva) {
            // El goteo es lento y espaciado: una cueva no gotea sin parar, y
            // si lo hiciera cansaria enseguida.
            minSeg = 3.5f; maxSeg = 11.0f;
            return;
        }

        if (esNoche(c.horaDelDia)) {
            // La noche es mas silenciosa, con sonidos mas separados.
            minSeg = 6.0f; maxSeg = 18.0f;
            return;
        }

        // De dia, en bosque, hay mas actividad.
        if (TerrainGen::EsBoscoso(c.bioma)) {
            minSeg = 2.5f; maxSeg = 9.0f;
            return;
        }

        minSeg = 5.0f; maxSeg = 15.0f;
    }
};

} // namespace Audio

#endif // AMBIENTE_MUNDO_H
