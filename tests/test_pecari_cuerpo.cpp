#include <doctest/doctest.h>
#include "fauna/PecariCuerpo.h"
#include <vector>
#include <cmath>

// ============================================================================
// TESTS DEL CUERPO MEJORADO
// ============================================================================
// Verifican lo que se pidio explicitamente, y ademas que no se rompio la
// anatomia medida al hacerlo:
//
//   1. MEDIDAS      - torso 50x40x46 px, y que eso sigue dando el animal real
//   2. CABEZA APARTE- girar la cabeza NO mueve el cuerpo
//   3. OJOS         - existen, son laterales, parpadean, y NO brillan
//   4. ARTICULACIONES - la pata dobla en tres puntos y solo al andar
//   5. PELO         - existe, es aguti, y llega a la coronilla
//   6. LUZ          - las piezas traen normal y emision para el motor
// ============================================================================

using namespace Fauna;

static PosePecari PoseBase() {
    PosePecari p;
    p.x = 100.0f; p.y = 64.0f; p.z = 100.0f;
    p.orientacionCuerpo = 0.0f;
    p.escala = 1.0f;
    p.semilla = 12345u;
    return p;
}

// ============================================================================
// 1. LAS MEDIDAS PEDIDAS
// ============================================================================

TEST_CASE("Cuerpo: el torso mide 70x56x64 px como se pidio") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    REQUIRE(cuerpo.size() > 0);

    // La primera pieza es el tronco.
    const PiezaCuerpo& tronco = cuerpo[0];

    // Se convierte de vuelta a px para comparar con lo pedido.
    // hx esta en BLOQUES; a metros es *0.60; a px se divide por METROS_POR_PX.
    const float anchoPx = (tronco.hx * 2.0f * 0.60f) / Px::METROS_POR_PX;
    const float altoPx  = (tronco.hy * 2.0f * 0.60f) / Px::METROS_POR_PX;
    const float largoPx = (tronco.hz * 2.0f * 0.60f) / Px::METROS_POR_PX;

    CHECK(largoPx == doctest::Approx(70.0f).epsilon(0.02));   // grueso
    CHECK(anchoPx == doctest::Approx(56.0f).epsilon(0.02));   // ancho
    CHECK(altoPx  == doctest::Approx(64.0f).epsilon(0.02));   // alto
}

TEST_CASE("Cuerpo: la escala se ancla a la altura a la cruz MEDIDA") {
    // EL CONFLICTO QUE ESTE TEST VIGILA:
    //
    // Anclando la escala al LARGO del tronco (50 px = 0.490 m MEDIDOS), el
    // torso de 46 px salia midiendo 0.4508 m de ALTO — mas que la altura a la
    // cruz MEDIDA (0.44 m). El animal no tendria sitio para las patas.
    //
    // Por eso el ancla es la CRUZ, no el largo. Si alguien lo cambia, esto
    // salta.
    CHECK(Px::aM(Px::CRUZ_PX) == doctest::Approx(0.44f));

    // El torso tiene que caber bajo la cruz, dejando hueco para la pata.
    CHECK(Px::aM(Px::TORSO_ALTO_PX) < Px::CRUZ_M);
    CHECK(Px::PATA_LIBRE_PX > 0.0f);
    CHECK(Px::TORSO_ALTO_PX + Px::PATA_LIBRE_PX == Px::CRUZ_PX);

    // Sigue siendo un barril: mas largo que ancho.
    CHECK(Px::TORSO_ANCHO_PX < Px::TORSO_LARGO_PX);

    // Y el animal conserva el orden de magnitud correcto: un pecari, no una
    // vaca ni un raton.
    CHECK(Px::aM(Px::TORSO_LARGO_PX) > 0.20f);
    CHECK(Px::aM(Px::TORSO_LARGO_PX) < 0.60f);
}

TEST_CASE("Cuerpo: el torso es mas macizo que antes pero sigue siendo barril") {
    // Antes el tronco era 0.25 x 0.25 x 0.49 m: practicamente cuadrado de
    // seccion. Ahora es mas alto y ancho, que es lo que se pidio.
    const float anchoM = Px::aM(Px::TORSO_ANCHO_PX);
    const float altoM  = Px::aM(Px::TORSO_ALTO_PX);

    CHECK(anchoM > 0.25f);   // mas ancho que antes
    CHECK(altoM  > 0.25f);   // mas alto que antes

    // Pero sigue siendo mas largo que ancho: la forma de barril se conserva.
    CHECK(Px::aM(Px::TORSO_LARGO_PX) > anchoM);
}

