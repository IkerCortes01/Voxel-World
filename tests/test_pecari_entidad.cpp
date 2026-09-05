#include <doctest/doctest.h>
#include "fauna/PecariEntidad.h"
#include <vector>
#include <cmath>

// ============================================================================
// TESTS DE LA ENTIDAD VIVA
// ============================================================================
// Lo que se verifica es que el animal se COMPORTA como una manada de
// pecaries, no como puntos que flotan:
//
//   1. COHESION   - no se dispersan; es la propiedad MEDIDA que los define
//   2. SEPARACION - no se atraviesan entre si
//   3. PATAS      - el ciclo de paso va ligado a la velocidad real, sin
//                   patinaje
//   4. ANATOMIA   - el cuerpo dibujado respeta los rasgos diagnosticos
//   5. ESTABILIDAD- la simulacion no explota ni con miles de pasos
// ============================================================================

using namespace Fauna;

// Crea una manada de n miembros agrupados alrededor de (cx, cz).
static std::vector<PecariAgente> CrearManada(int n, float cx, float cz, uint32_t seedBase = 1234) {
    std::vector<PecariAgente> m;
    for (int i = 0; i < n; ++i) {
        PecariAgente p;
        p.id = i;
        p.idManada = 0;
        // Repartidos en un anillo pequeno.
        const float ang = (float)i * 2.39996323f;
        p.x = cx + std::cos(ang) * 3.0f;
        p.z = cz + std::sin(ang) * 3.0f;
        p.y = 64.0f;
        p.semilla = seedBase + (uint32_t)i * 7919u;
        p.escala = EscalaDeEtapa(p.etapa);
        m.push_back(p);
    }
    return m;
}

// Avanza la simulacion de toda la manada n pasos.
static void Simular(std::vector<PecariAgente>& manada, int pasos, float dt = 0.05f) {
    std::vector<const PecariAgente*> vecinos;
    for (int paso = 0; paso < pasos; ++paso) {
        const CentroManada centro = CalcularCentro(manada, 0);
        for (size_t i = 0; i < manada.size(); ++i) {
            BuscarVecinos(manada, i, vecinos);
            ActualizarPecari(manada[i], centro, vecinos, dt);

            // ⭐ INTEGRAR LA POSICION.
            //
            // ActualizarPecari ya NO mueve al animal: solo deja la intencion
            // (vx, vz). De moverlo se encarga MundoPecaries, que es quien
            // tiene el mundo delante y puede negarse a atravesar una pared
            // (ver moverConColision).
            //
            // Aqui se integra a pelo porque estos tests prueban el
            // COMPORTAMIENTO DE MANADA en campo abierto, sin terreno: es
            // exactamente lo que hace el juego cuando no hay nada que
            // estorbe. La colision tiene sus propios tests en
            // test_pecari_mundo.cpp, con paredes de verdad.
            manada[i].x += manada[i].vx * dt;
            manada[i].z += manada[i].vz * dt;
        }
    }
}

TEST_CASE("Entidad: la manada NO se dispersa") {
    std::vector<PecariAgente> manada = CrearManada(9, 100.0f, 100.0f);

    // Es LA propiedad que define a esta especie.
    // MEDIDO (Byers y Bekoff 1981): "la unidad social es una manada cohesiva
    // en la que se mantienen distancias interindividuales pequenas".
    //
    // Si tras dos minutos de simulacion los animales se han desperdigado por
    // el mapa, no es una manada: son nueve animales sueltos.
    Simular(manada, 2400);   // 2400 * 0.05 s = 120 s

    const CentroManada c = CalcularCentro(manada, 0);
    for (const PecariAgente& p : manada) {
        const float dx = p.x - c.x;
        const float dz = p.z - c.z;
        const float dist = std::sqrt(dx*dx + dz*dz);
        // Nadie mas alla del radio de alarma: si alguien se aleja tanto,
        // la fuerza de cohesion lo trae de vuelta.
        CHECK(dist < PecariMovimiento::RADIO_ALARMA_BLOQUES * 1.5f);
    }
}

