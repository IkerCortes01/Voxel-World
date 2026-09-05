#pragma once

#include "../fauna/AnimalMalla.h"
#include <vector>
#include <cmath>
#include <cstdint>

// ============================================================================
// EL CUERPO DEL JUGADOR
// ============================================================================
// Un modelo 3D anatomico en primera persona: al mirar hacia abajo se ve el
// torso, los brazos y las piernas, como en la vida real.
//
// ----------------------------------------------------------------------------
// LA REGLA QUE MANDA SOBRE TODO LO DEMAS
// ----------------------------------------------------------------------------
// ⚠️ LA CAMARA NO SE TOCA.
//
// El cuerpo se DIBUJA en el mundo, pero no mueve el ojo ni un milimetro: la
// posicion de camara la sigue calculando CameraSystem exactamente igual que
// antes (eyeHeight 1.62, crouchEyeHeight 0.80). Este archivo no incluye ni
// consulta la camara -- solo produce geometria colocada respecto al ojo.
//
// Esto es deliberado y es lo primero que hay que comprobar si algun dia se
// toca: si la vista se movio, el fallo NO esta aqui.
//
// ----------------------------------------------------------------------------
// POR QUE REUTILIZA EL GENERADOR DE FAUNA
// ----------------------------------------------------------------------------
// `AnimalMalla.h` ya resuelve el problema dificil: coser secciones elipticas
// en tubos organicos, con LOD por numero de lados, normales, color por vertice
// y caja envolvente. Esta escrito, probado y en uso por el pecari.
//
// Escribir un generador nuevo para el jugador habria significado mantener dos
// sistemas que hacen lo mismo -- que es exactamente el patron que produjo las
// dos tablas de drops del motor. Aqui solo se aportan las MEDIDAS y las ZONAS
// propias de un biped; el cosido es el mismo.
//
// ----------------------------------------------------------------------------
// ANATOMIA: DE DONDE SALEN LAS PROPORCIONES
// ----------------------------------------------------------------------------
// El jugador mide 1.80 m (PlayerTypes.h: height = 1.8f). Todas las medidas de
// abajo son fracciones de esa estatura, tomadas de proporciones antropometricas
// estandar (canon de 7.5 cabezas), no inventadas:
//
//     cabeza          1/7.5 de la estatura   = 0.240 m
//     hombros (ancho) 0.259 de la estatura   = 0.466 m
//     brazo entero    0.44  de la estatura   = 0.792 m
//     pierna entera   0.53  de la estatura   = 0.954 m
//     mano (largo)    0.108 de la estatura   = 0.194 m
//
// Que las proporciones sean reales importa para lo que se pidio: mirar abajo y
// ver TU cuerpo. Un torso demasiado corto o unos brazos demasiado largos se
// notan inmediatamente en primera persona, mucho mas que en tercera.
// ============================================================================

namespace PlayerBody {

using Fauna::V3;
using Fauna::MallaAnimal;
using Fauna::VerticeAnimal;
using Fauna::SeccionCuerpo;
using Fauna::GeneradorMalla;

// ----------------------------------------------------------------------------
// ZONAS DEL CUERPO HUMANO
// ----------------------------------------------------------------------------
// Las de `Fauna::ZonaCuerpo` son de cuadrupedo (LOMO, GRUPA, PEZUNA...). Un
// biped necesita las suyas, y sobre todo necesita distinguir la PIEL DESNUDA
// de lo cubierto: es lo que decide el color y el brillo de cada vertice.
enum class Zona : uint8_t {
    TORSO = 0,
    PECHO,
    ABDOMEN,
    HOMBRO,
    BRAZO,          // brazo superior
    ANTEBRAZO,
    MANO,
    DEDO,
    MUSLO,
    PANTORRILLA,
    PIE,
    CUELLO,
    _COUNT
};

// ----------------------------------------------------------------------------
// TONO DE PIEL
// ----------------------------------------------------------------------------
// Un solo parametro en vez de una paleta cerrada: la piel humana varia de
// forma continua, y encerrarla en "5 tonos" seria una limitacion arbitraria.
//
// El tono base se modula por zona (las palmas son mas claras, los nudillos mas
// oscuros) y por un ruido finisimo que rompe el plastico del color plano.
struct TonoPiel {
    float r = 0.80f, g = 0.62f, b = 0.50f;