// ============================================================================
// 2. LA CABEZA GIRA SIN MOVER EL CUERPO
// ============================================================================

TEST_CASE("Cuerpo: girar la cabeza NO mueve el tronco") {
    // ESTE ES EL TEST DE LO QUE SE PIDIO LITERALMENTE.
    //
    // Con una lista plana de cajas esto seria imposible sin recalcular todo a
    // mano. Con el esqueleto jerarquico, el tronco ni se entera.
    PosePecari recto = PoseBase();
    PosePecari mirando = PoseBase();
    mirando.giroCabezaY = 1.0f;    // ~57 grados a un lado

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(recto, a);
    ConstruirCuerpoPecari(mirando, b);

    REQUIRE(a.size() == b.size());

    // El TRONCO (pieza 0) debe estar EXACTAMENTE igual.
    CHECK(a[0].cx == doctest::Approx(b[0].cx));
    CHECK(a[0].cy == doctest::Approx(b[0].cy));
    CHECK(a[0].cz == doctest::Approx(b[0].cz));
    CHECK(a[0].hx == doctest::Approx(b[0].hx));
    CHECK(a[0].hy == doctest::Approx(b[0].hy));
    CHECK(a[0].hz == doctest::Approx(b[0].hz));
}

TEST_CASE("Cuerpo: girar la cabeza SI mueve la cabeza y lo que cuelga de ella") {
    PosePecari recto = PoseBase();
    PosePecari mirando = PoseBase();
    mirando.giroCabezaY = 1.0f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(recto, a);
    ConstruirCuerpoPecari(mirando, b);

    // Algo tiene que haberse movido: si no, el giro no hace nada.
    int piezasMovidas = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        const float dx = a[i].cx - b[i].cx;
        const float dz = a[i].cz - b[i].cz;
        if (std::sqrt(dx*dx + dz*dz) > 0.005f) ++piezasMovidas;
    }

    // La cabeza arrastra: hocico, dos ojos (globo+pupila+brillo), dos orejas,
    // dos colmillos y los mechones de la coronilla. Son bastantes piezas.
    CHECK(piezasMovidas >= 8);
}

TEST_CASE("Cuerpo: las patas NO se mueven al girar la cabeza") {
    PosePecari recto = PoseBase();
    PosePecari mirando = PoseBase();
    mirando.giroCabezaX = 0.6f;    // agachar el hocico al suelo
    mirando.giroCabezaY = 0.9f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(recto, a);
    ConstruirCuerpoPecari(mirando, b);
    REQUIRE(a.size() == b.size());

    // Las patas cuelgan del TRONCO, no de la cabeza. Ninguna puede moverse.
    // Se buscan por altura: estan por debajo del centro del torso.
    int patasComprobadas = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].cy < a[0].cy - a[0].hy) {   // por debajo del tronco
            CHECK(a[i].cx == doctest::Approx(b[i].cx));
            CHECK(a[i].cy == doctest::Approx(b[i].cy));
            CHECK(a[i].cz == doctest::Approx(b[i].cz));
            ++patasComprobadas;
        }
    }
    CHECK(patasComprobadas > 0);
}

TEST_CASE("Cuerpo: el cuello tiene limites, la cabeza no gira 360") {
    // Un cuello corto y musculoso no da la vuelta entera. Si se pide un giro
    // absurdo, se recorta.
    PosePecari extremo = PoseBase();
    extremo.giroCabezaY = 99.0f;    // giro imposible

    PosePecari limite = PoseBase();
    limite.giroCabezaY = Cuello::GIRO_MAX_Y;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(extremo, a);
    ConstruirCuerpoPecari(limite, b);

    REQUIRE(a.size() == b.size());
    // Pedir 99 radianes debe dar exactamente lo mismo que pedir el maximo.
    for (size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].cx == doctest::Approx(b[i].cx));
        CHECK(a[i].cz == doctest::Approx(b[i].cz));
    }

    CHECK(LimitarGiroCuelloY( 99.0f) == doctest::Approx(Cuello::GIRO_MAX_Y));
    CHECK(LimitarGiroCuelloY(-99.0f) == doctest::Approx(-Cuello::GIRO_MAX_Y));
    CHECK(LimitarGiroCuelloX( 99.0f) == doctest::Approx(Cuello::GIRO_MAX_X));
}

