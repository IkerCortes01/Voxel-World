#include <doctest/doctest.h>
#include "render/Atmosfera.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>

using namespace Atmosfera;

namespace {
std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}
} // namespace

TEST_CASE("Atmosfera: de dia el horizonte es mas claro que el cenit") {
    const Cielo c = cieloPara(1.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.horizonte[i] > c.cenit[i]);
    CHECK(c.resplandor[0] == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: de noche es oscuro y las estrellas se ven") {
    const Cielo c = cieloPara(0.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.horizonte[i] < 0.15f);
    CHECK(brilloEstrellas(0.0f) == doctest::Approx(1.0f));
    CHECK(brilloEstrellas(1.0f) == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: el ocaso enrojece el horizonte, mas del lado del sol") {
    const Cielo c = cieloPara(0.5f);
    CHECK(c.horizonte[0] > c.horizonte[2]);
    float alSol[3], contra[3];
    colorEn(c, 0.05f, 1.0f, alSol);
    colorEn(c, 0.05f, -1.0f, contra);
    CHECK(alSol[0] > contra[0]);
}

TEST_CASE("Atmosfera: el horizonte coincide con la niebla y el color no se sale") {
    // La niebla ya no es de un solo color: es la del cielo que hay detras, en
    // cada direccion. Lo que importa es que coincidan donde se tocan -- el
    // horizonte --, mire uno hacia donde mire.
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.05f) {
        const Cielo c = cieloPara(luz);
        for (float haciaSol = -1.0f; haciaSol <= 1.0f; haciaSol += 0.25f) {
            float h[3], n[3];
            colorEn(c, 0.0f, haciaSol, h);
            colorNiebla(c, haciaSol, n);
            for (int i = 0; i < 3; ++i) CHECK(n[i] == doctest::Approx(h[i]));
        }
        // De lado al sol no hay brillo de ninguno de los dos: es el horizonte.
        float lado[3];
        colorNiebla(c, 0.0f, lado);
        for (int i = 0; i < 3; ++i) {
            CHECK(lado[i] == doctest::Approx(c.horizonte[i]));
            float v[3];
            colorEn(c, 0.3f, 1.0f, v);
            CHECK(v[i] >= 0.0f);
            CHECK(v[i] <= 1.0f);
        }
    }
}

TEST_CASE("Atmosfera: a mediodia la luz del sol es blanca") {
    // El terreno se ve con sus colores de siempre: el tinte es neutro.
    const Cielo c = cieloPara(1.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.luzSol[i] == doctest::Approx(1.0f));
    for (int i = 0; i < 3; ++i) CHECK(c.contraluz[i] == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: el ocaso dora la luz y la noche la enfria") {
    const Cielo ocaso = cieloPara(0.5f);
    CHECK(ocaso.luzSol[0] > ocaso.luzSol[2]);          // calida
    const Cielo noche = cieloPara(0.0f);
    CHECK(noche.luzSol[2] > noche.luzSol[0]);          // azulada
}

TEST_CASE("Atmosfera: el tinte cambia el color de la luz, no cuanta hay") {
    // La noche ya la oscurece LuzCielo.h. Si el tinte bajara la luminancia,
    // la noche saldria oscurecida dos veces.
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.1f) {
        const Cielo c = cieloPara(luz);
        const float y = 0.299f * c.luzSol[0] + 0.587f * c.luzSol[1] +
                        0.114f * c.luzSol[2];
        CHECK(y > 0.88f);
        CHECK(y < 1.05f);
    }
}

TEST_CASE("Atmosfera: de espaldas al sol poniente el cielo bajo se tine de rosa") {
    // El cinturon de Venus: del lado contrario al sol, abajo, sube el rojo y
    // el azul a la vez. A mediodia no existe.
    const Cielo c = cieloPara(0.5f);
    float contra[3], lado[3];
    colorEn(c, 0.05f, -1.0f, contra);
    colorEn(c, 0.05f, 0.0f, lado);
    CHECK(contra[0] > lado[0]);
    CHECK(contra[2] > lado[2]);
    // Y en el cenit no se nota.
    float arribaContra[3], arribaLado[3];
    colorEn(c, 1.0f, -1.0f, arribaContra);
    colorEn(c, 1.0f, 0.0f, arribaLado);
    for (int i = 0; i < 3; ++i)
        CHECK(arribaContra[i] == doctest::Approx(arribaLado[i]));
}

TEST_CASE("Atmosfera: el shader repite la cuenta de la niebla") {
    // Misma formula en dos lenguajes: si una cambia y la otra no, el terreno
    // lejano deja de fundirse con el cielo. Se vigila el texto del shader.
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + "src/render/TerrenoGL.h", std::ios::binary);
    REQUIRE(f.good());
    const std::string src((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    CHECK(src.find("uResplandor * (s * s * s)") != std::string::npos);
    CHECK(src.find("uContraluz * (e * e)") != std::string::npos);
}

TEST_CASE("Atmosfera: las estrellas son unitarias y siempre las mismas") {
    for (int i = 0; i < 320; ++i) {
        float a[4], b[4];
        estrella(i, a);
        estrella(i, b);
        CHECK(a[0] == b[0]);
        CHECK(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] == doctest::Approx(1.0f));
        CHECK(a[3] > 0.0f);
    }
}

TEST_CASE("Atmosfera: el crepusculo pasa por etapas y nunca da un salto") {
    // La hora azul: el sol ya se fue, arriba azul profundo y nada de naranja.
    const Cielo azul = cieloPara(0.22f);
    CHECK(azul.cenit[2] > azul.cenit[0] * 3.0f);
    CHECK(azul.horizonte[0] < 0.4f);
    // La hora dorada: el horizonte calido y claro, el cenit todavia azul.
    const Cielo dorada = cieloPara(0.78f);
    CHECK(dorada.horizonte[0] > dorada.horizonte[2]);
    CHECK(dorada.cenit[2] > dorada.cenit[0] * 2.0f);
    // Tras la puesta la brasa es mas roja que antes de ella.
    const Cielo antes = cieloPara(0.65f), despues = cieloPara(0.30f);
    CHECK(despues.resplandor[1] / despues.resplandor[0] <
          antes.resplandor[1] / antes.resplandor[0]);
    // Continuo: un paso pequeno de luz mueve poco el color.
    for (float luz = 0.0f; luz < 0.995f; luz += 0.01f) {
        const Cielo a = cieloPara(luz), b = cieloPara(luz + 0.01f);
        for (int i = 0; i < 3; ++i) {
            CHECK(std::fabs(a.cenit[i] - b.cenit[i]) < 0.03f);
            CHECK(std::fabs(a.horizonte[i] - b.horizonte[i]) < 0.03f);
        }
    }
}

TEST_CASE("Atmosfera: el arco amarillo esta sobre el horizonte, del lado del sol") {
    const Cielo c = cieloPara(0.5f);
    float alSol[3], lado[3];
    colorEn(c, 0.20f, 1.0f, alSol);
    colorEn(c, 0.20f, 0.0f, lado);
    CHECK(alSol[0] > lado[0]);
    CHECK(alSol[1] > lado[1]);
    // A mediodia no existe.
    for (int i = 0; i < 3; ++i) CHECK(cieloPara(1.0f).arco[i] == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: el catalogo tiene muchas tenues, pocas brillantes y la Via Lactea") {
    float centro[3], lado[3], polo[3];
    baseGalactica(centro, lado, polo);
    int brillantes = 0, tenues = 0, enFranja = 0;
    for (int i = 0; i < N_CATALOGO; ++i) {
        const Estrella e = estrellaCatalogo(i);
        const Estrella otra = estrellaCatalogo(i);
        CHECK(e.d[0] == otra.d[0]);                         // siempre la misma
        CHECK(e.d[0] * e.d[0] + e.d[1] * e.d[1] + e.d[2] * e.d[2] == doctest::Approx(1.0f));
        CHECK(e.brillo >= 0.18f);
        CHECK(e.brillo <= 1.0f);
        if (e.brillo > 0.7f) ++brillantes;
        if (e.brillo < 0.36f) ++tenues;
        const float b = e.d[0] * polo[0] + e.d[1] * polo[1] + e.d[2] * polo[2];
        if (std::fabs(b) < 0.17f) ++enFranja;               // a menos de 10 grados del plano
    }
    CHECK(brillantes > 5);
    CHECK(brillantes < N_CATALOGO / 15);
    CHECK(tenues > N_CATALOGO / 2);
    // Al azar caerian en esa franja el 17 %: aqui, bastante mas del doble.
    CHECK(enFranja > N_CATALOGO * 2 / 5);
    // Mas densa hacia el centro de la galaxia que en el lado contrario.
    CHECK(viaLactea(0.0f) > viaLactea(3.14159f) * 1.8f);
}

TEST_CASE("Atmosfera: las estrellas giran alrededor del polo celeste, como el sol") {
    const float lat = 23.0f * 3.14159265f / 180.0f;
    const float cl = std::cos(lat), sl = std::sin(lat);
    // El polo norte celeste no se mueve: al norte (-z), a 23 grados de alto.
    const float polo[3] = { 0.0f, 0.0f, 1.0f };
    for (float ts = 0.0f; ts < 6.3f; ts += 0.7f) {
        float w[3];
        ecuatorialAMundo(polo, std::cos(ts), std::sin(ts), cl, sl, w);
        CHECK(w[0] == doctest::Approx(0.0f));
        CHECK(w[1] == doctest::Approx(sl));
        CHECK(w[2] == doctest::Approx(-cl));
    }
    // Una estrella del ecuador celeste: en su punto mas alto mira al sur, antes
    // esta al este y despues al oeste; y sigue siendo unitaria.
    const float e[3] = { 1.0f, 0.0f, 0.0f };
    float antes[3], arriba[3], despues[3];
    ecuatorialAMundo(e, std::cos(-1.0f), std::sin(-1.0f), cl, sl, antes);
    ecuatorialAMundo(e, 1.0f, 0.0f, cl, sl, arriba);
    ecuatorialAMundo(e, std::cos(1.0f), std::sin(1.0f), cl, sl, despues);
    CHECK(antes[0] > 0.5f);
    CHECK(despues[0] < -0.5f);
    CHECK(arriba[1] > antes[1]);
    CHECK(arriba[2] > 0.0f);
    CHECK(antes[0] * antes[0] + antes[1] * antes[1] + antes[2] * antes[2] == doctest::Approx(1.0f));
}

TEST_CASE("Atmosfera: el horizonte apaga las estrellas y las tenues salen mas tarde") {
    CHECK(extincion(0.0f) == doctest::Approx(0.0f));
    CHECK(extincion(0.5f) == doctest::Approx(1.0f));
    CHECK(extincion(0.15f) > extincion(0.08f));
    CHECK(visible(0.3f, 1.0f) > 0.5f);        // al anochecer, las brillantes ya estan
    CHECK(visible(0.3f, 0.2f) == doctest::Approx(0.0f));
    CHECK(visible(1.0f, 0.2f) == doctest::Approx(1.0f));
}

TEST_CASE("Atmosfera: las estrellas fugaces son raras, breves y van por encima del horizonte") {
    int pasos = 0, con = 0;
    for (double t = 0.0; t < 3600.0; t += 0.05) {
        ++pasos;
        const Meteoro m = meteoro(t);
        if (!m.hay) continue;
        ++con;
        CHECK(m.cabeza[1] > 0.05f);
        CHECK(m.brillo >= 0.0f);
        CHECK(m.brillo <= 1.0f);
        CHECK(m.cabeza[0] * m.cabeza[0] + m.cabeza[1] * m.cabeza[1] + m.cabeza[2] * m.cabeza[2] ==
              doctest::Approx(1.0f));
    }
    CHECK(con > 0);
    CHECK(con < pasos / 20);                  // menos del 5 % del tiempo
}

TEST_CASE("Atmosfera: el cielo no se parte en franjas") {
    // PEDIDO: "que no este partido en colores el cielo". Subiendo del horizonte
    // al cenit, ni el color ni su pendiente dan saltos: una esquina en la
    // curva se ve como una raya. Se mira a todas las horas y hacia el sol, de
    // lado y de espaldas. (Pegado al horizonte la pendiente es grande a
    // proposito; se mide desde 2 grados.)
    const float PASO = 0.005f;
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.125f) {
        const Cielo c = cieloPara(luz);
        for (float haciaSol = -1.0f; haciaSol <= 1.0f; haciaSol += 0.5f) {
            for (float a = 0.035f; a + PASO <= 1.0f; a += PASO) {
                float abajo[3], aqui[3], arriba[3];
                colorEn(c, a - PASO, haciaSol, abajo);
                colorEn(c, a, haciaSol, aqui);
                colorEn(c, a + PASO, haciaSol, arriba);
                for (int i = 0; i < 3; ++i) {
                    CHECK(std::fabs(arriba[i] - aqui[i]) < 0.02f);                 // sin escalon
                    if (aqui[i] > 0.999f || abajo[i] > 0.999f || arriba[i] > 0.999f) continue;
                    CHECK(std::fabs(arriba[i] - 2.0f * aqui[i] + abajo[i]) < 0.004f);  // sin esquina
                }
            }
        }
    }
}

TEST_CASE("Sonido: romper bloques ya no suena") {
    // PEDIDO: "sobre todo elimina el sonido de romper bloques".
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("playBreakBlock") == std::string::npos);
    CHECK(src.find("playBreakingLoop") == std::string::npos);
    CHECK(src.find("playPlaceBlock") != std::string::npos);   // colocar sigue
}