    static TonoPiel claro()  { return { 0.94f, 0.78f, 0.68f }; }
    static TonoPiel medio()  { return { 0.80f, 0.62f, 0.50f }; }
    static TonoPiel oliva()  { return { 0.68f, 0.53f, 0.38f }; }
    static TonoPiel oscuro() { return { 0.42f, 0.29f, 0.21f }; }
};

// ----------------------------------------------------------------------------
// MEDIDAS DEL CUERPO
// ----------------------------------------------------------------------------
// Todo en METROS y derivado de la estatura, para que cambiar `estatura` escale
// el cuerpo entero sin descuadrarlo.
struct Medidas {
    float estatura = 1.80f;

    // --- Derivadas (canon de 7.5 cabezas) ---
    float cabeza()      const { return estatura / 7.5f; }        // 0.240
    float anchoHombros()const { return estatura * 0.259f; }      // 0.466
    float largoBrazo()  const { return estatura * 0.44f; }       // 0.792
    float largoPierna() const { return estatura * 0.53f; }       // 0.954
    float largoMano()   const { return estatura * 0.108f; }      // 0.194

    // El brazo se parte en dos: humero y antebrazo. La relacion real es
    // ~0.45 / 0.55 contando la mano aparte.
    float largoHumero()   const { return largoBrazo() * 0.45f; }
    float largoAntebrazo()const { return largoBrazo() * 0.40f; }

    // La pierna, igual: femur y tibia.
    float largoMuslo()      const { return largoPierna() * 0.46f; }
    float largoPantorrilla()const { return largoPierna() * 0.44f; }

    // Altura del hombro sobre el suelo. Es la referencia de la que cuelga el
    // brazo entero, y por tanto lo que decide donde aparecen las manos al
    // mirar abajo.
    float alturaHombro() const { return estatura * 0.82f; }      // 1.476

    // Altura de la cadera: de donde nacen las piernas.
    float alturaCadera() const { return estatura * 0.53f; }      // 0.954

    // Grosores. Un cuerpo humano no es un cilindro: el torso es una elipse
    // claramente mas ancha que profunda.
    float radioTorsoX() const { return anchoHombros() * 0.5f; }  // 0.233
    float radioTorsoZ() const { return anchoHombros() * 0.29f; } // 0.135
    float radioBrazo()  const { return estatura * 0.026f; }      // 0.047
    float radioMuslo()  const { return estatura * 0.042f; }      // 0.076
};

// ----------------------------------------------------------------------------
// ARTICULACIONES
// ----------------------------------------------------------------------------
// El esqueleto que anima el cuerpo. Cada una es un punto de pivote con su
// rotacion actual; la malla se construye respetandolos, asi que doblar el codo
// dobla la geometria en el sitio correcto y no en mitad del antebrazo.
//
// Se guardan los ANGULOS, no matrices: son 2 floats por articulacion en vez de
// 16, y es lo que hace barato interpolar entre poses.
struct Articulacion {
    float flexion = 0.0f;   // radianes, en el plano sagital (adelante/atras)
    float abduccion = 0.0f; // radianes, separacion lateral
};

struct Esqueleto {
    Articulacion hombroIzq, hombroDer;
    Articulacion codoIzq,   codoDer;
    Articulacion munecaIzq, munecaDer;
    Articulacion caderaIzq, caderaDer;
    Articulacion rodillaIzq, rodillaDer;
    Articulacion tobilloIzq, tobilloDer;