// ============================================================================
// 3. LOS OJOS
// ============================================================================

TEST_CASE("Cuerpo: tiene dos ojos y son LATERALES") {
    PosePecari pose = PoseBase();
    pose.orientacionCuerpo = 0.0f;   // mirando a +Z
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Se buscan las piezas de color de ojo.
    int ojos = 0;
    float xIzq = 0.0f, xDer = 0.0f;
    for (const PiezaCuerpo& p : cuerpo) {
        if (std::fabs(p.r - Tono::OJO_R) < 0.001f &&
            std::fabs(p.g - Tono::OJO_G) < 0.001f) {
            ++ojos;
            const float dx = p.cx - pose.x;
            if (dx < 0) xIzq = dx; else xDer = dx;
        }
    }
    CHECK(ojos == 2);

    // LATERALES: uno a cada lado del eje, no juntos al frente. Es una presa,
    // no un depredador.
    CHECK(xIzq < 0.0f);
    CHECK(xDer > 0.0f);
}

TEST_CASE("Cuerpo: los ojos NO brillan en la oscuridad") {
    // MEDIDO: el fondo de ojo del pecari es ATAPETAL — no tiene tapetum
    // lucidum. Es el error tipico al dibujar fauna nocturna, y aqui seria
    // biologicamente falso.
    // Fuente: Veterinary Ophthalmology (2018), PMID 29336116.
    CHECK(Ojo::TIENE_TAPETUM == false);

    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // NINGUNA pieza del animal puede tener emision propia.
    for (const PiezaCuerpo& p : cuerpo) {
        CHECK(p.emision == doctest::Approx(0.0f));
    }
}

TEST_CASE("Cuerpo: los ojos parpadean") {
    PosePecari abierto = PoseBase();
    abierto.parpadeo = 0.0f;

    PosePecari cerrado = PoseBase();
    cerrado.parpadeo = 1.0f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(abierto, a);
    ConstruirCuerpoPecari(cerrado, b);

    // Con el ojo cerrado hay MENOS piezas: desaparecen pupila y brillo.
    CHECK(b.size() < a.size());

    // Y con el ojo medio cerrado, la apertura es intermedia.
    PosePecari medio = PoseBase();
    medio.parpadeo = 0.5f;
    std::vector<PiezaCuerpo> c;
    ConstruirCuerpoPecari(medio, c);

    // Buscar la altura del globo ocular en cada caso.
    auto alturaOjo = [](const std::vector<PiezaCuerpo>& v) -> float {
        for (const PiezaCuerpo& p : v) {
            if (std::fabs(p.r - Tono::OJO_R) < 0.001f &&
                std::fabs(p.g - Tono::OJO_G) < 0.001f) return p.hy;
        }
        return -1.0f;
    };

    const float hAbierto = alturaOjo(a);
    const float hMedio   = alturaOjo(c);
    REQUIRE(hAbierto > 0.0f);
    REQUIRE(hMedio > 0.0f);
    CHECK(hMedio < hAbierto);
}

TEST_CASE("Cuerpo: la pupila es HORIZONTAL") {
    // Va con la franja visual horizontal MEDIDA en su retina y con ser una
    // presa de terreno abierto: ve mejor a lo ancho del horizonte.
    CHECK(Ojo::PUPILA_ANCHO_PX > Ojo::PUPILA_ALTO_PX);
    CHECK(Ojo::TIENE_FRANJA_VISUAL == true);
}

TEST_CASE("Cuerpo: los datos de vision son los MEDIDOS") {
    // Costa et al. 2020, PLoS One, n=6 retinas de 3 machos.
    CHECK(Ojo::CELULAS_GANGLIONARES_TOTAL > 1000000.0f);
    CHECK(Ojo::DENSIDAD_PICO_CG_POR_MM2 == doctest::Approx(6767.0f));
    CHECK(Ojo::AREA_RETINA_MM2 == doctest::Approx(837.8f));

    // Dominan los conos y la vision es dicromatica: es un ojo DIURNO.
    // Confirmacion fisiologica independiente del patron de actividad.
    CHECK(Ojo::DOMINAN_CONOS == true);
    CHECK(Ojo::TIPOS_DE_CONO == 2);
}

