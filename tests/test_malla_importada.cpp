#include <doctest/doctest.h>
#include "player/MallaImportada.h"
#include <cmath>
#include <vector>
#include <cstdio>
#include <string>

// ============================================================================
// EL MODELO IMPORTADO (resourcepacks/Modelos/jugador.vwm) CON EL ESQUELETO
// ============================================================================

using namespace ModeloJugador;

namespace {
std::string rutaModelo() {
#ifdef RAIZ_PROYECTO
    return std::string(RAIZ_PROYECTO) + "/resourcepacks/Modelos/jugador.vwm";
#else
    return "resourcepacks/Modelos/jugador.vwm";
#endif
}
} // namespace

TEST_CASE("Modelo importado: se carga y es ligero para la HD 4000") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    CHECK(mi.formato == 2);
    CHECK(mi.base.size() < 60000);
    CHECK(mi.indices.size() / 3 <= 80000);
    CHECK(mi.uv.size() == mi.base.size() * 2);
    CHECK(mi.indicesSinCabeza.size() < mi.indices.size());
}

TEST_CASE("Modelo importado: en su pose de referencia no se mueve nada") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Malla m;
    mi.deformar(resolver(mi.poseReferencia(), true), m);
    float peor = 0.0f;
    for (size_t i = 0; i < m.v.size(); ++i) peor = std::fmax(peor, largo(resta(m.v[i].p, mi.base[i].p)));
    CHECK(peor < 1e-4f);
}

TEST_CASE("Modelo importado: con los brazos abajo las manos van con el esqueleto") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Pose p;
    p.prop = mi.proporciones();
    limitar(p);
    const Esqueleto s = resolver(p, true);
    Malla m;
    mi.deformar(s, m);
    // Ningun vertice se queda en la cruz: todo queda cerca del cuerpo.
    float ancho = 0.0f, alto = 0.0f;
    for (const auto& v : m.v) { ancho = std::fmax(ancho, std::fabs(v.p.x)); alto = std::fmax(alto, v.p.y); }
    CHECK(ancho < 0.40f);
    CHECK(alto < 1.85f);
    // Y la mano de la malla esta donde la del esqueleto (en la cruz estaba a
    // mas de medio metro; la mano del modelo no mide justo lo mismo que la
    // del esqueleto, de ahi los centimetros de margen).
    float cerca = 1e9f;
    for (const auto& v : m.v) cerca = std::fmin(cerca, largo(resta(v.p, s.mano[1].dedo[1][2].p)));
    CHECK(cerca < 0.07f);
}

TEST_CASE("Modelo importado: el avatar lo moldea y volver atras lo deja igual") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const std::vector<VerticeJ> original = mi.base;
    const V nariz = mi.pt.nariz;
    AjustesMalla a;
    a.narizLargo = 1.25f;
    a.orejas = 1.2f;
    a.busto = 0.8f;
    mi.aplicarAjustes(a);
    // La nariz crece hacia delante; lo de lejos (los pies) no se mueve.
    float zNariz = -1e9f;
    for (const auto& v : mi.base) if (largo(resta(v.p, nariz)) < 0.03f) zNariz = std::fmax(zNariz, v.p.z);
    CHECK(zNariz > nariz.z + 0.002f);
    for (size_t i = 0; i < original.size(); ++i)
        if (original[i].p.y < 0.3f) REQUIRE(largo(resta(mi.base[i].p, original[i].p)) < 1e-6f);
    // Al volver a los valores por defecto queda como vino.
    mi.aplicarAjustes(AjustesMalla{});
    float peor = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) peor = std::fmax(peor, largo(resta(mi.base[i].p, original[i].p)));
    CHECK(peor < 1e-6f);
}

TEST_CASE("Modelo importado: unos brazos mas gruesos engordan el brazo, no la mano") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Pose p;
    p.prop = mi.proporciones();
    limitar(p);
    const Esqueleto s = resolver(p, true);
    auto anchoBrazo = [&](float k) {
        AjustesMalla a;
        a.brazo = k;
        mi.aplicarAjustes(a);
        Malla m;
        mi.deformar(s, m);
        // La distancia media al eje del brazo derecho de lo que es brazo.
        float suma = 0.0f;
        int n = 0;
        for (size_t i = 0; i < m.v.size(); ++i)
            if (mi.h0[i] == huesoDe(1, HL_HUMERO) && mi.w0[i] > 0.9f) {
                suma += detalle::distSegmento(m.v[i].p, s.hombro[1].p, s.codo[1].p);
                ++n;
            }
        return n ? suma / n : 0.0f;
    };
    CHECK(anchoBrazo(1.3f) > anchoBrazo(1.0f) * 1.2f);
}