    // ⚠️ EL CODO Y LA RODILLA SOLO DOBLAN EN UN SENTIDO.
    //
    // Es una articulacion de bisagra: la rodilla humana no se dobla hacia
    // delante ni el codo hacia atras. Sin este limite, una animacion mal
    // interpolada produce la clase de deformidad que rompe la ilusion al
    // instante -- y en primera persona se ve de cerca.
    void aplicarLimites() {
        auto bisagra = [](Articulacion& a, float maxRad) {
            if (a.flexion < 0.0f)    a.flexion = 0.0f;
            if (a.flexion > maxRad)  a.flexion = maxRad;
            a.abduccion = 0.0f;      // una bisagra no abduce
        };
        // ~150 grados es el recorrido real de un codo; la rodilla ~135.
        bisagra(codoIzq,    2.62f);
        bisagra(codoDer,    2.62f);
        bisagra(rodillaIzq, 2.36f);
        bisagra(rodillaDer, 2.36f);
    }
};

// ----------------------------------------------------------------------------
// QUE PARTES SE DIBUJAN
// ----------------------------------------------------------------------------
// ⭐ EN PRIMERA PERSONA NO SE DIBUJA LA CABEZA.
//
// Y no es por ahorrar: es que la camara esta DENTRO de ella. Dibujarla llenaria
// la pantalla con el interior del craneo. Es la razon por la que los ojos, los
// dientes y la cabeza existen en el modelo pero se omiten en esta vista.
//
// La lista es explicita en vez de un "todo menos la cabeza" para que anadir una
// parte nueva obligue a decidir conscientemente si se ve desde dentro.
struct PartesVisibles {
    bool torso      = true;
    bool brazos     = true;
    bool manos      = true;
    bool piernas    = true;
    bool pies       = true;

    // Estas existen en el modelo completo (tercera persona, sombra, multi-
    // jugador) pero NO en primera persona.
    bool cabeza     = false;
    bool ojos       = false;
    bool dientes    = false;

    static PartesVisibles primeraPersona() { return PartesVisibles{}; }

    static PartesVisibles cuerpoCompleto() {
        PartesVisibles p;
        p.cabeza = p.ojos = p.dientes = true;
        return p;
    }
};

// ============================================================================
// EL CONSTRUCTOR DEL CUERPO
// ============================================================================
class Cuerpo {
public:
    Medidas       medidas;
    TonoPiel      piel = TonoPiel::medio();
    Esqueleto     esqueleto;
    uint32_t      semilla = 12345;   // variacion de color, determinista

    // ------------------------------------------------------------------------
    // COLOR DE UN VERTICE
    // ------------------------------------------------------------------------
    // La piel no es de un solo color plano. Se modula por:
    //   - ZONA: las palmas y la cara interna del brazo son mas claras; los
    //     nudillos y las rodillas, mas oscuros y algo rojizos.
    //   - RUIDO fino: rompe el aspecto de plastico sin llegar a verse como
    //     manchas.
    void colorDe(Zona z, int i, int j, float& r, float& g, float& b) const {
        r = piel.r; g = piel.g; b = piel.b;

        switch (z) {
            case Zona::MANO:
            case Zona::DEDO:
                // Las palmas y los dedos, mas claros y algo rosados: hay menos
                // melanina y mas riego.
                r *= 1.06f; g *= 1.02f; b *= 1.02f;
                break;
            case Zona::ANTEBRAZO:
                r *= 1.02f; g *= 1.01f; b *= 1.00f;
                break;
            case Zona::PANTORRILLA:
            case Zona::MUSLO:
                // Las piernas suelen estar menos expuestas al sol.
                r *= 0.97f; g *= 0.97f; b *= 0.98f;
                break;
            case Zona::ABDOMEN:
                r *= 0.99f; g *= 0.99f; b *= 1.00f;
                break;
            default:
                break;
        }

        // Ruido de piel: +-2.5%. Muy sutil a proposito -- lo que se busca es
        // que la superficie no sea uniforme, no que parezca sucia.
        const float n = GeneradorMalla::ruido(semilla, i, j);
        const float k = 0.975f + n * 0.05f;
        r *= k; g *= k; b *= k;

        // ⚠️ ACOTADO AL RANGO VALIDO.
        //
        // Lo caza el test "Piel: el color se mantiene dentro de rango": con el
        // tono claro (r = 0.94), el realce de la palma (x1.06) y el ruido
        // (x1.025) subian el rojo a 1.01.
        //
        // Pasarse de 1.0 no da un error -- da algo peor: OpenGL satura a
        // blanco, asi que la mano perderia su tono y se veria descolorida
        // justo en la parte que mas se mira en primera persona. Y solo le
        // pasaria a las pieles claras, que es la clase de bug que no se
        // reproduce cuando lo buscas.
        auto acotar = [](float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); };
        r = acotar(r); g = acotar(g); b = acotar(b);
    }

