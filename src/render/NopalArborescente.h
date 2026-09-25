#pragma once

#include <cstdint>
#include <cmath>
#include <vector>

// ============================================================================
// NOPAL DE CASTILLA (Opuntia ficus-indica) -- MALLA ARBORESCENTE PROCEDURAL
// ============================================================================
// RESPONSABILIDAD UNICA: convertir una semilla y una posicion en una lista de
// triangulos. No lee el mundo, no toca OpenGL, no decide donde crece la
// planta. Eso lo hace quien llama.
//
// Es la misma frontera que ya respeta AnimalMalla.h con el pecari, y por el
// mismo motivo: asi la geometria se puede probar sin contexto grafico.
//
// ----------------------------------------------------------------------------
// POR QUE RAMIFICACION Y NO ROSETA
// ----------------------------------------------------------------------------
// El agave es una ROSETA: todas sus hojas salen del mismo punto, en abanico.
// Un nopal no. Es un arbusto ARBORESCENTE: cada penca (cladodio) brota del
// BORDE SUPERIOR de otra penca, y cada una puede dar varias hijas.
//
// La diferencia no es de adorno. Una roseta se genera con un bucle y un
// angulo; esto necesita recursion, porque la posicion de una penca depende de
// la de su padre, que depende de la del suyo.
//
// ----------------------------------------------------------------------------
// POR QUE NO SE USA glm
// ----------------------------------------------------------------------------
// El encargo pedia glm. No esta en este proyecto (external/ solo tiene doctest,
// glfw y stb) y anadirlo seria una dependencia nueva para lo que aqui son dos
// rotaciones y una traslacion.
//
// Se usa el Vec3 del motor y una matriz 3x4 propia -- exactamente el mismo
// criterio que ya sigue AnimalSkinning.h con TransHueso, que tambien podria
// haber usado glm y no lo hace. Un modulo mas que arrastre glm obligaria a
// meterlo en el build entero.
//
// ----------------------------------------------------------------------------
// LA UNIDAD: EL PIXEL VOXEL
// ----------------------------------------------------------------------------
// 1 bloque = 1 metro = 16 px. Todas las medidas se declaran en PX y se
// convierten en un solo sitio (PX), que es como ya trabaja PecariCuerpo.h.
// Declararlas en metros obligaria a pensar en decimales de cuatro cifras para
// algo que en pantalla es una rejilla de 16.
// ============================================================================

namespace Render {
namespace Nopal {

// ----------------------------------------------------------------------------
// VECTOR Y MATRIZ MINIMOS
// ----------------------------------------------------------------------------
// Propios, para que el header no dependa del Vec3 de main.cpp (que arrastra el
// motor entero) ni de glm. Es el mismo criterio de AnimalMalla.h con su V3.
struct V3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    V3() = default;
    V3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    V3 operator+(const V3& o) const { return V3(x+o.x, y+o.y, z+o.z); }
    V3 operator-(const V3& o) const { return V3(x-o.x, y-o.y, z-o.z); }
    V3 operator*(float s)     const { return V3(x*s, y*s, z*s); }

    float longitud() const { return std::sqrt(x*x + y*y + z*z); }
    V3 normalizado() const {
        const float l = longitud();
        return (l > 1e-6f) ? V3(x/l, y/l, z/l) : V3(0,1,0);
    }
};

inline V3 cruz(const V3& a, const V3& b) {
    return V3(a.y*b.z - a.z*b.y,
              a.z*b.x - a.x*b.z,
              a.x*b.y - a.y*b.x);
}

// Transformacion afin: rotacion 3x3 por filas + traslacion.
//
// Es lo minimo que hace falta para "coloca esta penca girada asi, ahi". No hay
// escalado: una penca hija es mas pequena, pero eso se consigue generando su
// geometria mas pequena, no escalando la matriz -- asi el GROSOR no encoge con
// ella y la planta no acaba con pencas de papel en las puntas.
struct Trans {
    float m[9];
    V3    t;