TEST_CASE("Modelo importado: lo que lleva en la mano va en la mano de la malla, no en el antebrazo") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const Esqueleto s = resolver(mi.poseReferencia(), true);
    const V muneca = s.mano[1].muneca.p, punta = s.mano[1].dedo[1][3].p;
    const float L = largo(resta(punta, muneca));
    const V dir = escala(resta(punta, muneca), 1.0f / L);
    const V g = mi.agarre(s).p;
    const V q = resta(g, muneca);
    const float a = q.x * dir.x + q.y * dir.y + q.z * dir.z;       // cuanto se aleja de la muneca
    // En el tramo medio de la mano: pasado el arranque y antes de las puntas.
    CHECK(a > 0.30f * L);
    CHECK(a < 0.70f * L);
    // Mas lejos de la muneca que el agarre de una mano humana, o el objeto
    // se quedaba en el bloque del antebrazo (el bug).
    const V viejo = agarre(s).p;
    CHECK(a > largo(resta(viejo, muneca)) + 0.02f);
    // Y hay malla alrededor: esta DENTRO de la mano, no en el aire.
    int cerca = 0;
    for (const auto& v : mi.base) if (largo(resta(v.p, g)) < 0.06f) ++cerca;
    CHECK(cerca > 20);
    // Sigue al antebrazo, que en este modelo es una pieza con la mano: con el
    // brazo en otra postura, a la misma distancia del codo.
    Pose p;
    p.prop = mi.proporciones();
    limitar(p);
    const Esqueleto s2 = resolver(p, true);
    CHECK(largo(resta(mi.agarre(s2).p, s2.codo[1].p)) ==
          doctest::Approx(largo(resta(g, s.codo[1].p))).epsilon(0.005));
}

TEST_CASE("Modelo importado: en primera persona no queda nada de la cabeza; desde fuera, entera") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const float yCabeza = mi.yDeLaCabeza();       // la cabeza Y el cuello
    float yMax = 0.0f;
    for (const auto& v : mi.base) yMax = std::fmax(yMax, v.p.y);
    REQUIRE(yMax > yCabeza + 0.10f);                  // el modelo tiene cabeza

    // Sin cabeza: ningun triangulo llega por encima de donde empieza.
    const auto& sin = mi.indicesDe(false);
    float ySin = 0.0f;
    for (uint32_t i : sin) ySin = std::fmax(ySin, mi.base[i].p.y);
    CHECK(ySin <= yCabeza + 1e-4f);
    // ...y no se ha llevado el cuerpo: hombros, brazos y piernas siguen.
    CHECK(sin.size() > mi.indices.size() / 2);
    int deHombrosAbajo = 0, deHombrosAbajoSin = 0;
    for (uint32_t i : mi.indices) if (mi.base[i].p.y < yCabeza - 0.05f) ++deHombrosAbajo;
    for (uint32_t i : sin) if (mi.base[i].p.y < yCabeza - 0.05f) ++deHombrosAbajoSin;
    CHECK(deHombrosAbajoSin > deHombrosAbajo * 0.97f);

    // Con cabeza (lo que veria otro jugador): la malla entera, hasta arriba.
    const auto& con = mi.indicesDe(true);
    CHECK(con.size() == mi.indices.size());
    float yCon = 0.0f;
    for (uint32_t i : con) yCon = std::fmax(yCon, mi.base[i].p.y);
    CHECK(yCon == doctest::Approx(yMax));
}