TEST_CASE("Entidad: un rezagado vuelve al grupo") {
    std::vector<PecariAgente> manada = CrearManada(8, 50.0f, 50.0f);

    // Se aparta a uno MUY lejos, como si se hubiera perdido.
    manada[0].x = 50.0f + 40.0f;
    manada[0].z = 50.0f;

    const CentroManada antes = CalcularCentro(manada, 0);
    const float dxA = manada[0].x - antes.x;
    const float dzA = manada[0].z - antes.z;
    const float distAntes = std::sqrt(dxA*dxA + dzA*dzA);

    Simular(manada, 600);   // 30 s

    const CentroManada despues = CalcularCentro(manada, 0);
    const float dxD = manada[0].x - despues.x;
    const float dzD = manada[0].z - despues.z;
    const float distDespues = std::sqrt(dxD*dxD + dzD*dzD);

    // Debe haberse acercado claramente.
    CHECK(distDespues < distAntes);
    // Y debe estar ya dentro de un margen razonable del grupo.
    CHECK(distDespues < PecariMovimiento::RADIO_ALARMA_BLOQUES * 1.5f);
}

TEST_CASE("Entidad: el rezagado TROTA, no pasea") {
    std::vector<PecariAgente> manada = CrearManada(6, 0.0f, 0.0f);
    manada[0].x = 60.0f;   // muy lejos
    manada[0].z = 0.0f;

    std::vector<const PecariAgente*> vecinos;
    const CentroManada centro = CalcularCentro(manada, 0);
    BuscarVecinos(manada, 0, vecinos);
    ActualizarPecari(manada[0], centro, vecinos, 0.05f);

    // Alcanzar a la manada es urgente: debe trotar.
    CHECK(manada[0].modo == ModoPecari::TROTANDO);
}

TEST_CASE("Entidad: no se atraviesan entre si") {
    // Se colocan DOS pecaries casi encima uno del otro y se comprueba que la
    // fuerza de separacion los aparta.
    std::vector<PecariAgente> manada;
    for (int i = 0; i < 2; ++i) {
        PecariAgente p;
        p.id = i; p.idManada = 0;
        p.x = 10.0f + (float)i * 0.1f;   // casi superpuestos
        p.z = 10.0f;
        p.y = 64.0f;
        p.semilla = 555u + (uint32_t)i;
        p.escala = 1.0f;
        manada.push_back(p);
    }

    Simular(manada, 200);

    const float dx = manada[0].x - manada[1].x;
    const float dz = manada[0].z - manada[1].z;
    const float dist = std::sqrt(dx*dx + dz*dz);

    // Deben haberse separado hasta al menos rozar la distancia minima.
    // Se admite un margen: la separacion compite con la cohesion.
    CHECK(dist > PecariMovimiento::SEPARACION_MINIMA * 0.6f);
}

TEST_CASE("Entidad: las patas NO patinan") {
    // La fase del paso debe avanzar SOLO cuando el animal se mueve. Es lo que
    // separa una animacion creible de un muneco deslizandose.
    PecariAgente quieto;
    quieto.id = 0; quieto.idManada = -1;
    quieto.x = 0.0f; quieto.z = 0.0f; quieto.y = 64.0f;
    quieto.semilla = 99u;
    quieto.escala = 1.0f;
    // Su objetivo es exactamente donde ya esta: no tiene a donde ir.
    quieto.objetivoX = 0.0f;
    quieto.objetivoZ = 0.0f;
    quieto.tiempoHastaDecision = 1000.0f;   // que no elija otro destino

    const float faseInicial = quieto.fasePaso;
    CentroManada sinManada;
    std::vector<const PecariAgente*> nadie;

    for (int i = 0; i < 100; ++i) {
        ActualizarPecari(quieto, sinManada, nadie, 0.05f);
    }

    // Parado: la fase no avanza y el modo es QUIETO.
    CHECK(quieto.modo == ModoPecari::QUIETO);
    CHECK(quieto.fasePaso == doctest::Approx(faseInicial));

    // En movimiento: la fase SI avanza.
    PecariAgente andando = quieto;
    andando.objetivoX = 100.0f;    // lejos, para que camine
    andando.objetivoZ = 0.0f;
    andando.tiempoHastaDecision = 1000.0f;
    const float faseAntes = andando.fasePaso;
    for (int i = 0; i < 20; ++i) {
        ActualizarPecari(andando, sinManada, nadie, 0.05f);
    }
    CHECK(andando.fasePaso != doctest::Approx(faseAntes));
    CHECK(andando.modo != ModoPecari::QUIETO);
}