// ============================================================================
// 4. LAS ARTICULACIONES
// ============================================================================

TEST_CASE("Cuerpo: cada pata tiene TRES articulaciones") {
    PosePecari pose = PoseBase();
    pose.rapidez = 1.1f;
    pose.fasePaso = 1.0f;
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Cada pata son 3 segmentos con color de pata: superior (humero/femur),
    // medio (radioulna/tibia) y cana (metapodios fusionados).
    //
    // El cuello tambien usa un tono proximo, asi que se cuentan solo las
    // piezas que estan POR DEBAJO del centro del torso.
    int segmentos = 0;
    for (const PiezaCuerpo& p : cuerpo) {
        if (std::fabs(p.r - Tono::PATA_R) < 0.001f &&
            std::fabs(p.g - Tono::PATA_G) < 0.001f &&
            p.cy < cuerpo[0].cy) ++segmentos;
    }
    // 4 patas x 3 segmentos = 12
    CHECK(segmentos == 12);
}

TEST_CASE("Cuerpo: las articulaciones se doblan al andar") {
    PosePecari quieto = PoseBase();
    quieto.rapidez = 0.0f;
    quieto.fasePaso = 0.0f;

    PosePecari andando = PoseBase();
    andando.rapidez = 1.1f;
    andando.fasePaso = 1.2f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(quieto, a);
    ConstruirCuerpoPecari(andando, b);
    REQUIRE(a.size() == b.size());

    // Las patas deben estar en posiciones distintas.
    int movidas = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::fabs(a[i].r - Tono::PATA_R) < 0.001f) {
            const float dx = a[i].cx - b[i].cx;
            const float dz = a[i].cz - b[i].cz;
            const float dy = a[i].cy - b[i].cy;
            if (std::sqrt(dx*dx + dy*dy + dz*dz) > 0.001f) ++movidas;
        }
    }
    CHECK(movidas > 0);
}

TEST_CASE("Cuerpo: parado, las patas NO oscilan") {
    // La amplitud va ligada a la rapidez: es lo que impide el patinaje.
    // Dos fases distintas con el animal parado deben dar la MISMA pose.
    PosePecari p1 = PoseBase();
    p1.rapidez = 0.0f; p1.fasePaso = 0.0f;

    PosePecari p2 = PoseBase();
    p2.rapidez = 0.0f; p2.fasePaso = 2.5f;   // otra fase

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(p1, a);
    ConstruirCuerpoPecari(p2, b);
    REQUIRE(a.size() == b.size());

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::fabs(a[i].r - Tono::PATA_R) < 0.001f) {
            CHECK(a[i].cx == doctest::Approx(b[i].cx).epsilon(0.001));
            CHECK(a[i].cy == doctest::Approx(b[i].cy).epsilon(0.001));
            CHECK(a[i].cz == doctest::Approx(b[i].cz).epsilon(0.001));
        }
    }
}

TEST_CASE("Cuerpo: las articulaciones son de BISAGRA, no rotulas") {
    // MEDIDO: radio y ulna FUSIONADOS, metapodios 3-4 FUSIONADOS, carpo y
    // tarso SOLDADOS. El antebrazo de este animal es UN hueso: no rota.
    //
    // Modelarlo con rotulas libres contradiria el esqueleto.
    CHECK(Articulacion::SOLO_BISAGRA == true);

    // Y el codo dobla al reves que la rodilla: es lo que hace que un
    // cuadrupedo parezca un cuadrupedo.
    CHECK(Articulacion::SENTIDO_DELANTERA * Articulacion::SENTIDO_TRASERA < 0.0f);
}

TEST_CASE("Cuerpo: dedos 4 delante y 3 detras, LA diferencia con el cerdo") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Se cuentan las pezunas.
    int pezunas = 0;
    for (const PiezaCuerpo& p : cuerpo) {
        if (std::fabs(p.r - Tono::PEZUNA_R) < 0.001f) ++pezunas;
    }

    // Delanteras: 2 que apoyan + 2 laterales = 4 dedos, x2 patas = 8
    // Traseras:   2 que apoyan + 1 lateral  = 3 dedos, x2 patas = 6
    // Total = 14
    CHECK(pezunas == 14);
}