    static Trans identidad() {
        Trans r;
        r.m[0]=1; r.m[1]=0; r.m[2]=0;
        r.m[3]=0; r.m[4]=1; r.m[5]=0;
        r.m[6]=0; r.m[7]=0; r.m[8]=1;
        r.t = V3(0,0,0);
        return r;
    }

    V3 punto(const V3& p) const {
        return V3(m[0]*p.x + m[1]*p.y + m[2]*p.z + t.x,
                  m[3]*p.x + m[4]*p.y + m[5]*p.z + t.y,
                  m[6]*p.x + m[7]*p.y + m[8]*p.z + t.z);
    }

    // Las direcciones NO llevan traslacion. Y como la matriz es rotacion pura
    // (sin escalado), vale la misma sin transponer ni invertir.
    V3 direccion(const V3& d) const {
        return V3(m[0]*d.x + m[1]*d.y + m[2]*d.z,
                  m[3]*d.x + m[4]*d.y + m[5]*d.z,
                  m[6]*d.x + m[7]*d.y + m[8]*d.z);
    }

    // this * o
    Trans operator*(const Trans& o) const {
        Trans r;
        for (int f = 0; f < 3; ++f)
            for (int c = 0; c < 3; ++c)
                r.m[f*3+c] = m[f*3+0]*o.m[0*3+c]
                           + m[f*3+1]*o.m[1*3+c]
                           + m[f*3+2]*o.m[2*3+c];
        r.t = punto(o.t);
        return r;
    }
};

inline Trans rotacionY(float a) {
    const float c = std::cos(a), s = std::sin(a);
    Trans r = Trans::identidad();
    r.m[0] =  c; r.m[2] = s;
    r.m[6] = -s; r.m[8] = c;
    return r;
}

inline Trans rotacionZ(float a) {
    const float c = std::cos(a), s = std::sin(a);
    Trans r = Trans::identidad();
    r.m[0] = c; r.m[1] = -s;
    r.m[3] = s; r.m[4] =  c;
    return r;
}

inline Trans traslacion(const V3& d) {
    Trans r = Trans::identidad();
    r.t = d;
    return r;
}

// ----------------------------------------------------------------------------
// SALIDA
// ----------------------------------------------------------------------------
// Un vertice tal y como lo consume el mesher de este motor: posicion, color y
// UV en arrays paralelos (ver BatchCPU en render/MallaChunk.h).
//
// Se devuelve en ESTE formato y no como un `std::vector<Vertex>` entrelazado
// porque es el que el motor ya sabe subir a la GPU. Entregar otro obligaria a
// escribir un puente, y el puente seria el sitio donde algun dia se colaria un
// desajuste.
struct MallaNopal {
    std::vector<float> posiciones;   // x,y,z por vertice
    std::vector<float> colores;      // r,g,b,a por vertice
    std::vector<float> uvs;          // u,v por vertice

    size_t vertices() const { return posiciones.size() / 3; }

    void limpiar() {
        posiciones.clear();
        colores.clear();
        uvs.clear();
    }
};

// ----------------------------------------------------------------------------
// MEDIDAS, EN PIXELES VOXEL
// ----------------------------------------------------------------------------
// 1 bloque = 1 m = 16 px.
namespace Px {
    constexpr float POR_BLOQUE = 16.0f;
    constexpr float aBloques(float px) { return px / POR_BLOQUE; }
}

struct Config {
    // --- LA PENCA ---
    //
    // Medidas del cladodio raiz. Las hijas se generan mas pequenas (ver
    // MERMA_HIJA), porque un nopal se va afinando hacia las puntas.
    float largoPx  = 13.0f;   // eje mayor del ovalo
    float anchoPx  =  9.0f;   // eje menor
    float grosorPx =  3.0f;   // ⭐ VOLUMEN REAL, no un plano

    // Cuanto encoge cada generacion respecto a su padre.
    float mermaHija = 0.78f;

