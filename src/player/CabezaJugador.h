#pragma once

#include <cmath>
#include <cstdint>
#include "Avatar.h"
#include "MallaJugador.h"
#include "MallaImportada.h"

// ============================================================================
// LA CABEZA, EL CUELLO, EL PELO Y LA BARBA (y como el avatar viste el cuerpo)
// ============================================================================
// En primera persona la cabeza no se dibuja: la camara esta dentro. Donde si
// se ve es en la pantalla de AVATAR, que muestra al personaje entero y lo
// deja girar. Aqui se construye, con las mismas herramientas que el cuerpo
// (MallaJugador.h): elipsoides y tubos con su seccion.
//
//   CRANEO Y CARA  craneo, cara, mandibula, menton, pomulos y frente, cada uno
//                  con su ajuste.
//   OJOS           esclerotica, iris del color de los genes (HERC2) y pupila;
//                  parpado de arriba y cejas del color del pelo.
//   NARIZ          puente, punta y aletas.
//   BOCA           labio de arriba y de abajo.
//   OREJAS         despegadas o pegadas.
//   PELO           un casquete que sigue la linea del nacimiento del pelo
//                  (mas alta en la frente, baja por la nuca). Rizado, se
//                  abulta en ondas; con calvicie se abre por la coronilla y
//                  las entradas. Pasados 8 cm cae una melena por detras, que
//                  crece con los dias.
//   BARBA          desde la sombra de unos milimetros a una barba poblada.
//
// `aplicarAvatar` pasa el fenotipo y los ajustes a la Piel del cuerpo: el
// color de piel de los genes, la ropa y la Forma (grosores, mano, pie...).
// ============================================================================