TEST_CASE("Cuerpo: las patas se afinan hacia abajo") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // "Patas largas y delgadas sobre un cuerpo macizo" es una firma visual
    // de la especie. El segmento de arriba debe ser mas grueso que la cana.
    float masGrueso = 0.0f, masFino = 1e9f;
    for (const PiezaCuerpo& p : cuerpo) {
        if (std::fabs(p.r - Tono::PATA_R) < 0.001f) {
            if (p.hx > masGrueso) masGrueso = p.hx;
            if (p.hx < masFino)   masFino = p.hx;
        }
    }
    CHECK(masGrueso > masFino);

    // Y todas las patas son mucho mas finas que el torso.
    CHECK(masGrueso < cuerpo[0].hx * 0.4f);
}

// ============================================================================
// 5. EL PELO
// ============================================================================

TEST_CASE("Cuerpo: tiene pelo") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    int mechones = 0;
    for (const PiezaCuerpo& p : cuerpo) if (p.esPelo) ++mechones;

    // Lomo + coronilla + dos flancos.
    const int esperados = Pelo::MECHONES_LOMO + Pelo::MECHONES_CUELLO +
                          Pelo::MECHONES_FLANCO * 2;
    CHECK(mechones == esperados);
    CHECK(mechones > 0);
}

TEST_CASE("Cuerpo: el pelo tiene patron AGUTI, no color plano") {
    // MEDIDO: cada pelo lleva bandas alternas oscuras y claras, y de lejos el
    // animal se ve "grizzled black and gray" — jaspeado, no gris liso.
    CHECK(Pelo::PATRON_AGUTI == true);

    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Los mechones deben tener tonos DISTINTOS entre si.
    float minR = 1e9f, maxR = -1e9f;
    for (const PiezaCuerpo& p : cuerpo) {
        if (!p.esPelo) continue;
        if (p.r < minR) minR = p.r;
        if (p.r > maxR) maxR = p.r;
    }
    CHECK(maxR > minR);   // hay variacion de tono
}

TEST_CASE("Cuerpo: la melena llega a la CORONILLA") {
    // MEDIDO y casi siempre olvidado: la crin no empieza en el lomo, arranca
    // en la cabeza y va "de la coronilla a la grupa".
    CHECK(Pelo::MELENA_LLEGA_A_LA_CORONILLA == true);

    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Debe haber pelo por delante del centro del torso (o sea, en la cabeza).
    bool peloEnCabeza = false;
    for (const PiezaCuerpo& p : cuerpo) {
        if (p.esPelo && p.cz > pose.z + cuerpo[0].hz * 0.9f) {
            peloEnCabeza = true; break;
        }
    }
    CHECK(peloEnCabeza);
}

TEST_CASE("Cuerpo: el pelo del lomo se eriza") {
    PosePecari calmado = PoseBase();
    calmado.erizado = 0.0f;
    PosePecari alarmado = PoseBase();
    alarmado.erizado = 1.0f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(calmado, a);
    ConstruirCuerpoPecari(alarmado, b);
    REQUIRE(a.size() == b.size());

    // El pelo erizado debe ser mas largo.
    float sumaA = 0.0f, sumaB = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].esPelo) { sumaA += a[i].hy; sumaB += b[i].hy; }
    }
    CHECK(sumaB > sumaA * 1.5f);   // notablemente mas erizado
}

TEST_CASE("Cuerpo: cada individuo tiene su pelaje") {
    // La semilla varia el jaspeado, para que no sean clones.
    PosePecari a = PoseBase(); a.semilla = 111u;
    PosePecari b = PoseBase(); b.semilla = 999u;

    std::vector<PiezaCuerpo> ca, cb;
    ConstruirCuerpoPecari(a, ca);
    ConstruirCuerpoPecari(b, cb);
    REQUIRE(ca.size() == cb.size());

    int distintos = 0;
    for (size_t i = 0; i < ca.size(); ++i) {
        if (ca[i].esPelo && std::fabs(ca[i].r - cb[i].r) > 0.001f) ++distintos;
    }
    CHECK(distintos > 0);
}

// ============================================================================
// 6. LUZ
// ============================================================================

