#include <doctest/doctest.h>
#include "audio/FootstepSystem.h"
#include <cmath>
#include <set>

// ============================================================================
// LAS PISADAS: 100 CAPAS POR MATERIAL, COMBINADAS EN UNA PISADA UNICA
// ============================================================================

using namespace Audio;

TEST_CASE("Pisadas: cada material tiene 100 capas distintas y no vacias") {
    for (int m = 0; m < (int)StepMaterial::COUNT; ++m) {
        std::set<size_t> largos;
        int fases[3] = {};
        for (int c = 0; c < FootstepSystem::CAPAS; ++c) {
            const FootstepSynth::Capa k = FootstepSynth::capa((StepMaterial)m, (uint32_t)(m * 7919 + c * 104729 + 17));
            REQUIRE(k.s.size() > 100);
            for (float v : k.s) REQUIRE(std::isfinite(v));
            largos.insert(k.s.size());
            fases[k.fase]++;
        }
        CHECK(largos.size() > 40);           // de verdad distintas
        CHECK(fases[0] + fases[1] + fases[2] == FootstepSystem::CAPAS);
    }
}

TEST_CASE("Pisadas: cada pisada compuesta es unica, suave y de volumen comodo") {
    static FootstepSystem f;     // grande: no en la pila
    AudioMixer mezclador;        // sin abrir: solo hace falta para init
    f.init(&mezclador);
    for (int m = 0; m < (int)StepMaterial::COUNT; ++m) {
        std::set<uint64_t> huellas;
        for (int k = 0; k < 40; ++k) {
            const SoundBuffer* b = f.componer((StepMaterial)m, 0.6f);
            REQUIRE(b);
            const auto& s = b->samples;
            REQUIRE(s.size() > 22050 / 20);           // mas de 50 ms
            REQUIRE(s.size() < 22050 * 35 / 100);     // menos de 350 ms
            int pico = 0;
            uint64_t h = 1469598103934665603ull;
            for (int16_t v : s) { pico = (std::max)(pico, std::abs((int)v)); h = (h ^ (uint16_t)v) * 1099511628211ull; }
            // Normalizada: ni muda ni saturada (si el golpe del talon cae en el
            // primer 1,5 ms, la entrada suave le quita algo de pico).
            CHECK(pico > 32000 * 0.4);
            CHECK(pico < 32000 * 0.75);
            // Empieza y acaba sin clic.
            CHECK(std::abs((int)s.front()) < 2000);
            CHECK(std::abs((int)s.back()) < 2000);
            huellas.insert(h);
        }
        CHECK(huellas.size() == 40);
    }
}