TEST_CASE("Entidad: gira progresivamente, no de golpe") {
    PecariAgente p;
    p.id = 0; p.idManada = -1;
    p.x = 0.0f; p.z = 0.0f; p.y = 64.0f;
    p.semilla = 7u; p.escala = 1.0f;
    p.orientacion = 0.0f;               // mirando a +Z
    p.objetivoX = 0.0f;
    p.objetivoZ = -50.0f;               // justo detras: giro de 180 grados
    p.tiempoHastaDecision = 1000.0f;

    CentroManada sinManada;
    std::vector<const PecariAgente*> nadie;

    // Un solo paso no puede girarlo 180 grados: seria un tiron visible.
    ActualizarPecari(p, sinManada, nadie, 0.05f);
    const float giroEnUnPaso = std::fabs(p.orientacion);
    CHECK(giroEnUnPaso <= PecariMovimiento::VEL_GIRO * 0.05f + 1e-4f);
    CHECK(giroEnUnPaso > 0.0f);   // pero algo gira
}

TEST_CASE("Entidad: la simulacion es estable a largo plazo") {
    std::vector<PecariAgente> manada = CrearManada(12, 200.0f, 200.0f);

    // Diez minutos de juego. Nada debe volverse NaN, infinito, ni escaparse
    // al infinito. Es el test que detecta una fuerza mal signada.
    Simular(manada, 12000);

    for (const PecariAgente& p : manada) {
        CHECK(std::isfinite(p.x));
        CHECK(std::isfinite(p.z));
        CHECK(std::isfinite(p.orientacion));
        CHECK(std::isfinite(p.vx));
        CHECK(std::isfinite(p.vz));
        // No se han ido a la otra punta del mundo.
        CHECK(std::fabs(p.x - 200.0f) < 200.0f);
        CHECK(std::fabs(p.z - 200.0f) < 200.0f);
    }
}

TEST_CASE("Entidad: la vecindad topologica se limita a K_VECINOS") {
    // Ballerini et al. 2008 (PNAS): ~6-7 vecinos, independientemente de la
    // distancia. El limite es biologico Y de rendimiento.
    std::vector<PecariAgente> grande = CrearManada(30, 0.0f, 0.0f);

    std::vector<const PecariAgente*> vecinos;
    BuscarVecinos(grande, 0, vecinos);

    CHECK((int)vecinos.size() <= PecariMovimiento::K_VECINOS);
    CHECK(vecinos.size() > 0);

    // Y deben ser los MAS PROXIMOS: ninguno excluido puede estar mas cerca
    // que el mas lejano incluido.
    float peorIncluido = 0.0f;
    for (const PecariAgente* v : vecinos) {
        const float dx = v->x - grande[0].x;
        const float dz = v->z - grande[0].z;
        const float d = std::sqrt(dx*dx + dz*dz);
        if (d > peorIncluido) peorIncluido = d;
    }
    // Se comprueba que hay al menos uno excluido mas lejos que el peor
    // incluido (con 30 miembros y k=7, tiene que haberlos).
    int excluidosMasLejos = 0;
    for (size_t i = 1; i < grande.size(); ++i) {
        bool incluido = false;
        for (const PecariAgente* v : vecinos) if (v == &grande[i]) { incluido = true; break; }
        if (incluido) continue;
        const float dx = grande[i].x - grande[0].x;
        const float dz = grande[i].z - grande[0].z;
        if (std::sqrt(dx*dx + dz*dz) >= peorIncluido - 1e-3f) ++excluidosMasLejos;
    }
    CHECK(excluidosMasLejos > 0);
}

TEST_CASE("Entidad: solo interactua con SU manada") {
    // Dos manadas distintas no deben mezclarse.
    std::vector<PecariAgente> todos = CrearManada(5, 0.0f, 0.0f);
    for (int i = 0; i < 5; ++i) {
        PecariAgente p;
        p.id = 100 + i;
        p.idManada = 1;               // OTRA manada
        p.x = 2.0f; p.z = 2.0f;       // muy cerca fisicamente
        p.y = 64.0f;
        p.semilla = 4242u + (uint32_t)i;
        p.escala = 1.0f;
        todos.push_back(p);
    }

    std::vector<const PecariAgente*> vecinos;
    BuscarVecinos(todos, 0, vecinos);

    // Ninguno de los vecinos puede ser de la manada 1.
    for (const PecariAgente* v : vecinos) {
        CHECK(v->idManada == 0);
    }
}