    // --- RAMIFICACION ---
    int   generaciones   = 3;      // profundidad del arbol
    int   hijasMin       = 1;
    int   hijasMax       = 3;

    // Apertura de la hija respecto a la normal del padre, en radianes.
    // 30-60 grados, que es el rango que pediste y coincide con lo medido en
    // Opuntia: el 95% de los cladodios se inclinan menos de 50 grados.
    float aperturaMin = 0.52f;     // 30 grados
    float aperturaMax = 1.05f;     // 60 grados

    // --- TUNAS ---
    // 3x3x4 px, en el borde superior de las pencas terminales.
    float tunaAnchoPx = 3.0f;
    float tunaAltoPx  = 4.0f;
    int   tunasMax    = 3;

    // --- COLORES ---
    // Van por vertice porque el motor no tiene shaders propios para esto: el
    // pipeline fijo interpola el color y la textura lo multiplica.
    float verdeCara[3]  = { 0.42f, 0.66f, 0.34f };   // cara ancha
    float verdeCanto[3] = { 0.31f, 0.52f, 0.26f };   // el filo, mas oscuro
    float tunaRoja[3]   = { 0.78f, 0.24f, 0.16f };
    float tunaNaranja[3]= { 0.90f, 0.52f, 0.14f };

    // --- ATLAS ---
    // Rectangulo de la penca dentro del atlas, en coordenadas UV 0..1.
    float uvPencaU0 = 0.0f, uvPencaV0 = 0.0f;
    float uvPencaU1 = 1.0f, uvPencaV1 = 1.0f;

    // ⭐ LA FRANJA DEL CANTO. Ver la nota grande de mapearUV.
    float uvCantoU0 = 0.0f, uvCantoV0 = 0.0f;
    float uvCantoU1 = 1.0f, uvCantoV1 = 0.0625f;   // 1/16: una fila del atlas

    uint32_t semilla = 1u;
};

// ----------------------------------------------------------------------------
// RUIDO DETERMINISTA
// ----------------------------------------------------------------------------
// LCG con semilla explicita, igual que FootstepSynth y AnimalMalla. Es lo que
// hace que el mismo nopal salga IGUAL siempre: la planta se regenera al
// recargar el chunk, y si dependiera de rand() cambiaria de forma cada vez.
class Rng {
public:
    explicit Rng(uint32_t s) : e(s ? s : 1u) {}

