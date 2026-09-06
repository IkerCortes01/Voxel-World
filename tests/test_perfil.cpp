#include <doctest/doctest.h>
#include "render/PerfilRendimiento.h"

// ============================================================================
// EL PERFIL DE RENDIMIENTO POR HARDWARE
// ============================================================================
// Lo que se verifica:
//
//   1. RECONOCIMIENTO - GPUs reales caen en el nivel que les toca
//   2. PRUDENCIA      - lo desconocido NUNCA se trata como potente
//   3. COHERENCIA     - mejor maquina => nunca peores ajustes
//   4. ROBUSTEZ       - cadenas vacias, nulas o basura no rompen nada
//
// La 2 y la 4 son las que hacen que el juego se pueda llevar en un USB a un
// ordenador cualquiera sin que se trabe: ante la duda, lo conservador.
// ============================================================================

using namespace Render;

// ============================================================================
// 1. RECONOCIMIENTO DE GPUs REALES
// ============================================================================

TEST_CASE("Perfil: la maquina de referencia (HD 4000) sale como MINIMO") {
    // Es la maquina donde se midio todo este motor: 756.000 caras a la vista
    // cuestan 10,4 ms solo de GPU, o sea un techo de ~96 FPS aunque la CPU no
    // hiciera absolutamente nada. Tiene que salir en el nivel mas bajo.
    const Perfil p = detectar(
        "Intel(R) HD Graphics 4000", "Intel", 8, 8192);

    CHECK(p.nivel == NivelMaquina::MINIMO);
    CHECK(p.distanciaRender <= 3);
}

TEST_CASE("Perfil: el render por software sale como MINIMO") {
    // Sin GPU de verdad detras (maquina virtual, sin drivers). Si esto saliera
    // alto, el juego seria injugable en el acto.
    CHECK(detectar("llvmpipe (LLVM 15.0.6, 256 bits)", "Mesa", 8, 16000).nivel
          == NivelMaquina::MINIMO);
    CHECK(detectar("SwiftShader Device", "Google", 8, 16000).nivel
          == NivelMaquina::MINIMO);
    CHECK(detectar("GDI Generic", "Microsoft", 4, 8192).nivel
          == NivelMaquina::MINIMO);
}

