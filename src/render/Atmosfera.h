#pragma once

#include <cmath>

// ============================================================================
// LOS COLORES DEL CIELO
// ============================================================================
// Sin OpenGL: el motor los pinta, los tests los comprueban.
//
//   cenit      lo mas alto, mas saturado
//   horizonte  mas claro: la luz cruza mas aire. Es tambien el color de la
//              niebla, y por eso el terreno lejano se funde con el cielo.
//   resplandor el tono del lado del sol cuando esta bajo
//
// `luz` es luzSolar(): 0 de noche, 1 en pleno dia.
// ============================================================================

namespace Atmosfera {

struct Cielo {
    float cenit[3];
    float horizonte[3];
    float resplandor[3];   // suma, solo en la parte baja y mirando al sol
    float contraluz[3];    // suma, en la parte baja y DE ESPALDAS al sol
    float luzSol[3];       // color de la luz que cae sobre el terreno
    float halo[3];         // suma: el resplandor alrededor del sol (dispersion de Mie)
    float brilloNocturno[3]; // suma: la franja tenue sobre el horizonte de noche
    float arco[3];         // suma: el arco amarillo del crepusculo, sobre el naranja
};

inline float mezcla(float a, float b, float t) { return a + (b - a) * t; }

// ⭐ EL CIELO POR ETAPAS. PEDIDO: "mejora todo el diseno del cielo para que sea
// mas realista". Antes el color era una recta entre la noche y el dia con un
// naranja sumado en medio, y el crepusculo quedaba como un dia a medio apagar.
// Un atardecer de verdad pasa por momentos distintos, y cada uno tiene su
// cielo:
//
//   luz 0.00  NOCHE          casi negro arriba, azul muy oscuro abajo.
//   luz 0.22  HORA AZUL      el sol ya se fue: arriba azul profundo, abajo un
//                            malva apagado; del lado del sol queda la brasa.
//   luz 0.50  PUESTA         el sol en el horizonte: naranja abajo, y arriba
//                            un azul que todavia es de dia pero mas hondo.
//   luz 0.78  HORA DORADA    el sol bajo: el horizonte palido y calido.
//   luz 1.00  DIA            azul limpio arriba, casi blanco abajo.
//
// Entre etapa y etapa se mezcla en linea recta.
constexpr int ETAPAS = 5;
inline void porEtapas(const float tabla[ETAPAS][3], float luz, float out[3]) {
    static const float EN[ETAPAS] = { 0.0f, 0.22f, 0.50f, 0.78f, 1.0f };
    int a = 0;
    while (a + 2 < ETAPAS && luz > EN[a + 1]) ++a;
    float t = (luz - EN[a]) / (EN[a + 1] - EN[a]);
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    for (int i = 0; i < 3; ++i) out[i] = mezcla(tabla[a][i], tabla[a + 1][i], t);
}

inline Cielo cieloPara(float luz) {
    if (luz < 0.0f) luz = 0.0f;
    if (luz > 1.0f) luz = 1.0f;

    // El cenit de dia es un azul mas hondo que antes: mirando arriba se cruza
    // poco aire y el azul de la dispersion de Rayleigh queda puro. De noche el
    // horizonte no es negro: es la luz de luna dispersada (y sigue bajo 0.15).
    static const float CENIT[ETAPAS][3] = {
        { 0.008f, 0.012f, 0.045f }, { 0.035f, 0.060f, 0.180f }, { 0.150f, 0.240f, 0.480f },
        { 0.180f, 0.370f, 0.740f }, { 0.160f, 0.390f, 0.820f } };
    static const float HORIZONTE[ETAPAS][3] = {
        { 0.035f, 0.055f, 0.125f }, { 0.300f, 0.230f, 0.330f }, { 0.900f, 0.560f, 0.340f },
        { 0.860f, 0.800f, 0.700f }, { 0.720f, 0.850f, 0.950f } };

    // Maximo a media transicion, nulo en pleno dia y de noche cerrada.
    const float ocaso = 1.0f - std::fabs(luz - 0.5f) * 2.0f;

    Cielo c;
    porEtapas(CENIT, luz, c.cenit);
    porEtapas(HORIZONTE, luz, c.horizonte);

    // EL HALO DEL SOL. El aire desvia hacia delante parte de la luz del sol
    // (dispersion de Mie, la de las particulas grandes): alrededor del disco
    // el cielo es mas claro. Blanco calido de dia, naranja al atardecer.
    for (int i = 0; i < 3; ++i) c.halo[i] = 0.0f;
    static const float HALO_DIA[3] = { 1.00f, 0.95f, 0.82f };
    static const float HALO_OCASO[3] = { 1.00f, 0.55f, 0.22f };
    for (int i = 0; i < 3; ++i)
        c.halo[i] = HALO_DIA[i] * 0.32f * luz * (1.0f - ocaso) + HALO_OCASO[i] * 0.55f * ocaso;

    // EL BRILLO NOCTURNO DEL AIRE: de noche cerrada, una franja muy tenue,
    // entre verde y azul, justo por encima del horizonte.
    const float noche = 1.0f - luz / 0.3f > 0.0f ? 1.0f - luz / 0.3f : 0.0f;
    c.brilloNocturno[0] = 0.006f * noche;
    c.brilloNocturno[1] = 0.016f * noche;
    c.brilloNocturno[2] = 0.020f * noche;
    // EL RESPLANDOR DEL LADO DEL SOL. Dorado mientras el sol sigue arriba y
    // rojo cuando ya se hundio: la luz que llega entonces ha cruzado tanto
    // aire que solo queda el rojo. Es la brasa que se ve tras la puesta.
    {
        float rojo = (0.60f - luz) / 0.35f;
        rojo = rojo < 0.0f ? 0.0f : (rojo > 1.0f ? 1.0f : rojo);
        c.resplandor[0] = mezcla(0.46f, 0.58f, rojo) * ocaso;
        c.resplandor[1] = mezcla(0.24f, 0.14f, rojo) * ocaso;
        c.resplandor[2] = mezcla(0.02f, 0.08f, rojo) * ocaso;
    }

    // EL ARCO DEL CREPUSCULO: encima de la franja naranja, del lado del sol,
    // el cielo pasa por un amarillo palido antes de llegar al azul. Sin el, el
    // naranja y el azul se mezclaban en un gris sucio.
    c.arco[0] = 0.15f * ocaso;
    c.arco[1] = 0.12f * ocaso;
    c.arco[2] = 0.03f * ocaso;

    // EL CINTURON DE VENUS: en el ocaso, el cielo bajo del lado CONTRARIO al
    // sol se tine de rosa y violeta. Es la sombra de la Tierra proyectada
    // sobre la atmosfera, con la franja iluminada por el sol rojo encima.
    // Sin esto, mirar de espaldas al atardecer daba un horizonte naranja
    // plano, igual en todas direcciones.
    c.contraluz[0] = 0.16f * ocaso;
    c.contraluz[1] = 0.05f * ocaso;
    c.contraluz[2] = 0.20f * ocaso;

    // EL COLOR DE LA LUZ DEL SOL. Blanca a mediodia (el terreno se ve con
    // sus colores de siempre), dorada cuando el sol esta bajo y azulada de
    // noche, que es como el ojo ve la luz de luna. Multiplica la luz del
    // cielo del terreno, no su brillo: de dia y a pleno sol da (1, 1, 1).
    //
    // Los dos tonos van compensados para que su luminancia ronde 1 (0.94 y
    // 0.92): cambian el COLOR de la luz, no cuanta hay. La noche ya la
    // oscurece LuzCielo.h; oscurecerla aqui otra vez la dejaba negra.
    static const float LUZ_LUNA[3]  = { 0.85f, 0.95f, 1.15f };
    static const float LUZ_OCASO[3] = { 1.08f, 0.88f, 0.66f };
    for (int i = 0; i < 3; ++i)
        c.luzSol[i] = mezcla(mezcla(LUZ_LUNA[i], 1.0f, luz),
                             LUZ_OCASO[i], ocaso * 0.8f);
    return c;
}

// El color en un punto del cielo. `senAlt` es la altura (0 horizonte, 1 cenit) y
// `haciaSol` el coseno del angulo con el sol.
inline void colorEn(const Cielo& c, float senAlt, float haciaSol, float out[3]) {
    float t = senAlt <= 0.0f ? 0.0f : senAlt;
    // ⭐ SIN FRANJAS. PEDIDO: "que no este partido en colores el cielo". Todo lo
    // que cambia con la altura lo hace ahora con curvas suaves (sin esquinas):
    // una esquina en la curva, aunque el color no salte, se ve como una raya
    // que separa dos cielos. La raiz cuadrada de antes subia en vertical desde
    // el horizonte y dejaba el color de abajo en una cinta estrecha.
    t = std::pow(t, 0.62f);               // la franja clara se queda abajo
    if (t > 1.0f) t = 1.0f;

    float s = haciaSol > 0.0f ? haciaSol : 0.0f;
    s = s * s * s;
    float e = haciaSol < 0.0f ? -haciaSol : 0.0f;
    e = e * e;
    const float bajo = 1.0f - t;

    // Lo que solo existe POR ENCIMA del horizonte: el halo del sol y el brillo
    // nocturno. En el horizonte mismo valen 0, que es lo que mantiene igual el
    // color de la niebla (el shader del terreno repite esa cuenta).
    float sobre = senAlt > 0.0f ? (senAlt / 0.16f < 1.0f ? senAlt / 0.16f : 1.0f) : 0.0f;
    sobre = sobre * sobre * (3.0f - 2.0f * sobre);
    const float m = haciaSol > 0.0f ? haciaSol : 0.0f;
    const float m2 = m * m, m4 = m2 * m2, m8 = m4 * m4, m32 = m8 * m8 * m8 * m8;
    // Tres anchos, como el halo de verdad: un velo amplio (el cielo es algo
    // mas claro en toda la mitad del sol), la corona y el nucleo.
    const float halo = (0.05f * m2 + 0.12f * m4 + 0.45f * m32) * sobre;
    // El arco amarillo: una banda a media altura, solo hacia el sol.
    // (una campana ancha, que se desvanece sin borde)
    float alArco = (senAlt - 0.24f) / 0.22f;
    alArco = std::exp(-alArco * alArco) * m2 * sobre;
    // La brasa ancha: tras la puesta, todo el cuadrante del sol sigue encendido
    // abajo, no solo un punto.
    const float ancha = 0.30f * m * sobre;
    float banda = (senAlt - 0.10f) / 0.14f;
    banda = std::exp(-banda * banda) * sobre;

    for (int i = 0; i < 3; ++i) {
        float v = mezcla(c.horizonte[i], c.cenit[i], t) +
                  (c.resplandor[i] * (s + ancha) + c.contraluz[i] * e) * bajo +
                  c.halo[i] * halo + c.brilloNocturno[i] * banda + c.arco[i] * alArco;
        out[i] = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }
}

// EL COLOR DE LA NIEBLA EN UNA DIRECCION: el del cielo en el horizonte de esa
// misma direccion. Es lo que hace que el terreno lejano se FUNDA con el cielo
// que tiene detras -- tambien al atardecer, mirando al sol, donde una niebla
// de un solo color dejaba una raya entre el monte gris y el cielo naranja.
//
// El shader del terreno repite esta cuenta (TerrenoGL.h); el test comprueba
// que la de aqui coincide con el cielo.
inline void colorNiebla(const Cielo& c, float haciaSol, float out[3]) {
    colorEn(c, 0.0f, haciaSol, out);
}

// Las estrellas salen al anochecer, cuando ya no queda luz que las tape.
inline float brilloEstrellas(float luz) {
    const float b = 1.0f - luz / 0.35f;
    return b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);
}