// ----------------------------------------------------------------------------
// EL MODELO DE BLOQUES: PIEZAS RIGIDAS
// ----------------------------------------------------------------------------
namespace {
// Cuanto se estira la arista que mas se estira al poner la malla en `p`.
float peorEstiramiento(const MallaImportada& mi, Pose p) {
    p.prop = mi.proporciones();
    limitar(p);
    Malla m;
    mi.deformar(resolver(p, true), m);
    float peor = 0.0f;
    for (size_t k = 0; k + 2 < mi.indices.size(); k += 3)
        for (int e = 0; e < 3; ++e) {
            const uint32_t a = mi.indices[k + e], b = mi.indices[k + (e + 1) % 3];
            const float l0 = largo(resta(mi.base[a].p, mi.base[b].p));
            if (l0 < 0.004f) continue;
            peor = std::fmax(peor, largo(resta(m.v[a].p, m.v[b].p)) / l0);
        }
    return peor;
}
} // namespace

TEST_CASE("Modelo importado: es de bloques y cada bloque va con un solo hueso") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    std::vector<int> pieza;
    const int piezas = mi.piezasDeLaMalla(pieza);
    CHECK(piezas > 100);                       // el modelo actual tiene 324
    CHECK(mi.esDeBloques());
    CHECK(mi.piezasRigidas > piezas / 2);
    // Los brazos y las piernas siguen teniendo piezas de mas de un hueso: se doblan.
    std::vector<bool> usado(NUM_HUESOS, false);
    for (size_t i = 0; i < mi.h0.size(); ++i) usado[mi.h0[i]] = true;
    for (int l = 0; l < 2; ++l) {
        CHECK(usado[huesoDe(l, HL_HUMERO)]);
        CHECK(usado[huesoDe(l, HL_RADIO)]);
        CHECK(usado[huesoDe(l, HL_FEMUR)]);
        CHECK(usado[huesoDe(l, HL_TIBIA)]);
    }
}

TEST_CASE("Modelo importado: al moverse ningun bloque se estira") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    // Antes de atar por piezas, andando habia 2.900 aristas a mas del triple
    // de su largo, y alguna a mas de cien veces.
    Pose reposo;
    CHECK(peorEstiramiento(mi, reposo) < 1.05f);
    Pose andar;
    andar.brazo[0].flex = 0.6f; andar.brazo[1].flex = -0.6f; andar.brazo[0].codo = andar.brazo[1].codo = 0.6f;
    andar.pierna[0].flex = 0.5f; andar.pierna[1].flex = -0.3f; andar.pierna[0].rodilla = 0.6f; andar.pierna[1].rodilla = 0.2f;
    CHECK(peorEstiramiento(mi, andar) < 1.05f);
    Pose golpe;
    golpe.brazo[1].flex = 0.9f; golpe.brazo[1].codo = 1.2f;
    for (int d = 0; d < 5; ++d) for (int f = 0; f < 3; ++f) golpe.brazo[1].dedo[d][f] = 1.2f;
    CHECK(peorEstiramiento(mi, golpe) < 1.05f);
    Pose agachado;
    agachado.pierna[0].flex = agachado.pierna[1].flex = 1.3f;
    agachado.pierna[0].rodilla = agachado.pierna[1].rodilla = 1.8f;
    CHECK(peorEstiramiento(mi, agachado) < 1.05f);
}

TEST_CASE("Modelo importado: el esqueleto llega hasta la mano de la malla") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const Esqueleto s = resolver(mi.poseReferencia(), true);
    // La punta de los dedos del esqueleto, donde acaba la mano del modelo: lo
    // mas bajo del brazo derecho (por fuera de x = 0.30 no hay piernas).
    V punta = { 0, 9, 0 };
    for (const auto& v : mi.base) if (v.p.x > 0.30f && v.p.y > 0.55f && v.p.y < punta.y) punta = v.p;
    CHECK(largo(resta(s.mano[1].dedo[1][3].p, punta)) < 0.12f);
    // Y el antebrazo ya no es el de 19 cm que dejaba la muneca a medio camino.
    CHECK(largo(resta(s.mano[1].muneca.p, s.codo[1].p)) > 0.28f);
}