TEST_CASE("Perfil: las dedicadas modernas salen como ALTO") {
    CHECK(detectar("NVIDIA GeForce RTX 4070", "NVIDIA", 16, 32000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("AMD Radeon RX 7800 XT", "AMD", 16, 32000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("NVIDIA GeForce GTX 1660 Ti", "NVIDIA", 12, 16000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("Intel(R) Arc(TM) A770 Graphics", "Intel", 16, 32000).nivel
          == NivelMaquina::ALTO);
}

TEST_CASE("Perfil: Apple Silicon es integrada pero rapida") {
    CHECK(detectar("Apple M1 Pro", "Apple", 10, 16000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("Apple M3 Max", "Apple", 16, 48000).nivel
          == NivelMaquina::ALTO);
}

TEST_CASE("Perfil: las integradas actuales van a la zona media-baja") {
    // Van bien, pero no son una dedicada. Ni MINIMO ni ALTO.
    const Perfil iris = detectar("Intel(R) Iris(R) Xe Graphics", "Intel", 8, 16000);
    CHECK(iris.nivel == NivelMaquina::BAJO);

    const Perfil vega = detectar("AMD Radeon(TM) Vega 8 Graphics", "AMD", 8, 16000);
    CHECK(vega.nivel == NivelMaquina::BAJO);
}

// ============================================================================
// 2. PRUDENCIA: LO DESCONOCIDO NO SE TRATA COMO POTENTE
// ============================================================================
// Es la regla que hace que llevar el juego a otro PC sea seguro. Un juego que
// va corto de vista pero fluido se siente bien; uno a tirones se siente roto,
// y el jugador no sabe que puede subir un ajuste.
//
// Ademas el regulador SUBE la calidad cuando sobra margen, asi que equivocarse
// por abajo se corrige solo en segundos. Por arriba, no.

TEST_CASE("Perfil: una GPU desconocida NUNCA sale como ALTO") {
    const char* raras[] = {
        "Mali-G78",
        "Adreno (TM) 650",
        "VideoCore VII",
        "Tarjeta Grafica Marca Blanca 9000",
        "PowerVR Rogue GE8320",
        ""
    };

    for (const char* nombre : raras) {
        // Incluso con MUCHA CPU y MUCHA RAM: si no se reconoce la GPU, no se
        // supone que sea buena.
        const Perfil p = detectar(nombre, "Desconocido", 32, 64000);
        INFO("GPU: ", nombre);
        CHECK(p.nivel != NivelMaquina::ALTO);
    }
}

TEST_CASE("Perfil: sin GPU reconocida y con poca maquina, lo minimo") {
    const Perfil p = detectar("Chip Grafico Raro", "Nadie", 2, 4096);
    CHECK(p.nivel == NivelMaquina::MINIMO);
    CHECK(p.distanciaRender <= 3);
}

TEST_CASE("Perfil: sin GPU reconocida pero con buena CPU y RAM, zona media") {
    // Un equipo con 8 nucleos y 16 GB probablemente no es una tostadora,
    // aunque su GPU no este en la lista.
    const Perfil p = detectar("GPU No Listada", "Nadie", 8, 16000);
    CHECK(p.nivel == NivelMaquina::MEDIO);
    CHECK(p.nivel != NivelMaquina::ALTO);   // pero tampoco se dispara
}

// ============================================================================
// 3. COHERENCIA: MEJOR MAQUINA, NUNCA PEORES AJUSTES
// ============================================================================

TEST_CASE("Perfil: la distancia de render crece con el nivel") {
    const Perfil mn = perfilPara(NivelMaquina::MINIMO, 8);
    const Perfil bj = perfilPara(NivelMaquina::BAJO,   8);
    const Perfil md = perfilPara(NivelMaquina::MEDIO,  8);
    const Perfil al = perfilPara(NivelMaquina::ALTO,   8);

    CHECK(mn.distanciaRender < bj.distanciaRender);
    CHECK(bj.distanciaRender < md.distanciaRender);
    CHECK(md.distanciaRender < al.distanciaRender);
}

TEST_CASE("Perfil: el presupuesto de mallado crece con el nivel") {
    CHECK(perfilPara(NivelMaquina::MINIMO, 8).presupuestoMalladoMs
        < perfilPara(NivelMaquina::BAJO,   8).presupuestoMalladoMs);
    CHECK(perfilPara(NivelMaquina::BAJO,   8).presupuestoMalladoMs
        < perfilPara(NivelMaquina::MEDIO,  8).presupuestoMalladoMs);
    CHECK(perfilPara(NivelMaquina::MEDIO,  8).presupuestoMalladoMs
        < perfilPara(NivelMaquina::ALTO,   8).presupuestoMalladoMs);
}

TEST_CASE("Perfil: todos los niveles apuntan a 120 FPS o mas") {
    // Lo que se pidio: fluidez por defecto. Ningun nivel se conforma con 60.
    for (int i = 0; i <= 3; ++i) {
        const Perfil p = perfilPara((NivelMaquina)i, 8);
        INFO("nivel ", nombreNivel((NivelMaquina)i));
        CHECK(p.fpsObjetivo >= 120);
    }

    // Y las maquinas justas apuntan MAS alto en FPS, porque compensan con
    // menos distancia: es el intercambio "fluidez primero".
    CHECK(perfilPara(NivelMaquina::MINIMO, 8).fpsObjetivo >= 150);
}

// ============================================================================
// 4. ROBUSTEZ: NADA DE ESTO PUEDE ROMPER EL ARRANQUE
// ============================================================================
// Si la deteccion falla, el juego no arranca. Tiene que aguantar cualquier
// cosa que devuelva un driver.

TEST_CASE("Perfil: punteros nulos no rompen nada") {
    const Perfil p = detectar(nullptr, nullptr, 4, 8192);
    CHECK(p.distanciaRender >= DISTANCIA_MINIMA);
    CHECK(p.distanciaRender <= DISTANCIA_MAXIMA);
    CHECK(p.hilosTrabajo >= 1);
}

TEST_CASE("Perfil: cero nucleos y cero memoria no rompen nada") {
    // hardware_concurrency() puede devolver 0 si no lo sabe.
    const Perfil p = detectar("Lo que sea", "Quien sea", 0, 0);
    CHECK(p.hilosTrabajo >= 1);
    CHECK(p.distanciaRender >= DISTANCIA_MINIMA);
}

TEST_CASE("Perfil: siempre queda al menos un hilo para el resto del sistema") {
    // Con N nucleos nunca se piden N hilos de trabajo: el hilo principal
    // dibuja, y el sistema operativo tambien existe.
    for (unsigned n = 1; n <= 32; ++n) {
        const Perfil p = perfilPara(NivelMaquina::ALTO, n);
        INFO("nucleos = ", n);
        CHECK(p.hilosTrabajo >= 1);
        CHECK(p.hilosTrabajo <= 6);          // tope duro
        if (n > 2) CHECK(p.hilosTrabajo < (int)n);
    }
}

TEST_CASE("Perfil: la distancia siempre cae dentro de los topes") {
    for (int i = 0; i <= 3; ++i) {
        const Perfil p = perfilPara((NivelMaquina)i, 8);
        CHECK(p.distanciaRender >= DISTANCIA_MINIMA);
        CHECK(p.distanciaRender <= DISTANCIA_MAXIMA);
    }
}

TEST_CASE("Perfil: acotarDistancia respeta los limites") {
    CHECK(acotarDistancia(-5) == DISTANCIA_MINIMA);
    CHECK(acotarDistancia(0)  == DISTANCIA_MINIMA);
    CHECK(acotarDistancia(99) == DISTANCIA_MAXIMA);
    CHECK(acotarDistancia(5)  == 5);
}

TEST_CASE("Perfil: el nombre de la GPU se conserva para el log") {
    const Perfil p = detectar("Intel(R) HD Graphics 4000", "Intel", 8, 8192);
    CHECK(p.descripcionGPU == "Intel(R) HD Graphics 4000");

    // Y si no hay nombre, algo legible en vez de una cadena vacia.
    const Perfil q = detectar("", "", 4, 8192);
    CHECK_FALSE(q.descripcionGPU.empty());
}

TEST_CASE("Perfil: reconocer no depende de mayusculas") {
    // Los drivers no se ponen de acuerdo en como escriben su nombre.
    CHECK(detectar("NVIDIA GEFORCE RTX 3060", "NVIDIA", 8, 16000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("nvidia geforce rtx 3060", "nvidia", 8, 16000).nivel
          == NivelMaquina::ALTO);
    CHECK(detectar("Intel(R) hd graphics 4000", "intel", 8, 8192).nivel
          == NivelMaquina::MINIMO);
}