// La estrella numero `i`: direccion unitaria y brillo propio, siempre los mismos.
inline void estrella(int i, float out[4]) {
    auto azar = [](unsigned v) {
        v ^= v >> 16; v *= 0x7feb352du;
        v ^= v >> 15; v *= 0x846ca68bu;
        v ^= v >> 16;
        return (float)(v & 0xFFFFu) / 65535.0f;
    };
    const unsigned n = (unsigned)i * 3u + 1u;
    const float y = azar(n) * 2.0f - 1.0f;
    const float ang = azar(n + 1u) * 6.2831853f;
    const float r = std::sqrt(1.0f - y * y);
    out[0] = r * std::cos(ang);
    out[1] = y;
    out[2] = r * std::sin(ang);
    const float b = azar(n + 2u);
    out[3] = 0.35f + 0.65f * b * b;       // muchas tenues y pocas brillantes
}

// ============================================================================
// ⭐ EL CIELO DE NOCHE, DE VERDAD
// ============================================================================
// PEDIDO: "mejora todo el diseno del cielo para que sea mas realista". Antes
// eran 320 puntos iguales repartidos al azar. Un cielo oscuro de verdad tiene:
//
//   - MUCHAS estrellas tenues y muy pocas brillantes (aqui 1.400).
//   - COLOR: blancoazuladas las calientes, amarillas, naranjas y alguna roja.
//   - LA VIA LACTEA: casi la mitad se amontonan en una franja que cruza el
//     cielo, mas densa hacia el centro de la galaxia, con su velo lechoso.
//     La franja esta donde esta la de verdad (polo galactico norte en
//     AR 192.86, dec +27.13; centro en AR 266.40, dec -28.94).
//   - GIRAN alrededor del polo celeste, no de un eje cualquiera: a 23 grados
//     de latitud la estrella polar queda baja, al norte, y todo da vueltas a
//     su alrededor. Y como el dia sideral es 4 minutos mas corto que el solar,
//     el cielo de cada noche va un poco corrido: a lo largo del ano cambia.
//   - EXTINCION: cerca del horizonte el aire las apaga.
//   - Las tenues salen mas tarde al anochecer, y la luna llena las tapa.
//
// Todo en coordenadas ECUATORIALES (x hacia AR 0, z hacia el polo norte
// celeste); `ecuatorialAMundo` las pone en el cielo del jugador.
constexpr int N_CATALOGO = 1400;
constexpr double ANO_SIDERAL = 365.25;       // dias de mundo para una vuelta