TEST_CASE("Cuerpo: las piezas traen datos para iluminar") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    for (const PiezaCuerpo& p : cuerpo) {
        // Normal unitaria (o al menos no nula): el motor la necesita para
        // sombrear las piezas finas.
        const float len = std::sqrt(p.nx*p.nx + p.ny*p.ny + p.nz*p.nz);
        CHECK(len > 0.5f);

        // Colores en rango valido: si se salen, el motor los recorta y el
        // animal sale quemado o negro.
        CHECK(p.r >= 0.0f); CHECK(p.r <= 1.0f);
        CHECK(p.g >= 0.0f); CHECK(p.g <= 1.0f);
        CHECK(p.b >= 0.0f); CHECK(p.b <= 1.0f);

        // Sin emision: ninguna parte de este animal produce luz propia.
        CHECK(p.emision == doctest::Approx(0.0f));
    }
}

// ============================================================================
// COHERENCIA GENERAL
// ============================================================================

TEST_CASE("Cuerpo: el animal esta SOBRE su posicion, no atravesado") {
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    // Nada por debajo del suelo (pose.y es el nivel de las pezunas) ni
    // absurdamente alto.
    for (const PiezaCuerpo& p : cuerpo) {
        CHECK(p.cy - p.hy > pose.y - 0.15f);
        CHECK(p.cy + p.hy < pose.y + 1.6f);
    }
}

TEST_CASE("Cuerpo: rota entero con la orientacion") {
    PosePecari haciaZ = PoseBase();
    haciaZ.orientacionCuerpo = 0.0f;
    PosePecari haciaX = PoseBase();
    haciaX.orientacionCuerpo = 1.5707963f;

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(haciaZ, a);
    ConstruirCuerpoPecari(haciaX, b);
    REQUIRE(a.size() == b.size());

    // El hocico marca hacia donde mira. Se busca por color.
    auto hocico = [](const std::vector<PiezaCuerpo>& v, const PosePecari& p,
                     float& dx, float& dz) {
        for (const PiezaCuerpo& q : v) {
            if (std::fabs(q.r - Tono::HOCICO_R) < 0.001f) {
                dx = q.cx - p.x; dz = q.cz - p.z; return true;
            }
        }
        return false;
    };

    float dxA = 0, dzA = 0, dxB = 0, dzB = 0;
    REQUIRE(hocico(a, haciaZ, dxA, dzA));
    REQUIRE(hocico(b, haciaX, dxB, dzB));

    CHECK(dzA > 0.2f);                 // mirando a +Z
    CHECK(std::fabs(dxA) < 0.1f);
    CHECK(dxB > 0.2f);                 // ahora mirando a +X
    CHECK(std::fabs(dzB) < 0.1f);
}

TEST_CASE("Cuerpo: una cria es mas pequena en todo") {
    PosePecari adulto = PoseBase();
    adulto.escala = 1.0f;
    PosePecari cria = PoseBase();
    cria.escala = 0.30f;   // DERIVADO de 0.5 kg / 18.7 kg

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(adulto, a);
    ConstruirCuerpoPecari(cria, b);
    REQUIRE(a.size() == b.size());

    for (size_t i = 0; i < a.size(); ++i) {
        CHECK(b[i].hx <= a[i].hx + 1e-5f);
        CHECK(b[i].hy <= a[i].hy + 1e-5f);
        CHECK(b[i].hz <= a[i].hz + 1e-5f);
    }
}

TEST_CASE("Cuerpo: no genera un numero absurdo de piezas") {
    // El encargo original pedia "sin abrumar". Un cuerpo con cientos de
    // piezas por animal, multiplicado por miles de agentes, hunde el motor.
    PosePecari pose = PoseBase();
    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    CHECK(cuerpo.size() < 90);
    CHECK(cuerpo.size() > 30);   // pero con detalle suficiente
}

TEST_CASE("Cuerpo: todas las dimensiones son finitas y positivas") {
    PosePecari pose = PoseBase();
    pose.rapidez = 3.2f;
    pose.fasePaso = 2.0f;
    pose.giroCabezaY = 0.8f;
    pose.giroCabezaX = -0.4f;
    pose.erizado = 0.7f;
    pose.parpadeo = 0.3f;

    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(pose, cuerpo);

    for (const PiezaCuerpo& p : cuerpo) {
        CHECK(std::isfinite(p.cx)); CHECK(std::isfinite(p.cy)); CHECK(std::isfinite(p.cz));
        CHECK(p.hx > 0.0f); CHECK(p.hy > 0.0f); CHECK(p.hz > 0.0f);
    }
}