TEST_CASE("Modelo importado: los miembros se separan, y las manos se siguen juntando al golpear") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Pose p;
    const float brazo = p.brazo[0].abd, pierna = p.pierna[0].abd;
    mi.separarMiembros(p);
    for (int l = 0; l < 2; ++l) {
        CHECK(p.brazo[l].abd == doctest::Approx(brazo + MallaImportada::ABRIR_BRAZOS));
        CHECK(p.pierna[l].abd == doctest::Approx(pierna + MallaImportada::ABRIR_PIERNAS));
    }
    // Con el brazo levantado (golpe, hacha a dos manos) no se abre.
    Pose g;
    g.brazo[1].flex = 1.2f;
    const float antes = g.brazo[1].abd;
    mi.separarMiembros(g);
    CHECK(g.brazo[1].abd == doctest::Approx(antes));
    CHECK(g.brazo[0].abd > antes);             // el otro, que cuelga, si
}

TEST_CASE("Modelo importado: corriendo, agachado y tumbado los brazos se abren mucho mas") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    auto manos = [&](Pose p, float corre) {
        Pose q = p;
        mi.separarMiembros(q, corre);
        limitar(q);
        limitar(p);
        const Esqueleto a = resolver(p, true), b = resolver(q, true);
        const float antes = std::fabs(a.mano[0].muneca.p.x - a.mano[1].muneca.p.x);
        const float ahora = std::fabs(b.mano[0].muneca.p.x - b.mano[1].muneca.p.x);
        MESSAGE("entre las manos: " << antes << " -> " << ahora);
        return ahora - antes;
    };
    // Corriendo: los codos doblados, los brazos adelante y atras.
    Pose corre;
    corre.brazo[0].flex = 0.6f; corre.brazo[1].flex = -0.6f;
    corre.brazo[0].codo = corre.brazo[1].codo = 1.4f;
    CHECK(manos(corre, 1.0f) > 0.15f);
    CHECK(manos(corre, 0.0f) < manos(corre, 1.0f) - 0.10f);      // andando, no tanto
    // Agachado.
    Pose ag = corre;
    ag.agachado = 1.0f;
    CHECK(manos(ag, 0.0f) > 0.15f);
    // Tumbado: los brazos por delante de la cabeza (antes no se abrian nada).
    Pose tu;
    tu.tumbado = 1.0f;
    for (Brazo& b : tu.brazo) { b.flex = 2.3f; b.codo = 0.6f; b.abd = 0.35f; }
    CHECK(manos(tu, 0.0f) > 0.15f);
    // La mano que golpea no se abre.
    Pose g = ag;
    const float abd = g.brazo[1].abd;
    mi.separarMiembros(g, 0.0f, false);
    CHECK(g.brazo[1].abd <= abd + MallaImportada::ABRIR_BRAZOS + 1e-4f);
    CHECK(g.brazo[0].abd > abd + 0.3f);
}

TEST_CASE("Modelo importado: recto y robotico, sin vaivenes de cadera, tronco ni munecas") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Estado e;
    e.enSuelo = true;
    e.rapidez = 4.3f;
    for (int k = 0; k < 16; ++k) {
        e.fasePaso = 0.4f * k;
        e.tiempo = 0.1f * k;
        Pose p = poseDe(e);
        const Pose andar = p;
        mi.hacerRobotico(p, 0.3f, false);
        CHECK(p.pelvisLado == 0.0f);
        CHECK(p.giroPelvis == 0.0f);
        CHECK(p.respira == 0.0f);
        CHECK(p.giroTorso == doctest::Approx(0.3f * 0.35f));     // solo acompana a la cabeza
        for (int l = 0; l < 2; ++l) {
            CHECK(p.brazo[l].munFlex == 0.0f);
            CHECK(p.brazo[l].rot == 0.0f);
            // El paso y el braceo siguen ahi.
            CHECK(p.pierna[l].flex == doctest::Approx(andar.pierna[l].flex));
            CHECK(p.brazo[l].flex == doctest::Approx(andar.brazo[l].flex));
        }
    }
    // Con una accion, el brazo derecho conserva su gesto.
    Pose g;
    g.brazo[1].munDesv = -0.3f;
    g.giroTorso = 0.2f;
    mi.hacerRobotico(g, 0.0f, true);
    CHECK(g.brazo[1].munDesv == doctest::Approx(-0.3f));
    CHECK(g.giroTorso == doctest::Approx(0.2f));
}