    uint32_t siguiente() {
        e = e * 1664525u + 1013904223u;
        return e;
    }
    float entre(float a, float b) {
        return a + (b - a) * ((float)(siguiente() >> 8) * (1.0f / 16777216.0f));
    }
    int entero(int a, int b) {   // [a, b] inclusive
        if (b <= a) return a;
        return a + (int)(siguiente() % (uint32_t)(b - a + 1));
    }

private:
    uint32_t e;
};

// ============================================================================
// EL GENERADOR
// ============================================================================
class GeneradorNopal {
public:
    // ------------------------------------------------------------------------
    // PUNTO DE ENTRADA
    // ------------------------------------------------------------------------
    // Genera la planta entera. TODAS las coordenadas salen relativas a la celda
    // origen, con (0,0,0) en su esquina inferior.
    //
    // ⭐ POR QUE RELATIVAS A LA CELDA ORIGEN, Y NO AL CHUNK.
    //
    // Un nopal ocupa hasta 2x3x2 metros, asi que casi siempre cruza una
    // frontera de chunk. Si la malla se generara en coordenadas de chunk, la
    // mitad que cae en el vecino habria que recortarla, pasarla al otro
    // mesher y coserla -- y el vecino puede no estar cargado todavia.
    //
    // Emitiendo todo desde la celda origen, el chunk que contiene esa celda
    // dibuja la planta ENTERA, incluso lo que sobresale. Es lo mismo que ya
    // hace este motor con los arboles, y lo que evita que media planta
    // desaparezca al cruzar una linea invisible.
    static void generar(MallaNopal& salida, const Config& cfg) {
        salida.limpiar();

        Rng rng(cfg.semilla);

        // La penca raiz nace vertical en el origen, mirando al norte (+Z).
        // Su transformacion es la identidad: es el ancla de todo el arbol.
        ramificar(salida, cfg, rng,
                  Trans::identidad(),
                  cfg.largoPx, cfg.anchoPx,
                  /*generacion=*/0);
    }

private:
    // ------------------------------------------------------------------------
    // LA RECURSION
    // ------------------------------------------------------------------------
    // Dibuja una penca y lanza sus hijas desde el BORDE SUPERIOR.
    //
    // Es iterativa en el sentido que importa --la profundidad esta acotada por
    // `generaciones`, que es 3-- asi que no hay riesgo de desbordar la pila:
    // con 3 hijas por nivel, el peor caso son 1+3+9+27 = 40 pencas.
    static void ramificar(MallaNopal& salida, const Config& cfg, Rng& rng,
                          const Trans& anclaje,
                          float largoPx, float anchoPx,
                          int generacion) {
        const bool terminal = (generacion >= cfg.generaciones - 1);

        emitirPenca(salida, cfg, anclaje, largoPx, anchoPx);

        // Las pencas terminales llevan fruto: la tuna sale en el borde de los
        // cladodios del ano, que son los de fuera. Ponerlas en las interiores
        // seria como colgarle manzanas al tronco.
        if (terminal) {
            emitirTunas(salida, cfg, rng, anclaje, largoPx, anchoPx);
            return;
        }

        const int nHijas = rng.entero(cfg.hijasMin, cfg.hijasMax);

        for (int i = 0; i < nHijas; ++i) {
            // ================================================================
            // ⭐ DONDE NACE LA HIJA: EL BORDE SUPERIOR PERIMETRAL
            // ================================================================
            // No en el centro de la penca ni en su punta: en un punto del
            // borde de arriba. Es lo que da la silueta escalonada del nopal,
            // con cada penca saliendo del filo de la anterior.
            //
            // `u` reparte los puntos de anclaje a lo largo de ese borde. Se
            // evita el centro exacto (0.5) sumando una desviacion, porque dos
            // hijas naciendo del mismo punto se solapan.
            const float u = (nHijas == 1)
                          ? rng.entre(0.30f, 0.70f)
                          : (0.18f + 0.64f * (float)i / (float)(nHijas - 1));

            // El borde superior del ovalo, en el espacio local de la penca.
            const V3 anclaLocal = puntoBordeSuperior(u, largoPx, anchoPx);

            // ================================================================
            // ⭐ LA ORIENTACION: APERTURA + GIRO
            // ================================================================
            // Dos rotaciones, y las dos hacen falta:
            //
            //   APERTURA (Z)  separa la hija del plano del padre. Sin ella
            //                 todas las pencas quedarian alineadas y la planta
            //                 seria una sola lamina vertical.
            //
            //   GIRO (Y)      reparte las hijas alrededor del eje. Sin el, la
            //                 planta seria PLANA: todas las ramas abririan
            //                 hacia el mismo lado.
            //
            // El giro se sesga hacia el lado contrario a la hija anterior
            // (el termino con `i`) para que no se amontonen, y lleva ruido
            // encima para que no quede simetrico como una antena.
            const float apertura = rng.entre(cfg.aperturaMin, cfg.aperturaMax);
            const float giro = (6.28318531f * (float)i / (float)nHijas)
                             + rng.entre(-0.55f, 0.55f);

            // El orden importa: primero se coloca el origen de la hija en el
            // borde del padre, luego se gira. Al reves, la rotacion moveria
            // tambien el punto de anclaje y la penca saldria despegada.
            const Trans local = traslacion(anclaLocal)
                              * rotacionY(giro)
                              * rotacionZ(apertura);

            ramificar(salida, cfg, rng,
                      anclaje * local,
                      largoPx * cfg.mermaHija,
                      anchoPx * cfg.mermaHija,
                      generacion + 1);
        }
    }