// ============================================================================
// 7. EL TORSO GIRA - REGRESION DE UN BUG REAL
// ============================================================================

TEST_CASE("Cuerpo: el TORSO gira con el animal, no solo se mueve de sitio") {
    // EL BUG QUE ESTE TEST VIGILA:
    //
    // Cada pieza se dibujaba como AABB, alineada a los ejes del mundo. Se
    // midieron hx/hy/hz del torso a 0, 30, 60 y 90 grados y salieron
    // IDENTICOS: el animal cambiaba de sitio pero su cuerpo miraba siempre al
    // norte. Al caminar en diagonal se veia atravesado.
    //
    // El arreglo fue dar a cada pieza su propia rotacion.
    PosePecari recto = PoseBase();
    recto.orientacionCuerpo = 0.0f;

    PosePecari girado = PoseBase();
    girado.orientacionCuerpo = 1.5707963f;   // 90 grados

    std::vector<PiezaCuerpo> a, b;
    ConstruirCuerpoPecari(recto, a);
    ConstruirCuerpoPecari(girado, b);

    CHECK(a[0].giroY == doctest::Approx(0.0f));
    CHECK(b[0].giroY == doctest::Approx(1.5707963f));
    CHECK(std::fabs(b[0].giroY - a[0].giroY) > 1.0f);
}

TEST_CASE("Cuerpo: TODAS las piezas heredan la orientacion del animal") {
    PosePecari girado = PoseBase();
    girado.orientacionCuerpo = 0.8f;

    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(girado, cuerpo);

    // Ninguna pieza puede quedarse sin rotar: si una se queda a 0, esa parte
    // del cuerpo mirara al norte mientras el resto gira.
    for (const PiezaCuerpo& p : cuerpo) {
        CHECK(std::fabs(p.giroY) > 0.1f);
    }
}

TEST_CASE("Cuerpo: la cabeza acumula su giro SOBRE el del cuerpo") {
    PosePecari p = PoseBase();
    p.orientacionCuerpo = 0.5f;
    p.giroCabezaY = 0.6f;

    std::vector<PiezaCuerpo> cuerpo;
    ConstruirCuerpoPecari(p, cuerpo);

    CHECK(cuerpo[0].giroY == doctest::Approx(0.5f));

    bool hayCabezaGirada = false;
    for (const PiezaCuerpo& q : cuerpo) {
        if (q.giroY > 0.6f) { hayCabezaGirada = true; break; }
    }
    CHECK(hayCabezaGirada);
}

// ============================================================================
// 8. PIES PEGADOS AL SUELO
// ============================================================================

TEST_CASE("Cuerpo: las pezunas TOCAN el suelo, sin hueco ni hundimiento") {
    // Se pidio que no hubiera espacio entre los pies y el terreno.
    //
    // Hubo un bug real aqui: la altura del hombro se calculaba mal y la
    // cadena de la pata salia 25.6 px de mas, hundiendo las pezunas 12.7 cm.
    for (int etapa = 0; etapa <= 4; ++etapa) {
        PosePecari pose = PoseBase();
        pose.etapa = etapa;
        pose.rapidez = 0.0f;      // parado: sin rebote

        std::vector<PiezaCuerpo> cuerpo;
        ConstruirCuerpoPecari(pose, cuerpo);

        float masBajo = 1e9f;
        for (const PiezaCuerpo& q : cuerpo) {
            const float lo = q.cy - q.hy;
            if (lo < masBajo) masBajo = lo;
        }

        CHECK(masBajo > pose.y - 0.03f);
        CHECK(masBajo < pose.y + 0.05f);
    }
}

TEST_CASE("Cuerpo: al andar las pezunas no se hunden en el suelo") {
    // El rebote vertical solo va HACIA ARRIBA, precisamente para esto.
    for (int paso = 0; paso < 16; ++paso) {
        PosePecari pose = PoseBase();
        pose.rapidez = 1.1f;
        pose.fasePaso = (float)paso * 0.4f;

        std::vector<PiezaCuerpo> cuerpo;
        ConstruirCuerpoPecari(pose, cuerpo);

        for (const PiezaCuerpo& q : cuerpo) {
            CHECK(q.cy - q.hy > pose.y - 0.06f);
        }
    }
}