    // ------------------------------------------------------------------------
    // SECCIONES DE UN MIEMBRO
    // ------------------------------------------------------------------------
    // Un brazo o una pierna no son cilindros: tienen vientre muscular. El
    // radio sigue una curva que engorda a un tercio del recorrido y se afila
    // hacia la articulacion, que es como se ve un miembro real.
    static std::vector<SeccionCuerpo> seccionesMiembro(
            float largo, float radioBase, float radioPunta,
            int pasos, float vientre) {

        std::vector<SeccionCuerpo> secs;
        secs.reserve(pasos + 1);

        for (int i = 0; i <= pasos; ++i) {
            const float t = (float)i / (float)pasos;

            // Interpolacion base entre los dos radios.
            float r = radioBase + (radioPunta - radioBase) * t;

            // Vientre muscular: una campana centrada en t=0.33.
            const float d = (t - 0.33f) / 0.45f;
            r *= 1.0f + vientre * expf(-d * d);

            SeccionCuerpo s;
            s.z       = largo * t;
            s.radioX  = r;
            s.radioY  = r;
            s.centroY = 0.0f;
            secs.push_back(s);
        }
        return secs;
    }

    // ------------------------------------------------------------------------
    // SECCIONES DEL TORSO
    // ------------------------------------------------------------------------
    // El torso es la parte que mas se ve al mirar abajo, asi que es donde mas
    // importa la forma. No es un cilindro ni una caja:
    //
    //   - Es una ELIPSE: claramente mas ancho que profundo.
    //   - Se estrecha en la cintura y se ensancha en el pecho y la cadera.
    //   - El pecho tiene mas profundidad que el abdomen (caja toracica).
    std::vector<SeccionCuerpo> seccionesTorso(int pasos) const {
        const float cadera  = medidas.alturaCadera();
        const float hombro  = medidas.alturaHombro();
        const float alto    = hombro - cadera;
        const float rx      = medidas.radioTorsoX();
        const float rz      = medidas.radioTorsoZ();

        std::vector<SeccionCuerpo> secs;
        secs.reserve(pasos + 1);

        for (int i = 0; i <= pasos; ++i) {
            const float t = (float)i / (float)pasos;   // 0 cadera -> 1 hombro

            // Perfil del ancho: cadera ancha, cintura estrecha (t~0.35),
            // pecho ancho otra vez.
            float k;
            if (t < 0.35f) {
                const float u = t / 0.35f;
                k = 0.94f - 0.10f * u;            // cadera -> cintura
            } else {
                const float u = (t - 0.35f) / 0.65f;
                k = 0.84f + 0.22f * u;            // cintura -> hombros
            }

            // La profundidad crece mas que el ancho hacia el pecho: es la
            // caja toracica.
            const float kz = 0.90f + 0.30f * t;

            SeccionCuerpo s;
            s.z       = cadera + alto * t;
            s.radioX  = rx * k;
            s.radioY  = rz * kz;
            s.centroY = 0.0f;
            secs.push_back(s);
        }
        return secs;
    }