    // ------------------------------------------------------------------------
    // EL PERFIL DE LA PENCA
    // ------------------------------------------------------------------------
    // ⭐ NO ES UN RECTANGULO. Un cladodio es un ovalo en forma de lagrima:
    // estrecho donde se une al padre y ancho arriba.
    //
    // Se resuelve con un poligono de N lados cuyo radio varia con el angulo.
    // Ocho lados bastan: en la rejilla de un voxel, mas lados no se
    // distinguen y solo cuestan triangulos.
    //
    // El perfil sale de dos factores multiplicados:
    //   - la ELIPSE base (largo x ancho)
    //   - un ESTRECHAMIENTO en la parte baja, que es lo que da la lagrima
    static constexpr int LADOS = 8;

    static V3 puntoPerfil(int i, float largoPx, float anchoPx) {
        const float ang = 6.28318531f * (float)i / (float)LADOS;

        // Elipse: el eje largo va en Y (la penca crece hacia arriba).
        float px = std::cos(ang) * anchoPx * 0.5f;
        float py = std::sin(ang) * largoPx * 0.5f;

        // LA LAGRIMA. En la mitad inferior (py < 0) el ancho se reduce hasta
        // un 55%: es por donde la penca se une a su padre, y en la planta real
        // ese cuello es notablemente mas estrecho que el cuerpo.
        if (py < 0.0f) {
            const float t = -py / (largoPx * 0.5f);      // 0 centro .. 1 base
            px *= (1.0f - 0.45f * t * t);
        }

        // Se sube todo para que la penca NAZCA en y=0 en vez de estar centrada
        // en el origen: asi el punto de anclaje coincide con la base.
        py += largoPx * 0.5f;

        return V3(Px::aBloques(px), Px::aBloques(py), 0.0f);
    }

    // El punto del borde SUPERIOR donde se engancha una hija.
    // `u` va de 0 (un extremo del borde) a 1 (el otro).
    static V3 puntoBordeSuperior(float u, float largoPx, float anchoPx) {
        // El borde de arriba es el arco entre los lados que quedan por encima
        // del ecuador. Se interpola en el angulo, no entre dos vertices, para
        // que el punto caiga en la curva y no en la cuerda.
        const float ang = 3.14159265f * (0.15f + 0.70f * u);   // arco superior
        const float px = std::cos(ang) * anchoPx * 0.5f;
        const float py = std::sin(ang) * largoPx * 0.5f + largoPx * 0.5f;
        return V3(Px::aBloques(px), Px::aBloques(py), 0.0f);
    }