// ============================================================================
// ANATOMIA DEL CUERPO DIBUJADO
// ============================================================================

TEST_CASE("Anatomia: el cuerpo tiene todas sus piezas") {
    PecariAgente p;
    p.x = 10.0f; p.y = 64.0f; p.z = 10.0f;
    p.escala = 1.0f;
    p.orientacion = 0.0f;

    std::vector<CajaDibujo> cuerpo;
    ConstruirCuerpo(p, cuerpo);

    // tronco + cabeza + hocico + collar + cresta + 4 patas + cola = 10
    CHECK(cuerpo.size() == 10);
}

TEST_CASE("Anatomia: el hocico es MAS ESTRECHO que la cabeza") {
    // Es un rasgo diagnostico: la cabeza es "en cuna", ancha en el craneo y
    // afilada hacia el hocico. Si el hocico fuera igual de ancho, la cabeza
    // pareceria un ladrillo y el animal no se reconoceria.
    PecariAgente p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    p.escala = 1.0f; p.orientacion = 0.0f;

    std::vector<CajaDibujo> cuerpo;
    ConstruirCuerpo(p, cuerpo);

    const CajaDibujo& cabeza = cuerpo[1];
    const CajaDibujo& hocico = cuerpo[2];
    CHECK(hocico.hx < cabeza.hx);

    // Y el afilado debe rondar el valor anatomico declarado (~0.55).
    CHECK(hocico.hx / cabeza.hx == doctest::Approx(::Pecari::Externo::AFILADO_CABEZA).epsilon(0.01));
}

TEST_CASE("Anatomia: la cola es MINUSCULA, no de cerdo") {
    // MEDIDO: 1.2 cm. El error mas facil de cometer con este animal es darle
    // cola de cerdo, y se veria al instante.
    PecariAgente p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    p.escala = 1.0f; p.orientacion = 0.0f;

    std::vector<CajaDibujo> cuerpo;
    ConstruirCuerpo(p, cuerpo);

    const CajaDibujo& cola = cuerpo.back();
    const CajaDibujo& tronco = cuerpo[0];

    // La cola debe ser una fraccion diminuta del tronco.
    CHECK(cola.hz < tronco.hz * 0.05f);

    // En bloques: 1.2 cm / 0.60 m = 0.02 bloques de largo total.
    CHECK(cola.hz * 2.0f < 0.03f);
}

TEST_CASE("Anatomia: las patas son finas frente a un cuerpo macizo") {
    // El contraste es una firma visual de la especie: patas largas y
    // delgadas bajo un tronco en barril.
    PecariAgente p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    p.escala = 1.0f; p.orientacion = 0.0f;

    std::vector<CajaDibujo> cuerpo;
    ConstruirCuerpo(p, cuerpo);

    const CajaDibujo& tronco = cuerpo[0];
    // Las cuatro patas son los indices 5,6,7,8
    for (int i = 5; i <= 8; ++i) {
        CHECK(cuerpo[i].hx < tronco.hx * 0.5f);
    }
}

TEST_CASE("Anatomia: las crias son mucho mas pequenas") {
    // DERIVADO: 0.5 kg al nacer frente a 18.7 kg adulto. La masa va con el
    // cubo de la longitud, asi que la cria mide ~0.30 del adulto.
    PecariAgente adulto;
    adulto.escala = EscalaDeEtapa(EtapaPecari::ADULTO);
    PecariAgente cria;
    cria.escala = EscalaDeEtapa(EtapaPecari::NEONATO);

    CHECK(cria.escala < adulto.escala * 0.4f);

    std::vector<CajaDibujo> cuerpoAdulto, cuerpoCria;
    ConstruirCuerpo(adulto, cuerpoAdulto);
    ConstruirCuerpo(cria, cuerpoCria);

    // Todas las piezas de la cria son mas pequenas.
    for (size_t i = 0; i < cuerpoAdulto.size(); ++i) {
        CHECK(cuerpoCria[i].hx <= cuerpoAdulto[i].hx);
        CHECK(cuerpoCria[i].hy <= cuerpoAdulto[i].hy);
    }

    // Y la escala respeta el anclaje medido.
    CHECK(EscalaDeEtapa(EtapaPecari::NEONATO) == doctest::Approx(0.30f));
}