// ============================================================================
// 9. BEBES, ADOLESCENTES Y ADULTOS
// ============================================================================

TEST_CASE("Cuerpo: las tres edades son claramente distintas") {
    PosePecari bebe = PoseBase();       bebe.etapa = 0;        // NEONATO
    PosePecari adol = PoseBase();       adol.etapa = 2;        // SUBADULTO
    PosePecari adulto = PoseBase();     adulto.etapa = 3;      // ADULTO

    std::vector<PiezaCuerpo> cb, ca, cad;
    ConstruirCuerpoPecari(bebe, cb);
    ConstruirCuerpoPecari(adol, ca);
    ConstruirCuerpoPecari(adulto, cad);

    CHECK(cb[0].hz < ca[0].hz);
    CHECK(ca[0].hz < cad[0].hz);
}

TEST_CASE("Cuerpo: un bebe NO es un adulto encogido") {
    // Es alometria real: el craneo crece antes que el resto del cuerpo, asi
    // que una cria tiene la cabeza proporcionalmente ENORME.
    const ProporcionEdad pb = ProporcionesDe(0);
    const ProporcionEdad pa = ProporcionesDe(3);

    CHECK(pb.factorCabeza > pa.factorCabeza);   // cabezon
    CHECK(pb.factorPatas  > pa.factorPatas);    // patas larguiruchas
    CHECK(pb.factorTorso  < pa.factorTorso);    // cuerpo poco desarrollado
    CHECK(pb.factorHocico < pa.factorHocico);   // hocico corto, cara chata

    const float razonBebe   = pb.factorCabeza / pb.factorTorso;
    const float razonAdulto = pa.factorCabeza / pa.factorTorso;
    CHECK(razonBebe > razonAdulto * 1.2f);
}

TEST_CASE("Cuerpo: las etapas siguen los hitos MEDIDOS") {
    // La escala del neonato es DERIVADA: 0.5 kg al nacer / 18.7 kg adulto.
    // La masa va con el cubo de la longitud -> (0.5/18.7)^(1/3) = 0.30
    CHECK(ProporcionesDe(0).escalaGeneral == doctest::Approx(0.30f));

    CHECK(ProporcionesDe(0).escalaGeneral < ProporcionesDe(1).escalaGeneral);
    CHECK(ProporcionesDe(1).escalaGeneral < ProporcionesDe(2).escalaGeneral);
    CHECK(ProporcionesDe(2).escalaGeneral < ProporcionesDe(3).escalaGeneral);
}

// ============================================================================
// 10. MOVIMIENTO CORPORAL
// ============================================================================

TEST_CASE("Cuerpo: un pecari PARADO sigue respirando") {
    // Un animal quieto que no se mueve NADA se lee como una estatua.
    PosePecari a = PoseBase();
    a.rapidez = 0.0f; a.fasePaso = 0.0f; a.tiempoVivo = 0.0f;

    PosePecari b = PoseBase();
    b.rapidez = 0.0f; b.fasePaso = 0.0f; b.tiempoVivo = 1.2f;

    std::vector<PiezaCuerpo> ca, cb;
    ConstruirCuerpoPecari(a, ca);
    ConstruirCuerpoPecari(b, cb);

    CHECK(ca[0].hx != doctest::Approx(cb[0].hx));
}

TEST_CASE("Cuerpo: el balanceo crece con la velocidad") {
    PosePecari quieto = PoseBase();
    quieto.rapidez = 0.0f; quieto.fasePaso = 1.0f;

    PosePecari corriendo = PoseBase();
    corriendo.rapidez = 3.2f; corriendo.fasePaso = 1.0f;

    std::vector<PiezaCuerpo> cq, cc;
    ConstruirCuerpoPecari(quieto, cq);
    ConstruirCuerpoPecari(corriendo, cc);

    const bool distinto = (std::fabs(cq[0].cy - cc[0].cy) > 0.001f) ||
                          (std::fabs(cq[0].giroX - cc[0].giroX) > 0.001f);
    CHECK(distinto);
}