    // ------------------------------------------------------------------------
    // EMITIR UNA PENCA: DOS CARAS ANCHAS + EL CANTO
    // ------------------------------------------------------------------------
    // El grosor es REAL: la penca es un prisma de 8 lados, no un plano. Se
    // emite en tres partes porque cada una lleva UV distinta.
    static void emitirPenca(MallaNopal& salida, const Config& cfg,
                            const Trans& T, float largoPx, float anchoPx) {
        const float medioGrosor = Px::aBloques(cfg.grosorPx) * 0.5f;

        // Los dos anillos del prisma: delantero (+Z) y trasero (-Z).
        V3 frente[LADOS], detras[LADOS];
        for (int i = 0; i < LADOS; ++i) {
            const V3 p = puntoPerfil(i, largoPx, anchoPx);
            frente[i] = T.punto(V3(p.x, p.y,  medioGrosor));
            detras[i] = T.punto(V3(p.x, p.y, -medioGrosor));
        }

        const V3 nFrente = T.direccion(V3(0,0, 1));
        const V3 nDetras = T.direccion(V3(0,0,-1));

        // --- CARA ANCHA DELANTERA ---
        // Abanico de triangulos desde el centro. Es lo que usa la textura con
        // las areolas (los puntitos de donde salen las espinas).
        const V3 centroF = T.punto(V3(0.0f, Px::aBloques(largoPx * 0.5f),  medioGrosor));
        const V3 centroD = T.punto(V3(0.0f, Px::aBloques(largoPx * 0.5f), -medioGrosor));

        for (int i = 0; i < LADOS; ++i) {
            const int j = (i + 1) % LADOS;

            // Delante: sentido antihorario visto desde +Z.
            triangulo(salida, cfg,
                      centroF, frente[i], frente[j],
                      uvCara(cfg, 0.5f, 0.5f),
                      uvCaraDe(cfg, i, largoPx, anchoPx),
                      uvCaraDe(cfg, j, largoPx, anchoPx),
                      cfg.verdeCara, nFrente);

            // Detras: el orden se invierte, o el culling la descarta.
            triangulo(salida, cfg,
                      centroD, detras[j], detras[i],
                      uvCara(cfg, 0.5f, 0.5f),
                      uvCaraDe(cfg, j, largoPx, anchoPx),
                      uvCaraDe(cfg, i, largoPx, anchoPx),
                      cfg.verdeCara, nDetras);
        }

        // --- EL CANTO PERIMETRAL ---
        //
        // ⭐ AQUI VA LA FRANJA VERDE SOLIDA DEL ATLAS.
        //
        // Son los 3 px de grosor vistos de lado. En la planta real ese filo no
        // tiene areolas: son solo espinas en el borde. Usar la textura de la
        // cara ancha aqui estiraria los puntitos a lo largo de una tira de 3
        // px y se veria como un borron.
        for (int i = 0; i < LADOS; ++i) {
            const int j = (i + 1) % LADOS;

            // Normal del canto: hacia fuera, en el plano de la penca.
            const V3 medio = V3((frente[i].x + frente[j].x) * 0.5f,
                                (frente[i].y + frente[j].y) * 0.5f,
                                (frente[i].z + frente[j].z) * 0.5f);
            const V3 ejeCentro = T.punto(V3(0.0f,
                                            Px::aBloques(largoPx * 0.5f),
                                            0.0f));
            const V3 nCanto = (medio - ejeCentro).normalizado();

            // El quad del canto, como dos triangulos.
            //
            // La U recorre el PERIMETRO (asi la franja se repite a lo largo
            // del borde) y la V cruza el grosor. Es lo que hace que la tira
            // del atlas se lea como un filo continuo.
            const float u0 = (float)i / (float)LADOS;
            const float u1 = (float)j / (float)LADOS;

            triangulo(salida, cfg,
                      frente[i], detras[i], detras[j],
                      uvCanto(cfg, u0, 0.0f),
                      uvCanto(cfg, u0, 1.0f),
                      uvCanto(cfg, u1, 1.0f),
                      cfg.verdeCanto, nCanto);

            triangulo(salida, cfg,
                      frente[i], detras[j], frente[j],
                      uvCanto(cfg, u0, 0.0f),
                      uvCanto(cfg, u1, 1.0f),
                      uvCanto(cfg, u1, 0.0f),
                      cfg.verdeCanto, nCanto);
        }
    }

    // ------------------------------------------------------------------------
    // LAS TUNAS
    // ------------------------------------------------------------------------
    // Cubos de 3x3x4 px DE PIE sobre el borde superior de las pencas
    // terminales. Es como crece el fruto: erguido sobre el filo, no colgando.
    static void emitirTunas(MallaNopal& salida, const Config& cfg, Rng& rng,
                            const Trans& T, float largoPx, float anchoPx) {
        const int n = rng.entero(0, cfg.tunasMax);

        for (int i = 0; i < n; ++i) {
            const float u = rng.entre(0.15f, 0.85f);
            const V3 base = puntoBordeSuperior(u, largoPx, anchoPx);

            // Una variedad da tunas de UN color -- es de la planta, no del
            // fruto -- asi que se elige una vez por nopal con la semilla.
            const bool roja = ((cfg.semilla >> 3) & 1u) != 0u;
            const float* col = roja ? cfg.tunaRoja : cfg.tunaNaranja;

            const float a = Px::aBloques(cfg.tunaAnchoPx) * 0.5f;
            const float h = Px::aBloques(cfg.tunaAltoPx);

            // Se hunde un poco en la penca: un fruto posado justo encima deja
            // una junta visible, y ademas se despegaria al girar la planta.
            const float hundido = Px::aBloques(1.0f);

            emitirCubo(salida, cfg, T,
                       V3(base.x, base.y - hundido, base.z),
                       a, h, a, col);
        }
    }