TEST_CASE("Modelo importado: agachado y tumbado tampoco queda cabeza ni cuello delante de la camara") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    // Lo que se quita no depende de la postura (es la misma lista de
    // triangulos de pie, agachado y tumbado), y llega por debajo del menton.
    CHECK(mi.yDeLaCabeza() < mi.pt.menton.y - 0.03f);
    const auto& sin = mi.indicesDe(false);
    const float R = std::fmax(0.22f, mi.pt.cabezaRadio * 2.0f);
    int enLaColumna = 0;
    for (uint32_t i : sin) {
        const V p = mi.base[i].p;
        const float dx = p.x - mi.pt.cabeza.x, dz = p.z - mi.pt.cabeza.z;
        if (p.y > mi.yDeLaCabeza() + 1e-4f && dx * dx + dz * dz < R * R) ++enLaColumna;
    }
    CHECK(enLaColumna == 0);
    // Los hombros no se han ido con el cuello.
    int hombros = 0;
    for (uint32_t i : sin) if (std::fabs(mi.base[i].p.x) > 0.25f && mi.base[i].p.y > 1.25f) ++hombros;
    CHECK(hombros > 100);
}

TEST_CASE("Modelo importado: la mano es una pieza con el antebrazo y no se separa de el") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    REQUIRE(mi.esDeBloques());
    // Ningun vertice va ya con un hueso de la mano (carpos, metacarpos, dedos).
    int deLaMano = 0;
    for (size_t i = 0; i < mi.h0.size(); ++i) {
        const int b = mi.h0[i];
        if (b < H_POR_LADO_INICIO) continue;
        const int k = (b - H_POR_LADO_INICIO) % HL_NUM;
        if (k >= HL_CARPO0 && k < HL_COXAL) ++deLaMano;
    }
    CHECK(deLaMano == 0);

    // Un vertice de lo mas bajo de la PALMA y uno del antebrazo, en la referencia.
    // (Era la punta de la pinza; los dedos tienen ahora sus huesos y se doblan
    // -- ver los tests de los dedos --. Lo que no puede separarse del
    // antebrazo es la mano: la palma, el dorso y el pulgar.)
    const Esqueleto s0 = resolver(mi.poseReferencia(), true);
    const V codo = s0.codo[1].p, muneca = s0.mano[1].muneca.p;
    size_t punta = 0, ante = 0;
    float yMin = 9.0f, cerca = 9.0f;
    const V medio = mezclar(codo, muneca, 0.5f);
    for (size_t i = 0; i < mi.base.size(); ++i) {
        const V p = mi.base[i].p;
        if (!mi.dedoDe[i] && p.x > 0.30f && p.y > 0.55f && p.y < yMin) { yMin = p.y; punta = i; }
        if (largo(resta(p, medio)) < cerca) { cerca = largo(resta(p, medio)); ante = i; }
    }
    const float d0 = largo(resta(mi.base[punta].p, mi.base[ante].p));
    REQUIRE(d0 > 0.20f);

    // Corriendo, saltando, agachado y tirando: con la muneca y el antebrazo
    // girados de todas las maneras, la mano sigue a la misma distancia.
    for (int caso = 0; caso < 6; ++caso) {
        Pose p;
        p.prop = mi.proporciones();
        Brazo& b = p.brazo[1];
        b.flex = -0.7f + 0.5f * caso;
        b.codo = 0.2f + 0.3f * caso;
        b.prono = -0.4f + 0.35f * caso;          // el giro del antebrazo
        b.munFlex = -0.6f + 0.25f * caso;        // y la muneca, de un extremo al otro
        b.munDesv = 0.4f - 0.16f * caso;
        b.rot = -0.5f + 0.2f * caso;
        for (int d = 0; d < 5; ++d) for (int f = 0; f < 3; ++f) b.dedo[d][f] = 0.25f * caso;
        limitar(p);
        const Esqueleto s = resolver(p, true);
        Malla m;
        mi.deformar(s, m);
        CAPTURE(caso);
        CHECK(largo(resta(m.v[punta].p, m.v[ante].p)) == doctest::Approx(d0).epsilon(0.002));
    }
}