struct Estrella {
    float d[3];        // direccion unitaria, ecuatorial
    float brillo;      // 0.18 (apenas se ve) .. 1 (de las mas brillantes)
    float color[3];
};

inline float azarCielo(unsigned v) {
    v ^= v >> 16; v *= 0x7feb352du;
    v ^= v >> 15; v *= 0x846ca68bu;
    v ^= v >> 16;
    return (float)(v & 0xFFFFFFu) / 16777215.0f;
}

// La base de la galaxia en ecuatoriales: `centro` hacia el centro galactico,
// `polo` el polo norte galactico y `lado` perpendicular a los dos.
inline void baseGalactica(float centro[3], float lado[3], float polo[3]) {
    const float G = 3.14159265f / 180.0f;
    const float pr = 192.86f * G, pd = 27.13f * G, cr = 266.40f * G, cd = -28.94f * G;
    polo[0] = std::cos(pd) * std::cos(pr); polo[1] = std::cos(pd) * std::sin(pr); polo[2] = std::sin(pd);
    float c[3] = { std::cos(cd) * std::cos(cr), std::cos(cd) * std::sin(cr), std::sin(cd) };
    // Se quita lo poco que el centro no es perpendicular al polo.
    const float p = c[0] * polo[0] + c[1] * polo[1] + c[2] * polo[2];
    float n = 0.0f;
    for (int i = 0; i < 3; ++i) { c[i] -= polo[i] * p; n += c[i] * c[i]; }
    n = std::sqrt(n);
    for (int i = 0; i < 3; ++i) centro[i] = c[i] / n;
    lado[0] = polo[1] * centro[2] - polo[2] * centro[1];
    lado[1] = polo[2] * centro[0] - polo[0] * centro[2];
    lado[2] = polo[0] * centro[1] - polo[1] * centro[0];
}