    // Cubo alineado al espacio local de la penca, con sus seis caras.
    static void emitirCubo(MallaNopal& salida, const Config& cfg,
                           const Trans& T, const V3& base,
                           float medioX, float alto, float medioZ,
                           const float col[3]) {
        const float x0 = base.x - medioX, x1 = base.x + medioX;
        const float y0 = base.y,          y1 = base.y + alto;
        const float z0 = base.z - medioZ, z1 = base.z + medioZ;

        struct Cara { V3 a, b, c, d; V3 n; };
        const Cara caras[6] = {
            {{x0,y1,z0},{x1,y1,z0},{x1,y1,z1},{x0,y1,z1}, {0,1,0}},   // arriba
            {{x0,y0,z1},{x1,y0,z1},{x1,y0,z0},{x0,y0,z0}, {0,-1,0}},  // abajo
            {{x0,y0,z1},{x0,y1,z1},{x1,y1,z1},{x1,y0,z1}, {0,0,1}},   // +Z
            {{x1,y0,z0},{x1,y1,z0},{x0,y1,z0},{x0,y0,z0}, {0,0,-1}},  // -Z
            {{x1,y0,z1},{x1,y1,z1},{x1,y1,z0},{x1,y0,z0}, {1,0,0}},   // +X
            {{x0,y0,z0},{x0,y1,z0},{x0,y1,z1},{x0,y0,z1}, {-1,0,0}},  // -X
        };

        for (const Cara& f : caras) {
            const V3 A = T.punto(f.a), B = T.punto(f.b);
            const V3 C = T.punto(f.c), D = T.punto(f.d);
            const V3 N = T.direccion(f.n);

            // La tuna usa una esquina pequena del atlas: su textura es un
            // color casi plano, asi que no necesita mas.
            const float tu0 = cfg.uvPencaU0, tv0 = cfg.uvPencaV0;
            const float tu1 = cfg.uvPencaU0 + (cfg.uvPencaU1 - cfg.uvPencaU0) * 0.25f;
            const float tv1 = cfg.uvPencaV0 + (cfg.uvPencaV1 - cfg.uvPencaV0) * 0.25f;

            triangulo(salida, cfg, A, B, C,
                      {tu0,tv0}, {tu0,tv1}, {tu1,tv1}, col, N);
            triangulo(salida, cfg, A, C, D,
                      {tu0,tv0}, {tu1,tv1}, {tu1,tv0}, col, N);
        }
    }

    // ------------------------------------------------------------------------
    // ⭐⭐ MAPEO UV: DOS ZONAS DEL ATLAS PARA DOS SUPERFICIES DISTINTAS
    // ------------------------------------------------------------------------
    // LA CARA ANCHA usa el rectangulo grande del atlas, el que lleva dibujadas
    // las AREOLAS -- los puntitos de donde salen las espinas. Es la superficie
    // que el jugador mira de frente y la que identifica la planta.
    //
    // EL CANTO usa una FRANJA de una fila del atlas, verde solida. Y no es un
    // atajo: es lo correcto por dos razones.
    //
    //   1. BOTANICA. El filo de un cladodio no tiene areolas repartidas: solo
    //      las espinas del borde. Ponerle la misma textura le pintaria
    //      puntitos donde no los hay.
    //
    //   2. TECNICA, y es la que de verdad obliga. El canto mide 3 px de
    //      grosor. Si se le aplicara el rectangulo de la cara --pensado para
    //      13x9 px-- la textura se comprimiria a un tercio de pixel de alto:
    //      el filtrado mezclaria filas enteras y el resultado seria una banda
    //      de color indefinido, distinta en cada penca segun su angulo.
    //
    // COMO SE APLICA LA FRANJA. En el canto, la U recorre el PERIMETRO y la V
    // cruza el grosor. Asi la franja se lee a lo largo del filo como una cinta
    // continua, en vez de estirarse una sola vez alrededor de toda la penca.
    struct UV { float u, v; };