TEST_CASE("Modelo importado: el objeto va en la palma, entre los dedos, en cualquier postura") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const Esqueleto s0 = resolver(mi.poseReferencia(), true);
    const V g0 = mi.agarre(s0).p;
    // En la referencia: malla a los lados del agarre (esta ENTRE piezas de la
    // mano, no pegado a una cara) y mas alla de el hacia la punta (los dedos).
    const V haciaLaPunta = normalizar(resta(s0.mano[1].dedo[1][3].p, s0.mano[1].muneca.p));
    int alrededor = 0, masAlla = 0;
    std::vector<size_t> vecinos;
    for (size_t i = 0; i < mi.base.size(); ++i) {
        const V q = resta(mi.base[i].p, g0);
        if (largo(q) > 0.09f) continue;
        ++alrededor;
        vecinos.push_back(i);
        if (q.x * haciaLaPunta.x + q.y * haciaLaPunta.y + q.z * haciaLaPunta.z > 0.02f) ++masAlla;
    }
    CHECK(alrededor > 60);
    CHECK(masAlla > 15);

    // En otras posturas el agarre no se mueve respecto a esa malla: cada
    // vertice de la mano sigue a la misma distancia de el.
    for (int caso = 0; caso < 5; ++caso) {
        Pose p;
        p.prop = mi.proporciones();
        Brazo& b = p.brazo[1];
        b.flex = -0.6f + 0.5f * caso; b.codo = 0.3f + 0.35f * caso;
        b.prono = -0.3f + 0.4f * caso; b.munFlex = 0.5f - 0.25f * caso; b.munDesv = -0.3f + 0.15f * caso;
        limitar(p);
        const Esqueleto s = resolver(p, true);
        Malla m;
        mi.deformar(s, m);
        const V g = mi.agarre(s).p;
        float peor = 0.0f;
        for (size_t i : vecinos)
            peor = std::fmax(peor, std::fabs(largo(resta(m.v[i].p, g)) - largo(resta(mi.base[i].p, g0))));
        CAPTURE(caso);
        CHECK(peor < 0.003f);
    }
}

TEST_CASE("Modelo importado: la camara va en el medio de la cabeza, no delante de la cara") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    const Esqueleto s = resolver(mi.poseReferencia(), true);
    const V c = mi.ojos(s);
    // De delante atras: a la misma distancia de la cara que de la nuca.
    float zMin = 1e9f, zMax = -1e9f;
    for (const VerticeJ& v : mi.base)
        if (v.p.y > s.cuello.p.y + 0.06f) { zMin = std::fmin(zMin, v.p.z); zMax = std::fmax(zMax, v.p.z); }
    MESSAGE("cabeza de z " << zMin << " a " << zMax << ", camara en z " << c.z << ", retrocede " << mi.camaraAtras()
            << ", cuello z " << mi.pt.cuelloBase.z << " y " << mi.pt.cuelloBase.y << ", cabeza z " << mi.pt.cabeza.z
            << ", nariz z " << mi.pt.nariz.z << ", camara y " << c.y);
    {
        // El cuello de la MALLA: lo que hay justo por debajo del corte de la cabeza.
        float a = 1e9f, b = -1e9f;
        for (const VerticeJ& v : mi.base)
            if (std::fabs(v.p.y - mi.yDeLaCabeza()) < 0.03f && std::fabs(v.p.x) < 0.12f) { a = std::fmin(a, v.p.z); b = std::fmax(b, v.p.z); }
        MESSAGE("malla a la altura del corte: z " << a << " a " << b << " (y " << mi.yDeLaCabeza() << ")");
    }
    // Arriba del cuello, un poco por detras de su eje, y dentro de la cabeza.
    CHECK(c.z == doctest::Approx(mi.zDeLaCamara()).epsilon(0.02));
    CHECK(c.z <= 0.5f * (zMin + zMax));
    CHECK(c.z < mi.pt.cuelloBase.z - 0.02f);
    CHECK(c.z > mi.pt.cuelloBase.z - 0.08f);
    CHECK(c.z > zMin + 0.05f);
    CHECK(c.z < mi.pt.nariz.z - 0.04f);          // por detras de la nariz, donde iba
    CHECK(mi.camaraAtras() > 0.04f);
    // A la altura de los ojos y centrada.
    CHECK(c.y > mi.pt.ojo[0].y);
    CHECK(c.y < mi.pt.ojo[0].y + 0.08f);
    CHECK(std::fabs(c.x - mi.pt.cabeza.x) < 0.02f);
    // Y dentro de lo que no se dibuja en primera persona.
    CHECK(c.y > mi.yDeLaCabeza());
}