    // ------------------------------------------------------------------------
    // CONSTRUIR EL CUERPO
    // ------------------------------------------------------------------------
    // `lados` es el LOD: 12 para primera persona (se ve de cerca), 6 basta
    // para una sombra o un jugador lejano.
    //
    // El resultado esta en ESPACIO LOCAL DEL JUGADOR: origen en los pies,
    // +Y arriba, +Z hacia delante. Quien lo dibuje lo coloca donde toque.
    void construir(MallaAnimal& malla, const PartesVisibles& partes,
                   int lados = 12) const {
        malla.limpiar();

        // --- TORSO ---
        if (partes.torso) {
            const std::vector<SeccionCuerpo> secs = seccionesTorso(8);
            coserComoY(malla, secs, lados, Zona::TORSO);
        }

        // --- BRAZOS ---
        if (partes.brazos) {
            const float xHombro = medidas.anchoHombros() * 0.5f;
            construirBrazo(malla, -xHombro, esqueleto.hombroIzq,
                           esqueleto.codoIzq, lados, partes.manos);
            construirBrazo(malla, +xHombro, esqueleto.hombroDer,
                           esqueleto.codoDer, lados, partes.manos);
        }

        // --- PIERNAS ---
        if (partes.piernas) {
            const float xCadera = medidas.anchoHombros() * 0.19f;
            construirPierna(malla, -xCadera, esqueleto.caderaIzq,
                            esqueleto.rodillaIzq, lados, partes.pies);
            construirPierna(malla, +xCadera, esqueleto.caderaDer,
                            esqueleto.rodillaDer, lados, partes.pies);
        }

        malla.actualizarCaja();
    }

private:
    // Cose un tubo cuyo eje va en Y (vertical), que es como estan el torso y
    // los miembros de un biped. `AnimalMalla` cose a lo largo de Z porque un
    // cuadrupedo es horizontal: aqui se rota el resultado.
    void coserComoY(MallaAnimal& malla,
                    const std::vector<SeccionCuerpo>& secs,
                    int lados, Zona zona) const {
        if (secs.size() < 2 || lados < 3) return;

        const size_t base = malla.vertices.size();

        for (size_t i = 0; i < secs.size(); ++i) {
            const SeccionCuerpo& s = secs[i];
            for (int j = 0; j < lados; ++j) {
                const float a = 6.2831853f * (float)j / (float)lados;
                const float cx = cosf(a), cz = sinf(a);

                VerticeAnimal v;
                // Eje vertical: el "z" de la seccion es la ALTURA.
                v.pos    = V3(cx * s.radioX, s.z, cz * s.radioY);
                v.normal = V3(cx, 0.0f, cz);
                colorDe(zona, (int)i, j, v.r, v.g, v.b);
                v.zona = Fauna::ZonaCuerpo::TORSO;   // no se usa para color
                v.pelo = 0.0f;                        // piel desnuda
                malla.vertices.push_back(v);
            }
        }

        // Coser anillos consecutivos.
        for (size_t i = 0; i + 1 < secs.size(); ++i) {
            for (int j = 0; j < lados; ++j) {
                const int j2 = (j + 1) % lados;
                const uint16_t a = (uint16_t)(base + i * lados + j);
                const uint16_t b = (uint16_t)(base + i * lados + j2);
                const uint16_t c = (uint16_t)(base + (i + 1) * lados + j2);
                const uint16_t d = (uint16_t)(base + (i + 1) * lados + j);
                malla.indices.insert(malla.indices.end(), { a, b, c, a, c, d });
            }
        }
    }

    // Coloca un tubo ya cosido en una posicion, con una rotacion de flexion.
    void coserMiembro(MallaAnimal& malla,
                      const std::vector<SeccionCuerpo>& secs,
                      int lados, Zona zona,
                      float xOffset, float yTop, float flexion) const {
        if (secs.size() < 2 || lados < 3) return;

        const size_t base = malla.vertices.size();
        const float sf = sinf(flexion), cf = cosf(flexion);

        for (size_t i = 0; i < secs.size(); ++i) {
            const SeccionCuerpo& s = secs[i];
            // El miembro CUELGA: la seccion 0 esta arriba y crece hacia abajo.
            const float largoLocal = s.z;

            for (int j = 0; j < lados; ++j) {
                const float a = 6.2831853f * (float)j / (float)lados;
                const float cx = cosf(a), cz = sinf(a);

                // Punto sin rotar, colgando en -Y.
                const float px = cx * s.radioX;
                const float py = -largoLocal;
                const float pz = cz * s.radioY;

                // Flexion en el plano sagital: rota Y/Z alrededor del pivote.
                VerticeAnimal v;
                v.pos = V3(xOffset + px,
                           yTop + py * cf - pz * sf,
                           py * sf + pz * cf);
                v.normal = V3(cx, 0.0f, cz);
                colorDe(zona, (int)i, j, v.r, v.g, v.b);
                v.zona = Fauna::ZonaCuerpo::PATA;
                v.pelo = 0.0f;
                malla.vertices.push_back(v);
            }
        }

        for (size_t i = 0; i + 1 < secs.size(); ++i) {
            for (int j = 0; j < lados; ++j) {
                const int j2 = (j + 1) % lados;
                const uint16_t a = (uint16_t)(base + i * lados + j);
                const uint16_t b = (uint16_t)(base + i * lados + j2);
                const uint16_t c = (uint16_t)(base + (i + 1) * lados + j2);
                const uint16_t d = (uint16_t)(base + (i + 1) * lados + j);
                malla.indices.insert(malla.indices.end(), { a, b, c, a, c, d });
            }
        }
    }