    static UV uvCara(const Config& cfg, float u, float v) {
        return UV{ cfg.uvPencaU0 + (cfg.uvPencaU1 - cfg.uvPencaU0) * u,
                   cfg.uvPencaV0 + (cfg.uvPencaV1 - cfg.uvPencaV0) * v };
    }

    static UV uvCanto(const Config& cfg, float u, float v) {
        return UV{ cfg.uvCantoU0 + (cfg.uvCantoU1 - cfg.uvCantoU0) * u,
                   cfg.uvCantoV0 + (cfg.uvCantoV1 - cfg.uvCantoV0) * v };
    }

    // UV de un vertice del perfil, proyectando el ovalo sobre el rectangulo
    // del atlas. Se normaliza a 0..1 usando el propio perfil, asi que la
    // textura encaja igual sea cual sea el tamano de la penca -- que es lo que
    // permite que las hijas mas pequenas no salgan con la imagen recortada.
    static UV uvCaraDe(const Config& cfg, int i, float largoPx, float anchoPx) {
        const V3 p = puntoPerfil(i, largoPx, anchoPx);
        const float u = 0.5f + p.x / (Px::aBloques(anchoPx));
        const float v = p.y / Px::aBloques(largoPx);
        return uvCara(cfg, u, v);
    }

    // ------------------------------------------------------------------------
    // ESCRIBIR UN TRIANGULO
    // ------------------------------------------------------------------------
    // El sombreado va en el COLOR DE VERTICE, no en una normal: este motor
    // dibuja el terreno con el pipeline fijo y `glInterleavedArrays`, donde la
    // normal que se envia no ilumina nada por si sola.
    //
    // Se aproxima con un lambert contra una luz fija desde arriba, que es lo
    // mismo que hace el mesher del terreno con DIR_BRIGHT. Sin esto, la planta
    // saldria de un verde plano y no se le veria el volumen.
    static void triangulo(MallaNopal& s, const Config& cfg,
                          const V3& a, const V3& b, const V3& c,
                          UV ua, UV ub, UV uc,
                          const float col[3], const V3& normal) {
        (void)cfg;

        // Luz desde arriba y algo de lado, como el resto del motor.
        static const V3 LUZ = V3(0.35f, 0.90f, 0.25f).normalizado();
        const V3 n = normal.normalizado();

        float lam = n.x*LUZ.x + n.y*LUZ.y + n.z*LUZ.z;
        if (lam < 0.0f) lam = -lam * 0.45f;          // la cara opuesta recibe rebote
        const float f = 0.55f + 0.45f * lam;         // nunca negro del todo

        const V3 P[3] = { a, b, c };
        const UV U[3] = { ua, ub, uc };

        for (int i = 0; i < 3; ++i) {
            s.posiciones.push_back(P[i].x);
            s.posiciones.push_back(P[i].y);
            s.posiciones.push_back(P[i].z);

            s.colores.push_back(col[0] * f);
            s.colores.push_back(col[1] * f);
            s.colores.push_back(col[2] * f);
            s.colores.push_back(1.0f);

            s.uvs.push_back(U[i].u);
            s.uvs.push_back(U[i].v);
        }
    }
};

} // namespace Nopal
} // namespace Render