// ⭐ LOS HUESOS DE LOS DEDOS. PEDIDO: "dale huesos de animacion y que las
// animaciones de las manos tengan a los dedos de las manos".
namespace {
// La mano derecha con los dedos doblados `cuanto` radianes mas que en la
// referencia (negativo: abiertos). Solo cambian los dedos.
Pose conDedos(const MallaImportada& mi, float cuanto) {
    Pose p = mi.poseReferencia();
    for (int l = 0; l < 2; ++l)
        for (int d = 0; d < 4; ++d) {
            p.brazo[l].dedo[d][0] += cuanto;
            p.brazo[l].dedo[d][1] += cuanto;
        }
    return p;
}
} // namespace

TEST_CASE("Modelo importado: cada mano tiene huesos de dedo, en dos filas") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    REQUIRE(mi.esDeBloques());
    int porMano[2] = {}, fila[2][3] = {};
    for (int e = 0; e < mi.numDedos; ++e) {
        const MallaImportada::HuesoDedo& d = mi.dedos[e];
        ++porMano[d.lado];
        ++fila[d.lado][d.nivel];
        if (d.nivel == 2) {
            REQUIRE(d.padre >= 0);                       // cuelga de un bloque de la primera fila
            CHECK(mi.dedos[d.padre].nivel == 1);
            CHECK(mi.dedos[d.padre].lado == d.lado);
        }
    }
    CHECK(mi.numDedos <= MallaImportada::MAX_DEDOS);
    for (int l = 0; l < 2; ++l) {
        CHECK(porMano[l] >= 4);
        CHECK(fila[l][1] >= 2);
        CHECK(fila[l][2] >= 2);
    }
    // Solo son de dedo vertices de la mano: su hueso de siempre es el antebrazo.
    size_t deDedo = 0;
    for (size_t i = 0; i < mi.dedoDe.size(); ++i) {
        if (!mi.dedoDe[i]) continue;
        ++deDedo;
        const int l = mi.dedos[mi.dedoDe[i] - 1].lado;
        CHECK(mi.h0[i] == huesoDe(l, HL_RADIO));
        CHECK(mi.huesoDeDibujo(i) >= NUM_HUESOS);
    }
    CHECK(deDedo > 500);
    CHECK(deDedo < mi.modelo.size() / 8);                // los dedos, no medio cuerpo
}

TEST_CASE("Modelo importado: al cerrar la mano los dedos van hacia la palma y la palma no se mueve") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Malla quieta, cerrada, abierta;
    const Esqueleto s0 = resolver(mi.poseReferencia(), true);
    mi.deformar(s0, quieta);
    mi.deformar(resolver(conDedos(mi, 1.2f), true), cerrada);
    mi.deformar(resolver(conDedos(mi, -0.4f), true), abierta);
    for (int l = 0; l < 2; ++l) {
        const Rot rt = detalle::traspuesta(s0.mano[l].muneca.r);
        float haciaPalma = 0.0f, haciaDorso = 0.0f, palma = 0.0f, mayor = 0.0f;
        for (size_t i = 0; i < mi.modelo.size(); ++i) {
            const V c = rt.aplicar(resta(cerrada.v[i].p, quieta.v[i].p));
            const V a = rt.aplicar(resta(abierta.v[i].p, quieta.v[i].p));
            if (mi.dedoDe[i] && mi.dedos[mi.dedoDe[i] - 1].lado == l) {
                haciaPalma = std::fmax(haciaPalma, c.z);
                haciaDorso = std::fmax(haciaDorso, -a.z);
                mayor = std::fmax(mayor, largo(c));
            } else if (mi.h0[i] == huesoDe(l, HL_RADIO)) {
                palma = std::fmax(palma, std::fmax(largo(c), largo(a)));
            }
        }
        CHECK(haciaPalma > 0.02f);          // la punta se mete mas de 2 cm hacia la palma
        CHECK(haciaDorso > 0.005f);         // y al abrir va hacia el dorso
        CHECK(mayor < 0.15f);               // con tope: no atraviesa la mano
        CHECK(palma < 1e-4f);               // la palma, el pulgar y el antebrazo, quietos
    }
}