namespace ModeloJugador {

inline Color colorDe(Avatar::Color3 c) { return { c.r, c.g, c.b }; }

// El fenotipo y los ajustes, al cuerpo.
inline void aplicarAvatar(const Avatar::Datos& d, Piel& piel) {
    using namespace Avatar;
    const Fenotipo f = d.fenotipo();
    piel.piel = colorDe(f.piel);
    piel.camisa = colorDe(f.camisa);
    piel.pantalon = colorDe(f.shortC);
    Forma& F = piel.forma;
    // El aspecto (0 femenino .. 1 masculino) mueve las proporciones tipicas:
    // caderas mas anchas y hombros mas estrechos hacia lo femenino.
    const float fem = 1.0f - d[CUERPO_GENERO];
    const float grasa = d[CUERPO_GRASA] - 0.25f;
    F.hombros = d[HOMBRO_ANCHO] * (1.0f - 0.07f * fem);
    F.deltoides = d[HOMBRO_DELTOIDES];
    F.brazo = d[BRAZO_GROSOR] * (1.0f + 0.25f * grasa) * (1.0f - 0.06f * fem);
    F.biceps = d[BRAZO_BICEPS];
    F.antebrazo = d[BRAZO_ANTEBRAZO];
    F.mano = d[MANO_TAMANO] * (1.0f - 0.06f * fem);
    F.dedos = d[MANO_DEDOS_GROSOR];
    F.unaMm = f.una_mm;
    F.pecho = d[TORSO_PECHO] * (1.0f - 0.05f * fem);
    F.busto = d[TORSO_BUSTO];
    F.cintura = d[TORSO_CINTURA] * (1.0f + 0.30f * grasa) * (1.0f - 0.06f * fem);
    F.abdomen = (d[TORSO_ABDOMEN] - 0.1f) + 0.8f * grasa;
    F.cadera = d[CADERA_ANCHO] * (1.0f + 0.09f * fem + 0.15f * grasa);
    F.gluteos = d[CADERA_GLUTEOS] * (1.0f + 0.15f * fem);
    F.muslo = d[PIERNA_MUSLO] * (1.0f + 0.25f * grasa + 0.04f * fem);
    F.gemelo = d[PIERNA_GEMELO];
    F.pie = d[PIE_TAMANO] * (1.0f - 0.05f * fem);
    F.pieAncho = d[PIE_ANCHO];
    F.arco = d[PIE_ARCO];
    F.musculo = f.musculo;
}

// ⭐ El avatar sobre el modelo importado (ver MallaImportada.h): las mismas
// formas que el cuerpo procedural (con el genero y la grasa ya mezclados en
// aplicarAvatar), la cara por zonas y los colores de los genes y la ropa.
inline void aplicarAvatarMalla(const Avatar::Datos& d, AjustesMalla& a) {
    using namespace Avatar;
    Piel piel;
    aplicarAvatar(d, piel);
    const Forma& F = piel.forma;
    const Fenotipo f = d.fenotipo();
    a.cabeza = d[CAB_TAMANO];
    a.cuello = d[CUELLO_GROSOR];
    a.hombros = F.hombros * F.deltoides * 0.5f + 0.5f;
    a.brazo = F.brazo * (0.85f + 0.15f * F.musculo);
    a.biceps = F.biceps * F.musculo;
    a.antebrazo = F.antebrazo * F.brazo;
    a.mano = F.mano;
    a.pecho = F.pecho;
    a.cintura = F.cintura;
    a.cadera = F.cadera;
    a.muslo = F.muslo * (1.0f + (F.musculo - 1.0f) * 0.3f);
    a.gemelo = F.gemelo * F.musculo;
    a.pie = F.pie;
    a.caraAncho = d[CAB_ANCHO];
    a.caraLargo = d[CAB_LARGO];
    a.frente = d[CAB_FRENTE];
    a.pomulos = d[CAB_POMULOS];
    a.mandibula = d[CAB_MANDIBULA];
    a.menton = d[CAB_MENTON];
    a.ojos = d[OJO_TAMANO];
    a.ojosSeparacion = d[OJO_SEPARACION];
    a.ojosAltura = d[OJO_ALTURA];
    a.cejasAltura = d[CEJA_ALTURA];
    a.narizLargo = d[NARIZ_LARGO];
    a.narizAncho = d[NARIZ_ANCHO];
    a.narizPuente = d[NARIZ_PUENTE];
    a.bocaAncho = d[BOCA_ANCHO];
    a.labios = d[LABIOS_GROSOR];
    a.orejas = d[OREJA_TAMANO];
    a.orejasDespegadas = d[OREJA_DESPEGADA] - 0.2f;
    a.cuelloLargo = d[CUELLO_LARGO];
    a.busto = F.busto;
    a.abdomen = F.abdomen;
    a.gluteos = F.gluteos;
    a.craneo = d[CAB_CRANEO]; a.papada = d[CAB_PAPADA]; a.sienes = d[CAB_SIENES];
    a.ojosProfundidad = d[OJO_PROFUNDIDAD]; a.ojosInclinacion = d[OJO_INCLINACION];
    a.cejasArco = d[CEJA_ARCO]; a.cejasGrosor = d[CEJA_GROSOR];
    a.narizPunta = d[NARIZ_PUNTA]; a.narizAletas = d[NARIZ_ALETAS]; a.narizAltura = d[NARIZ_ALTURA];
    a.bocaAltura = d[BOCA_ALTURA]; a.sonrisa = d[BOCA_SONRISA]; a.bocaSaliente = d[BOCA_SALIENTE];
    a.orejasAltura = d[OREJA_ALTURA]; a.orejasPunta = d[OREJA_PUNTA]; a.lobulos = d[OREJA_LOBULO];
    a.peloVolumen = d[PELO_VOLUMEN] * (0.9f + 0.1f * d[PELO_DENSIDAD]);
    a.nuez = 0.3f + (d[CUELLO_NUEZ] * (1.0f - 0.7f * (1.0f - d[CUERPO_GENERO])) - 0.3f);
    a.trapecio = d[HOMBRO_TRAPECIO] * (0.9f + 0.1f * F.musculo);
    a.triceps = d[BRAZO_TRICEPS];
    a.dedosLargo = d[MANO_DEDOS_LARGO];
    a.espalda = d[TORSO_ESPALDA];
    a.pectoral = d[TORSO_PECTORAL] * F.musculo;
    a.cuadriceps = d[PIERNA_CUADRICEPS];
    a.pieDedos = d[PIE_DEDOS_LARGO];
    a.cinturaAltura = d[CADERA_ALTURA];    // Los colores: sin tocar nada el modelo se ve tal cual vino. Cuanto mas
    // se aleja el avatar de su color por defecto (genes, canas, ropa), mas
    // toma cada zona el color pedido, con el sombreado de la textura.
    static const Fenotipo f0 = Avatar::Datos{}.fenotipo();
    auto zona = [](float* o, float& mezcla, Avatar::Color3 c, Avatar::Color3 c0) {
        o[0] = c.r; o[1] = c.g; o[2] = c.b;
        const float d = std::fmax(std::fabs(c.r - c0.r), std::fmax(std::fabs(c.g - c0.g), std::fabs(c.b - c0.b)));
        mezcla = d * 8.0f > 1.0f ? 1.0f : d * 8.0f;
    };
    zona(a.piel, a.mezcla[0], f.piel, f0.piel);
    zona(a.pelo, a.mezcla[1], f.pelo, f0.pelo);
    zona(a.camisa, a.mezcla[2], f.camisa, f0.camisa);
    zona(a.shortC, a.mezcla[3], f.shortC, f0.shortC);
}

// ⭐ Los largos de los huesos y la flexibilidad, a la pose. La flexibilidad
// del deslizador (0..1) va de rigido a muy flexible, y con los anos se
// pierde: ~0,5 % de rango por ano desde los 30. Las mujeres tienen de media
// algo mas de rango articular.
inline void aplicarProporciones(const Avatar::Datos& d, Pose& p) {
    using namespace Avatar;
    p.prop.torso = d[LARGO_TORSO];
    p.prop.brazo = d[LARGO_BRAZO];
    p.prop.antebrazo = d[LARGO_ANTEBRAZO];
    p.prop.muslo = d[LARGO_MUSLO];
    p.prop.pierna = d[LARGO_PIERNA];
    const float edad = d[CUERPO_EDAD];
    float k = 0.80f + 0.40f * d[FLEXIBILIDAD];
    if (edad > 30.0f) k *= 1.0f - 0.005f * (edad - 30.0f);
    k *= 1.0f + 0.05f * (1.0f - d[CUERPO_GENERO]);
    p.flexibilidad = sujetar(k, 0.6f, 1.3f);
    p.prop.hombroY *= d[LARGO_CUELLO_HUESO];
    p.prop.mano *= d[LARGO_MANO];
    // ⭐ LA POSTURA DE CADA UNO: hombros, codos, manos, espalda, pelvis,
    // rodillas y pies. Se suman a la pose del juego en cada frame.
    for (int l = 0; l < 2; ++l) {
        Brazo& b = p.brazo[l];
        b.encoger += 0.5f * std::fmax(0.0f, d[HOMBRO_ALTURA]);
        b.adelantar += 0.6f * d[HOMBRO_ADELANTE] - 0.3f * std::fmax(0.0f, -d[HOMBRO_ALTURA]);
        b.abd += 0.10f * d[BRAZO_CODOS];
        b.rot -= 0.25f * d[BRAZO_CODOS];
        const float puno = d[MANO_PUNO] - 0.15f;
        for (int dd = 0; dd < 4; ++dd)
            for (int ff = 0; ff < 3; ++ff) b.dedo[dd][ff] += puno * (ff == 1 ? 1.3f : 1.0f);
        Pierna& g = p.pierna[l];
        g.rot += 0.25f * d[PIERNA_RODILLAS] + 0.30f * d[PIE_PUNTAS];
        g.abd += 0.07f * d[PIERNA_SEPARACION];
    }
    p.inclinacion += 0.15f * d[TORSO_POSTURA];
    p.cuelloFlex += 0.12f * d[TORSO_POSTURA] + 0.15f * d[CUELLO_ADELANTE];
    p.pelvisInclina += 0.12f * d[CADERA_POSTURA];
}

// Un casquete de elipsoide: solo lo que queda por ENCIMA de la linea del pelo
// (`linea(phi)` = angulo polar maximo en cada direccion) y fuera del claro de
// la calvicie (`claro(phi)` = angulo polar minimo). Con ondas para el rizo.
template <class FLinea, class FClaro>
inline void casquete(Malla& m, V c, const Rot& r, V radio, int lat, int lon, Color col,
                     FLinea linea, FClaro claro, float rizo, float semilla) {
    const uint32_t base = (uint32_t)m.v.size();
    for (int i = 0; i <= lat; ++i) {
        for (int j = 0; j < lon; ++j) {
            const float ph = 2.0f * PI * j / lon;
            const float t0 = claro(ph), t1 = linea(ph);
            const float th = t0 + (t1 - t0) * i / lat;
            const float sy = std::cos(th), sr = std::sin(th);
            // +Z delante: ph = PI/2 es la frente.
            V u = { sr * std::cos(ph), sy, sr * std::sin(ph) };
            const float onda = 1.0f + rizo * 0.07f * std::sin(ph * 9.0f + semilla) * std::sin(th * 11.0f + semilla * 0.7f);
            const V p = { u.x * radio.x * onda, u.y * radio.y * onda, u.z * radio.z * onda };
            const V n = { u.x / radio.x, u.y / radio.y, u.z / radio.z };
            // Las mechas: unas mas claras y otras mas oscuras.
            const float k = 1.0f + 0.08f * std::sin(ph * 23.0f + th * 5.0f + semilla);
            m.v.push_back({ suma(c, r.aplicar(p)), normalizar(r.aplicar(n)), col.r * k, col.g * k, col.b * k });
        }
    }
    for (int i = 0; i < lat; ++i)
        for (int j = 0; j < lon; ++j) {
            const uint32_t a = base + i * lon + j, b = base + i * lon + (j + 1) % lon;
            m.i.insert(m.i.end(), { a, a + lon, b + lon, a, b + lon, b });
        }
}

inline void generarCabeza(const Esqueleto& s, const Avatar::Datos& d, const Calidad& q, Malla& m) {
    using namespace Avatar;
    m.limpiar();
    const Fenotipo f = d.fenotipo();
    const float kRes = q.detalle <= 0 ? 0.5f : (q.detalle == 1 ? 0.75f : 1.0f);
    auto N = [&](int n) { const int r = (int)(n * kRes + 0.5f); return r < 3 ? 3 : r; };

    const Color piel = colorDe(f.piel);
    const Color pelo = colorDe(f.pelo);
    const Color labio = tinte(piel, 0.98f, 0.70f, 0.70f);
    const float fem = 1.0f - d[CUERPO_GENERO];

    // ---------------- EL CUELLO ----------------
    const Rot rC = s.cuello.r;
    const float Lc = 0.060f * d[CUELLO_LARGO];
    const float rCuello = 0.058f * d[CUELLO_GROSOR] * (1.0f - 0.10f * fem) * (0.9f + 0.1f * f.musculo);
    {
        auto eje = [&](float t, V& c, Rot& r) { r = rC; c = suma(s.cuello.p, rC.aplicar({ 0, -0.03f + (Lc + 0.04f) * t, 0.004f })); };
        auto sec = [&](float t, float a, float& dx, float& dz) {
            // Los esternocleidomastoideos marcan los lados; la nuez, delante.
            const float esterno = 0.004f * bulto(difAng(a, DELANTE + 0.9f), 0.5f) + 0.004f * bulto(difAng(a, DELANTE - 0.9f), 0.5f);
            const float nuez = (1.0f - fem) * 0.005f * bulto(t - 0.45f, 0.18f) * bulto(difAng(a, DELANTE), 0.35f);
            elipse(a, rCuello * (1.0f + 0.25f * bulto(t, 0.3f)), rCuello * 0.95f, esterno + nuez, dx, dz);
        };
        dibujarTubo(m, tubo(eje, sec), N(8), N(20), [&](float, float) { return piel; }, false, false);
    }

    // ---------------- EL CRANEO Y LA CARA ----------------
    const float S = 1.08f * d[CAB_TAMANO] * (1.0f - 0.05f * fem);
    const float an = d[CAB_ANCHO], la = d[CAB_LARGO];
    const V centro = suma(s.cuello.p, rC.aplicar({ 0, Lc + 0.095f * S, 0.010f }));
    auto P = [&](float x, float y, float z) { return suma(centro, rC.aplicar({ x * S, y * S, z * S })); };
    const V cranR = { 0.078f * an * S, 0.098f * S, 0.100f * S };
    elipsoide(m, P(0, 0.022f, -0.012f), rC, cranR, N(14), N(22), piel);
    // La frente.
    elipsoide(m, P(0, 0.030f, 0.040f), rC, { 0.066f * an * S, 0.055f * d[CAB_FRENTE] * S, 0.055f * S }, N(8), N(14), piel);
    // La cara y la mandibula.
    elipsoide(m, P(0, -0.030f * la, 0.020f), rC, { 0.068f * an * S, 0.076f * la * S, 0.078f * S }, N(12), N(18), piel);
    const float mand = d[CAB_MANDIBULA] * (1.0f - 0.10f * fem);
    elipsoide(m, P(0, -0.072f * la, 0.022f), rC, { 0.060f * mand * an * S, 0.034f * S, 0.064f * S }, N(8), N(14), piel);
    elipsoide(m, P(0, -0.096f * la, 0.066f), rC, { 0.022f * d[CAB_MENTON] * S, 0.020f * S, 0.020f * d[CAB_MENTON] * S },
              N(6), N(10), piel);
    for (float sx : { -1.0f, 1.0f })
        elipsoide(m, P(sx * 0.048f * an, -0.010f, 0.068f), rC,
                  { 0.022f * d[CAB_POMULOS] * S, 0.016f * S, 0.018f * S }, N(5), N(8), tinte(piel, 1.02f, 0.97f, 0.96f));

    // ---------------- LOS OJOS Y LAS CEJAS ----------------
    const float sep = 0.032f * d[OJO_SEPARACION], yOjo = 0.006f + (d[OJO_ALTURA] - 1.0f) * 0.08f;
    const float rOjo = 0.0125f * d[OJO_TAMANO];
    const Color iris = colorDe(f.iris);
    for (float sx : { -1.0f, 1.0f }) {
        // La cuenca: un hueco algo mas oscuro, y el ojo asomando de ella.
        elipsoide(m, P(sx * sep, yOjo + 0.002f, 0.083f), rC, { rOjo * 1.45f * S, rOjo * 1.15f * S, rOjo * 0.55f * S },
                  N(5), N(10), tinte(piel, 0.86f, 0.80f, 0.80f));
        const V ojo = P(sx * sep, yOjo, 0.086f);
        elipsoide(m, ojo, rC, { rOjo * S, rOjo * 0.85f * S, rOjo * 0.7f * S }, N(6), N(10), Color{ 0.94f, 0.93f, 0.90f });
        elipsoide(m, suma(ojo, rC.aplicar({ 0, 0, rOjo * 0.62f * S })), rC,
                  { rOjo * 0.52f * S, rOjo * 0.52f * S, rOjo * 0.14f * S }, N(4), N(10), iris);
        elipsoide(m, suma(ojo, rC.aplicar({ 0, 0, rOjo * 0.70f * S })), rC,
                  { rOjo * 0.24f * S, rOjo * 0.24f * S, rOjo * 0.10f * S }, N(3), N(8), Color{ 0.03f, 0.03f, 0.03f });
        // El parpado de arriba, de piel, medio cerrado sobre el ojo.
        elipsoide(m, suma(ojo, rC.aplicar({ 0, rOjo * 0.45f * S, rOjo * 0.10f * S })), rC,
                  { rOjo * 1.10f * S, rOjo * 0.55f * S, rOjo * 0.78f * S }, N(4), N(10), tinte(piel, 0.97f, 0.92f, 0.92f));
        // La ceja.
        const float hC = 0.026f * d[CEJA_ALTURA];
        // El arco de la ceja: el hueso que hace sombra sobre el ojo.
        elipsoide(m, P(sx * (sep + 0.002f), yOjo + hC - 0.004f, 0.084f), rC, { 0.021f * S, 0.007f * S, 0.010f * S },
                  N(4), N(8), piel);
        elipsoide(m, P(sx * (sep + 0.002f), yOjo + hC, 0.093f), rC.por(Rot::enZ(-sx * 0.10f)),
                  { 0.019f * S, 0.0035f * d[CEJA_GROSOR] * (1.0f - 0.3f * fem) * S, 0.006f * S }, N(3), N(8),
                  tinte(pelo, 0.9f, 0.9f, 0.9f));
    }

    // ---------------- LA NARIZ ----------------
    const float nL = d[NARIZ_LARGO], nA = d[NARIZ_ANCHO], nP = d[NARIZ_PUENTE];
    elipsoide(m, P(0, -0.006f, 0.094f + 0.004f * nP), rC, { 0.0085f * nA * S, 0.026f * nL * S, 0.012f * nP * S },
              N(6), N(8), piel);
    elipsoide(m, P(0, -0.030f * nL, 0.106f + 0.006f * nP), rC, { 0.013f * nA * S, 0.012f * S, 0.012f * S },
              N(6), N(10), tinte(piel, 1.03f, 0.96f, 0.95f));
    for (float sx : { -1.0f, 1.0f })
        elipsoide(m, P(sx * 0.012f * nA, -0.034f * nL, 0.099f), rC, { 0.009f * nA * S, 0.007f * S, 0.009f * S },
                  N(4), N(8), piel);

    // ---------------- LA BOCA ----------------
    const float bA = d[BOCA_ANCHO], lG = d[LABIOS_GROSOR] * (1.0f + 0.15f * fem);
    elipsoide(m, P(0, -0.056f * la, 0.093f), rC, { 0.022f * bA * S, 0.0045f * lG * S, 0.0085f * S }, N(4), N(10), labio);
    elipsoide(m, P(0, -0.0645f * la, 0.091f), rC, { 0.020f * bA * S, 0.0055f * lG * S, 0.0085f * S }, N(4), N(10), labio);

    // ---------------- LAS OREJAS ----------------
    const float oT = d[OREJA_TAMANO], oD = d[OREJA_DESPEGADA];
    for (float sx : { -1.0f, 1.0f }) {
        const Rot rO = rC.por(Rot::enY(sx * (0.15f + 0.55f * oD)));
        const V oreja = P(sx * (0.077f * an + 0.003f + 0.008f * oD), 0.002f, -0.018f);
        elipsoide(m, oreja, rO, { 0.0055f * S, 0.031f * oT * S, 0.020f * oT * S }, N(6), N(12), tinte(piel, 1.0f, 0.92f, 0.90f));
        // El hueco de la concha, mas oscuro, y el lobulo abajo.
        elipsoide(m, suma(oreja, rO.aplicar({ sx * 0.0035f * S, 0.0f, 0.002f * S })), rO,
                  { 0.0025f * S, 0.017f * oT * S, 0.011f * oT * S }, N(4), N(8), tinte(piel, 0.80f, 0.68f, 0.66f));
        elipsoide(m, suma(oreja, rO.aplicar({ 0.0f, -0.026f * oT * S, 0.003f * S })), rO,
                  { 0.005f * S, 0.008f * S, 0.007f * S }, N(4), N(8), tinte(piel, 1.02f, 0.92f, 0.90f));
    }

    // ---------------- LAS PECAS ----------------
    if (f.pecas > 0.15f) {
        Azar z{ 0x5EEDu + (uint32_t)(f.pecas * 100.0f) };
        const int n = (int)(f.pecas * 26.0f);
        const Color peca = tinte(piel, 0.78f, 0.62f, 0.52f);
        for (int k = 0; k < n; ++k) {
            const float sx = z() < 0.5f ? -1.0f : 1.0f;
            elipsoide(m, P(sx * z.entre(0.018f, 0.055f) * an, z.entre(-0.03f, 0.0f), z.entre(0.090f, 0.095f)), rC,
                      { 0.0018f * S, 0.0018f * S, 0.0012f * S }, 2, 4, peca);
        }
    }

    // ---------------- EL PELO ----------------
    const float largo = f.pelo_cm;
    if (largo > 0.05f) {
        const float grueso = (0.0035f + (std::min)(largo, 10.0f) * 0.0011f) * d[PELO_DENSIDAD] * f.grosorPelo;
        const V radio = { cranR.x + grueso, cranR.y + grueso, cranR.z + grueso };
        // La linea del pelo: alta en la frente (con entradas si hay
        // calvicie), a la altura de las orejas a los lados, baja en la nuca.
        auto linea = [&](float ph) {
            const float frente = std::sin(ph);          // 1 delante, -1 detras
            const float lado = std::fabs(std::cos(ph));
            const float entradas = f.calvicie * 0.35f * lado * (frente > 0 ? frente : 0.0f);
            // En radianes desde la coronilla: ~63 grados en la frente, ~95 a
            // los lados (la oreja) y ~115 en la nuca.
            return 1.55f - 0.45f * frente + 0.10f * lado - entradas;
        };
        // La coronilla que se despeja con la calvicie.
        auto claro = [&](float) { return f.calvicie * 0.75f; };
        casquete(m, P(0, 0.022f, -0.012f), rC, radio, N(10), N(28), pelo, linea, claro, f.rizo, 1.7f);
        // ⭐ LA MELENA: lo que pasa de 8 cm cae por detras, y crece.
        if (largo > 8.0f) {
            const float Lm = (std::min)((largo - 8.0f) / 100.0f, 0.50f);
            const V nuca = P(0, -0.020f, -0.085f);
            auto eje = [&](float t, V& c, Rot& r) { r = rC; c = suma(nuca, rC.aplicar({ 0, -Lm * t, -0.012f * t })); };
            const float ancho = 0.075f * an * S + grueso;
            auto sec = [&](float t, float a, float& dx, float& dz) {
                const float ond = 1.0f + f.rizo * 0.15f * std::sin(a * 7.0f + t * 20.0f);
                elipse(a, ancho * (1.0f - 0.25f * t) * ond, (0.022f + 0.012f * f.rizo) * (1.0f - 0.3f * t) * ond, 0.0f, dx, dz);
            };
            dibujarTubo(m, tubo(eje, sec), N(10), N(18),
                        [&](float t, float a) { const float k = 1.0f + 0.07f * std::sin(a * 13.0f + t * 6.0f);
                                                return tinte(pelo, k, k, k); }, false, true);
        }
    }

    // ---------------- LA BARBA ----------------
    const float barba = f.barba_mm;
    if (barba > 0.5f) {
        const float g = (std::min)(barba, 60.0f) * 0.00045f + 0.0012f;
        const Color cb = tinte(pelo, 0.95f, 0.95f, 0.95f);
        elipsoide(m, P(0, -0.074f * la, 0.026f), rC, { 0.061f * mand * an * S + g, 0.036f * S + g * 1.6f, 0.066f * S + g },
                  N(8), N(14), cb);
        elipsoide(m, P(0, -0.098f * la - g * 0.6f, 0.066f), rC, { 0.024f * S + g, 0.022f * S + g * 1.4f, 0.021f * S + g },
                  N(6), N(10), cb);
        // El bigote, sobre el labio.
        elipsoide(m, P(0, -0.048f * la, 0.090f), rC, { 0.024f * bA * S + g * 0.4f, 0.004f * S + g * 0.3f, 0.009f * S },
                  N(4), N(10), cb);
    }
}

} // namespace ModeloJugador