// Un punto de la franja: `l` la longitud galactica (0 el centro) y `b` cuanto
// se separa de ella, en radianes.
inline void puntoGalactico(float l, float b, float out[3]) {
    static float C[3], L[3], P[3];
    static bool lista = false;
    if (!lista) { baseGalactica(C, L, P); lista = true; }
    const float cb = std::cos(b), sb = std::sin(b), cl = std::cos(l), sl = std::sin(l);
    for (int i = 0; i < 3; ++i) out[i] = cb * (C[i] * cl + L[i] * sl) + P[i] * sb;
}

// Lo densa que es la Via Lactea a esa longitud (0..1): maxima en el centro
// (Sagitario), tenue en el lado contrario, y a parches.
inline float viaLactea(float l) {
    while (l > 3.14159265f) l -= 6.2831853f;
    while (l < -3.14159265f) l += 6.2831853f;
    const float nucleo = std::exp(-(l * l) / 1.3f);
    const float parches = 0.80f + 0.20f * std::sin(3.0f * l + 1.3f) * std::sin(7.0f * l + 0.4f);
    return (0.32f + 0.68f * nucleo) * parches;
}

// La estrella numero `i` del catalogo: siempre la misma.
inline Estrella estrellaCatalogo(int i) {
    const unsigned n = (unsigned)i * 7u + 101u;
    Estrella e;
    if (azarCielo(n) < 0.44f) {
        // De la franja: mas hacia el centro, y pegadas al plano.
        const float x = azarCielo(n + 1u) * 2.0f - 1.0f;
        const float l = 3.14159265f * x * (0.50f + 0.50f * std::fabs(x));
        const float b = (azarCielo(n + 2u) + azarCielo(n + 3u) + azarCielo(n + 4u) - 1.5f) * 0.26f;
        puntoGalactico(l, b, e.d);
    } else {
        const float z = azarCielo(n + 1u) * 2.0f - 1.0f;
        const float ang = azarCielo(n + 2u) * 6.2831853f;
        const float r = std::sqrt(1.0f - z * z);
        e.d[0] = r * std::cos(ang); e.d[1] = r * std::sin(ang); e.d[2] = z;
    }
    // Muchas tenues y muy pocas brillantes: una de cada cien pasa de 0.8.
    const float m = azarCielo(n + 5u);
    const float m2 = m * m;
    e.brillo = 0.18f + 0.82f * m2 * m2 * m2 * m;
    // El color, por temperatura.
    const float t = azarCielo(n + 6u);
    static const float TONO[5][3] = {
        { 0.74f, 0.84f, 1.00f },   // azulada
        { 1.00f, 1.00f, 1.00f },   // blanca
        { 1.00f, 0.96f, 0.82f },   // amarilla
        { 1.00f, 0.82f, 0.60f },   // naranja
        { 1.00f, 0.66f, 0.50f } }; // roja
    const int k = t < 0.16f ? 0 : (t < 0.56f ? 1 : (t < 0.80f ? 2 : (t < 0.94f ? 3 : 4)));
    for (int j = 0; j < 3; ++j) e.color[j] = TONO[k][j];
    return e;
}