TEST_CASE("Modelo importado: los bloques de los dedos no se deforman ni se despegan") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    Malla quieta, cerrada;
    const Esqueleto s1 = resolver(conDedos(mi, 1.2f), true);
    mi.deformar(resolver(mi.poseReferencia(), true), quieta);
    mi.deformar(s1, cerrada);
    // Rigidos: cada arista de un bloque de dedo mide lo mismo.
    float peor = 0.0f;
    for (size_t t = 0; t + 2 < mi.indices.size(); t += 3) {
        for (int k = 0; k < 3; ++k) {
            const uint32_t a = mi.indices[t + k], b = mi.indices[t + (k + 1) % 3];
            if (!mi.dedoDe[a]) continue;
            const float antes = largo(resta(quieta.v[a].p, quieta.v[b].p));
            const float ahora = largo(resta(cerrada.v[a].p, cerrada.v[b].p));
            peor = std::fmax(peor, std::fabs(ahora - antes));
        }
    }
    CHECK(peor < 1e-4f);
    // Pegados: junto a su bisagra un bloque casi no se mueve (respecto a lo
    // que tiene encima); se mueve la punta.
    for (int e = 0; e < mi.numDedos; ++e) {
        const MallaImportada::HuesoDedo& d = mi.dedos[e];
        if (d.nivel != 1) continue;
        float junto = 1e9f, lejos = 0.0f;
        for (size_t i = 0; i < mi.modelo.size(); ++i) {
            if (mi.dedoDe[i] != e + 1) continue;
            const V q = resta(mi.modelo[i].p, d.nudillo);
            // La distancia a la LINEA de la bisagra (el eje va a lo ancho).
            const float alEje = largo(resta(q, escala(d.eje, q.x * d.eje.x + q.y * d.eje.y + q.z * d.eje.z)));
            const float mueve = largo(resta(cerrada.v[i].p, quieta.v[i].p));
            if (alEje < 0.02f) junto = std::fmin(junto, mueve);
            lejos = std::fmax(lejos, mueve);
        }
        CHECK(junto < 0.016f);
        CHECK(lejos > junto + 0.01f);
    }
}

TEST_CASE("Modelo importado: los dedos se doblan con tope y cada bloque sigue al dedo que tiene encima") {
    MallaImportada mi;
    REQUIRE(mi.cargar(rutaModelo()));
    REQUIRE(mi.numDedos > 0);
    const Esqueleto ref = resolver(mi.poseReferencia(), true);
    const Esqueleto puno = resolver(conDedos(mi, 3.0f), true);
    const Esqueleto poco = resolver(conDedos(mi, 0.3f), true);
    for (int e = 0; e < mi.numDedos; ++e) {
        float a1, a2;
        mi.giroDeDedo(ref, e, a1, a2);
        CHECK(a1 == doctest::Approx(0.0f).epsilon(0.001));    // en la referencia, nada
        CHECK(a2 == doctest::Approx(0.0f).epsilon(0.001));
        mi.giroDeDedo(poco, e, a1, a2);
        CHECK(a1 > 0.10f);
        CHECK(a1 < 0.30f);                                    // solo una parte de lo que dice la pose
        mi.giroDeDedo(puno, e, a1, a2);
        CHECK(a1 <= MallaImportada::DEDO_CIERRA_MAX + 1e-4f);
        CHECK(a2 <= MallaImportada::DEDO_CIERRA_MAX + 1e-4f);
        if (mi.dedos[e].nivel == 1) CHECK(a2 == 0.0f);
    }
    // Solo el indice doblado: se cierran los bloques de su lado y no los del menique.
    Pose p = mi.poseReferencia();
    p.brazo[1].dedo[0][0] += 0.8f;
    const Esqueleto indice = resolver(p, true);
    float mas = 0.0f, menos = 1e9f;
    for (int e = 0; e < mi.numDedos; ++e) {
        if (mi.dedos[e].lado != 1 || mi.dedos[e].nivel != 1) continue;
        float a1, a2;
        mi.giroDeDedo(indice, e, a1, a2);
        mas = std::fmax(mas, a1);
        menos = std::fmin(menos, a1);
    }
    CHECK(mas > 0.2f);
    CHECK(menos < 0.05f);
}