TEST_CASE("Anatomia: el cuerpo ROTA con la orientacion") {
    // Si el cuerpo no rotara, el animal caminaria de lado como un cangrejo.
    PecariAgente mirandoZ;
    mirandoZ.x = 0.0f; mirandoZ.y = 0.0f; mirandoZ.z = 0.0f;
    mirandoZ.escala = 1.0f;
    mirandoZ.orientacion = 0.0f;          // hacia +Z

    PecariAgente mirandoX = mirandoZ;
    mirandoX.orientacion = 1.5707963f;    // 90 grados: hacia +X

    std::vector<CajaDibujo> a, b;
    ConstruirCuerpo(mirandoZ, a);
    ConstruirCuerpo(mirandoX, b);

    // La cabeza esta delante. Con orientacion 0 debe estar desplazada en +Z;
    // con 90 grados, en +X.
    const CajaDibujo& cabezaA = a[1];
    const CajaDibujo& cabezaB = b[1];

    CHECK(cabezaA.cz > 0.1f);              // adelante en Z
    CHECK(std::fabs(cabezaA.cx) < 0.05f);  // centrada en X

    CHECK(cabezaB.cx > 0.1f);              // ahora adelante en X
    CHECK(std::fabs(cabezaB.cz) < 0.05f);  // centrada en Z
}

TEST_CASE("Anatomia: el cuerpo sigue al animal por el mundo") {
    PecariAgente p;
    p.x = 500.0f; p.y = 70.0f; p.z = -300.0f;
    p.escala = 1.0f; p.orientacion = 0.0f;

    std::vector<CajaDibujo> cuerpo;
    ConstruirCuerpo(p, cuerpo);

    // El tronco debe estar cerca de la posicion del animal, no en el origen.
    const CajaDibujo& tronco = cuerpo[0];
    CHECK(std::fabs(tronco.cx - 500.0f) < 1.0f);
    CHECK(std::fabs(tronco.cz - (-300.0f)) < 1.0f);
    // Y por encima de sus pies (y es el nivel del suelo).
    CHECK(tronco.cy > 70.0f);
    CHECK(tronco.cy < 71.0f);
}

TEST_CASE("Anatomia: la cresta se levanta al erizarse") {
    // MEDIDO (cualitativo): el animal levanta las cerdas del lomo al
    // alarmarse, y erizado parece mucho mas grande.
    PecariAgente calmado;
    calmado.escala = 1.0f; calmado.erizado = 0.0f;
    PecariAgente alarmado = calmado;
    alarmado.erizado = 1.0f;

    std::vector<CajaDibujo> a, b;
    ConstruirCuerpo(calmado, a);
    ConstruirCuerpo(alarmado, b);

    // La cresta es el indice 4.
    CHECK(b[4].hy > a[4].hy);

    // El factor de erizado debe rondar el ~3.5 declarado.
    const float factor = b[4].hy / a[4].hy;
    CHECK(factor > 2.5f);
    CHECK(factor < 4.5f);
}

TEST_CASE("Anatomia: la hitbox sale de las medidas MEDIDAS") {
    PecariAgente p;
    p.escala = 1.0f;
    const HitboxPecari h = HitboxDe(p);

    // Alto = altura a la cruz MEDIDA (0.44 m) / 0.60 m por bloque = 0.733
    CHECK(h.alto == doctest::Approx(0.44f / 0.60f).epsilon(0.01));

    // Largo total = longitud cabeza-cuerpo MEDIDA (0.95 m) => semi = 0.475 m
    CHECK(h.semiLargo == doctest::Approx(0.475f / 0.60f).epsilon(0.01));

    // Y una cria tiene hitbox proporcionalmente menor.
    PecariAgente cria;
    cria.escala = EscalaDeEtapa(EtapaPecari::NEONATO);
    const HitboxPecari hc = HitboxDe(cria);
    CHECK(hc.alto < h.alto);
}