// De ecuatorial al cielo del jugador (x este, y arriba, z sur), con las mismas
// cuentas que ponen al sol en su sitio. `tiempoSideral` en radianes: el angulo
// horario del punto AR 0; un astro en (ar, dec) tiene angulo horario
// tiempoSideral - ar.
inline void ecuatorialAMundo(const float e[3], float cosTS, float senTS,
                             float cosLat, float senLat, float out[3]) {
    const float ex = e[0] * cosTS + e[1] * senTS;
    const float ey = e[0] * senTS - e[1] * cosTS;
    out[0] = -ey;
    out[1] = cosLat * ex + senLat * e[2];
    out[2] = senLat * ex - cosLat * e[2];
}

// Cuanto deja pasar el aire a esa altura (seno): nada en el horizonte, todo
// por encima de unos 17 grados.
inline float extincion(float senAlt) {
    float t = (senAlt - 0.02f) / 0.28f;
    t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

// Cuanto se ve una estrella de ese brillo con el cielo asi de oscuro
// (`oscuridad` = brilloEstrellas()): las brillantes salen primero.
inline float visible(float oscuridad, float brillo) {
    const float v = (oscuridad - (1.0f - brillo) * 0.55f) / 0.45f;
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

// ESTRELLAS FUGACES. De vez en cuando (mas o menos una por minuto) un trazo
// corto cruza el cielo durante menos de un segundo. Sale del reloj del mundo:
// no guarda estado. Direcciones en el cielo del jugador.
struct Meteoro {
    bool hay = false;
    float cabeza[3] = {}, cola[3] = {};
    float brillo = 0.0f;
};
constexpr double RANURA_METEORO = 22.0;      // segundos de mundo
constexpr double DURA_METEORO = 0.75;
inline Meteoro meteoro(double segundos) {
    Meteoro m;
    if (segundos < 0.0) return m;
    const double ranura = std::floor(segundos / RANURA_METEORO);
    const unsigned n = (unsigned)((long long)ranura & 0xFFFFFFF) * 11u + 977u;
    if (azarCielo(n) > 0.36f) return m;
    const double empieza = ranura * RANURA_METEORO +
                           azarCielo(n + 1u) * (RANURA_METEORO - DURA_METEORO);
    const double p = (segundos - empieza) / DURA_METEORO;
    if (p < 0.0 || p > 1.0) return m;
    // Donde nace: alto en el cielo. Hacia donde va: de lado y hacia abajo.
    const float y = 0.35f + 0.55f * azarCielo(n + 2u);
    const float ang = azarCielo(n + 3u) * 6.2831853f;
    const float r = std::sqrt(1.0f - y * y);
    const float o[3] = { r * std::cos(ang), y, r * std::sin(ang) };
    const float giro = ang + 1.5708f + (azarCielo(n + 4u) - 0.5f) * 1.2f;
    const float v[3] = { std::cos(giro) * 0.85f, -0.50f, std::sin(giro) * 0.85f };
    const float largo = 0.20f + 0.18f * azarCielo(n + 5u);
    auto punto = [&](float s, float out[3]) {
        float q = 0.0f;
        for (int i = 0; i < 3; ++i) { out[i] = o[i] + v[i] * s; q += out[i] * out[i]; }
        q = std::sqrt(q);
        for (int i = 0; i < 3; ++i) out[i] /= q;
    };
    punto((float)p * largo, m.cabeza);
    punto((float)p * largo - 0.07f * (float)(p < 0.3 ? p / 0.3 : 1.0), m.cola);
    m.brillo = (float)std::sin(3.14159265 * p) * (0.6f + 0.4f * azarCielo(n + 6u));
    m.hay = m.cabeza[1] > 0.05f;
    return m;
}

// ============================================================================
// LAS FASES DE LA LUNA
// ============================================================================
// PEDIDO: "las fases lunares igual que el mundo real por noches, y que a veces
// se pueda ver la luna de dia pero mas transparente".
//
// Como en la realidad, la fase sale de cuanto se ha separado la luna del sol
// en el cielo (la ELONGACION): en luna nueva van juntos (y la luna no se ve),
// en cuarto creciente la luna va un cuarto de vuelta por detras del sol, en
// llena justo enfrente. Esa misma separacion es la que la pone en el cielo:
//
//   NUEVA     sale y se pone con el sol: invisible.
//   CRECIENTE se ve al atardecer, hacia el oeste, poco despues del sol.
//   CUARTO C. sale a mediodia: se ve DE DIA por la tarde, y de noche.
//   LLENA     sale al ponerse el sol y esta toda la noche.
//   CUARTO M. sale a medianoche: se ve de madrugada y DE DIA por la manana.
//
// Un ciclo dura lo mismo que el real, 29.53 noches. La fase que se dibuja
// cambia a MEDIODIA, nunca a media noche. Un mundo nuevo empieza con la luna
// gibosa creciente, a pocas noches de la llena.
constexpr double MES_LUNAR = 29.530588;
constexpr double EDAD_INICIAL = 10.0;     // dias de luna al crear el mundo
constexpr int NUM_FASES = 8;

// Los dias de luna (0 nueva .. 29.53), continuos. `dias` = dias de mundo.
inline double edadLunar(double dias) {
    double e = std::fmod(dias + EDAD_INICIAL, MES_LUNAR);
    return e < 0.0 ? e + MES_LUNAR : e;
}
// La separacion entre la luna y el sol, en radianes (0 nueva, PI llena).
inline double elongacion(double dias) {
    return edadLunar(dias) / MES_LUNAR * 6.283185307179586;
}
// La fase que se ve ESTA noche: 0 nueva, 1 creciente, 2 cuarto creciente,
// 3 gibosa creciente, 4 llena, 5 gibosa menguante, 6 cuarto menguante,
// 7 menguante. Cambia a mediodia.
inline int faseLunar(double dias) {
    const double noche = std::floor(dias + 0.5);
    const double e = edadLunar(noche) / MES_LUNAR * NUM_FASES;
    const int f = (int)std::floor(e + 0.5);
    return f % NUM_FASES;
}
// Cuanto disco esta iluminado (0 nueva, 1 llena).
inline float iluminada(double dias) {
    return (float)((1.0 - std::cos(elongacion(dias))) * 0.5);
}
// De dia la luna se ve, pero palida: el cielo claro la lava. `luz` es
// luzSolar() (0 noche, 1 dia).
inline float opacidadLuna(float luz) {
    luz = luz < 0.0f ? 0.0f : (luz > 1.0f ? 1.0f : luz);
    return 1.0f - 0.58f * luz;
}

} // namespace Atmosfera