    void construirBrazo(MallaAnimal& malla, float x,
                        const Articulacion& hombro, const Articulacion& codo,
                        int lados, bool conMano) const {
        const float rb = medidas.radioBrazo();
        const float yH = medidas.alturaHombro();

        // Humero: del hombro al codo. Vientre marcado (biceps).
        const std::vector<SeccionCuerpo> humero =
            seccionesMiembro(medidas.largoHumero(), rb * 1.15f, rb * 0.95f, 5, 0.18f);
        coserMiembro(malla, humero, lados, Zona::BRAZO, x, yH, hombro.flexion);

        // Antebrazo: del codo a la muneca. Se afila claramente hacia la
        // muneca, que es el rasgo que mas se nota al mirarse las manos.
        const float yCodo = yH - medidas.largoHumero() * cosf(hombro.flexion);
        const std::vector<SeccionCuerpo> antebrazo =
            seccionesMiembro(medidas.largoAntebrazo(), rb * 0.95f, rb * 0.62f, 5, 0.12f);
        coserMiembro(malla, antebrazo, lados, Zona::ANTEBRAZO, x, yCodo,
                     hombro.flexion + codo.flexion);

        if (conMano) {
            const float yMuneca = yCodo -
                medidas.largoAntebrazo() * cosf(hombro.flexion + codo.flexion);
            // La mano es mas plana que redonda: se aplasta el radio en Z.
            std::vector<SeccionCuerpo> mano =
                seccionesMiembro(medidas.largoMano() * 0.55f,
                                 rb * 0.70f, rb * 0.60f, 3, 0.05f);
            for (SeccionCuerpo& s : mano) s.radioY *= 0.55f;
            coserMiembro(malla, mano, lados, Zona::MANO, x, yMuneca,
                         hombro.flexion + codo.flexion);
        }
    }

    void construirPierna(MallaAnimal& malla, float x,
                         const Articulacion& cadera, const Articulacion& rodilla,
                         int lados, bool conPie) const {
        const float rm = medidas.radioMuslo();
        const float yC = medidas.alturaCadera();

        // Muslo: el miembro mas grueso del cuerpo.
        const std::vector<SeccionCuerpo> muslo =
            seccionesMiembro(medidas.largoMuslo(), rm * 1.10f, rm * 0.78f, 5, 0.15f);
        coserMiembro(malla, muslo, lados, Zona::MUSLO, x, yC, cadera.flexion);

        // Pantorrilla: vientre muy marcado (gemelo) y tobillo fino.
        const float yRodilla = yC - medidas.largoMuslo() * cosf(cadera.flexion);
        const std::vector<SeccionCuerpo> pantorrilla =
            seccionesMiembro(medidas.largoPantorrilla(), rm * 0.78f, rm * 0.42f, 6, 0.22f);
        coserMiembro(malla, pantorrilla, lados, Zona::PANTORRILLA, x, yRodilla,
                     cadera.flexion + rodilla.flexion);

        if (conPie) {
            const float yTobillo = yRodilla -
                medidas.largoPantorrilla() * cosf(cadera.flexion + rodilla.flexion);
            std::vector<SeccionCuerpo> pie =
                seccionesMiembro(medidas.estatura * 0.055f,
                                 rm * 0.42f, rm * 0.38f, 3, 0.0f);
            for (SeccionCuerpo& s : pie) s.radioY *= 0.60f;
            coserMiembro(malla, pie, lados, Zona::PIE, x, yTobillo,
                         cadera.flexion + rodilla.flexion);
        }
    }
};

} // namespace PlayerBody
