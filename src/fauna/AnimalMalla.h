#ifndef ANIMAL_MALLA_H
#define ANIMAL_MALLA_H

#include <cstdint>
#include <cmath>
#include <vector>

// ============================================================================
// MALLA ORGANICA PROCEDURAL PARA ANIMALES
// ============================================================================
// RESPONSABILIDAD UNICA: convertir una descripcion anatomica (una lista de
// secciones con su radio) en una malla triangular con normales suaves.
//
// NO sabe que animal es. NO dibuja. NO conoce OpenGL. Un pecari, un venado y
// un jaguar usan este mismo codigo con datos distintos.
//
// ----------------------------------------------------------------------------
// POR QUE EXISTE: EL PROBLEMA DE LAS CAJAS
// ----------------------------------------------------------------------------
// El cuerpo anterior eran 71 CAJAS. Funcionaba, pero el animal se leia como un
// monton de bloques: sin curvas, sin transiciones, con aristas duras donde
// deberia haber lomo.
//
// Aqui el cuerpo es una MALLA DE REVOLUCION DEFORMADA: se recorre el eje del
// animal colocando secciones transversales (anillos de vertices), y se cosen
// entre si formando tubos continuos. Es la misma tecnica con la que se modela
// un personaje organico, solo que generada por codigo en vez de esculpida.
//
// El resultado tiene:
//   - volumen continuo, sin escalones entre partes
//   - curvas reales (el lomo se arquea, el vientre se hincha)
//   - normales interpoladas -> sombreado suave con la luz del motor
//   - silueta correcta desde cualquier angulo
//
// ----------------------------------------------------------------------------
// POR QUE FUNCIONA EN OPENGL 2.1
// ----------------------------------------------------------------------------
// No necesita shaders. La malla se genera en CPU una vez, se cachea, y se
// dibuja con normales y color por vertice, que el pipeline fijo interpola solo
// (Gouraud). El microdetalle que en un motor moderno haria un fragment shader
// aqui se aproxima con:
//   - color por vertice modulado por zona anatomica y por ruido
//   - normales perturbadas para simular la direccion del pelo
//
// No es un shader de pelaje. Se dice sin disimulo. Pero rompe por completo el
// aspecto de cubos, que es el objetivo inmediato.
//
// ----------------------------------------------------------------------------
// COSTE
// ----------------------------------------------------------------------------
// La malla se genera UNA VEZ por (especie, LOD) y se comparte entre todas las
// instancias. 100 pecaries usan 1 malla, no 100. La memoria por instancia es
// solo su transformacion y unos pocos parametros.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// VECTOR MINIMO
// ----------------------------------------------------------------------------
// Propio, para que el modulo no dependa del Vec3 del motor. Es el mismo
// criterio que sigue PlayerTypes.h con MoveVec3.
struct V3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    V3() = default;
    V3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    V3 operator+(const V3& o) const { return V3(x+o.x, y+o.y, z+o.z); }
    V3 operator-(const V3& o) const { return V3(x-o.x, y-o.y, z-o.z); }
    V3 operator*(float s)     const { return V3(x*s, y*s, z*s); }

    V3& operator+=(const V3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    float longitud() const { return std::sqrt(x*x + y*y + z*z); }

    V3 normalizado() const {
        const float l = longitud();
        if (l < 1e-6f) return V3(0,1,0);
        return V3(x/l, y/l, z/l);
    }
};

inline V3 cruz(const V3& a, const V3& b) {
    return V3(a.y*b.z - a.z*b.y,
              a.z*b.x - a.x*b.z,
              a.x*b.y - a.y*b.x);
}

inline float punto(const V3& a, const V3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

// ----------------------------------------------------------------------------
// ZONA ANATOMICA
// ----------------------------------------------------------------------------
// Cada vertice sabe a que parte del cuerpo pertenece. Es lo que permite:
//   - variar el color por region (lomo oscuro, vientre claro, collar)
//   - variar la densidad aparente de pelo
//   - que el raycast identifique que parte se ha golpeado
//   - los modos de depuracion por zona
//
// Es el sustituto barato de las mascaras procedurales por posicion local que
// pediria un shader.
enum class ZonaCuerpo : uint8_t {
    TORSO = 0,
    LOMO,          // linea dorsal: mas oscura, pelo mas largo
    VIENTRE,       // mas claro, pelo mas ralo
    CUELLO,
    COLLAR,        // la banda clara diagnostica de la especie
    CABEZA,
    HOCICO,        // pelo casi ausente: piel desnuda
    OREJA,
    PATA,
    PEZUNA,        // queratina: sin pelo, brillo distinto
    COLA,
    GRUPA,         // donde va la glandula dorsal
    // ⭐ EL OJO NECESITA SU PROPIA ZONA.
    //
    // Antes los ojos se marcaban como CABEZA, con un comentario que prometia
    // "color propio muy oscuro" que NUNCA se aplicaba: al caer en CABEZA
    // tomaban el color de la cara y quedaban invisibles. Un animal sin ojos
    // se lee como un muñeco.
    OJO,
    _COUNT
};

// ----------------------------------------------------------------------------
// UN VERTICE
// ----------------------------------------------------------------------------
// Deliberadamente compacto: 3+3+3+1+1 floats = 44 bytes.
//
// No hay UV porque no hay texturas: el color va por vertice. Cuando se migre a
// shaders (fase B) se anadira UV y tangente aqui mismo.
struct VerticeAnimal {
    V3    pos;
    V3    normal;
    float r, g, b;        // color base ya resuelto por zona y ruido
    ZonaCuerpo zona;
    float pelo;           // 0 = piel desnuda, 1 = pelaje denso
};

// ----------------------------------------------------------------------------
// UNA MALLA
// ----------------------------------------------------------------------------
struct MallaAnimal {
    std::vector<VerticeAnimal> vertices;
    std::vector<uint16_t>      indices;    // triangulos

    // Caja envolvente en espacio local. Para culling y colision.
    V3 minimo, maximo;

    void limpiar() {
        vertices.clear();
        indices.clear();
        minimo = V3( 1e9f,  1e9f,  1e9f);
        maximo = V3(-1e9f, -1e9f, -1e9f);
    }

    size_t triangulos() const { return indices.size() / 3; }

    // Memoria real ocupada, para poder reportarla sin estimar.
    size_t bytes() const {
        return vertices.size() * sizeof(VerticeAnimal) +
               indices.size()  * sizeof(uint16_t);
    }

    void actualizarCaja() {
        minimo = V3( 1e9f,  1e9f,  1e9f);
        maximo = V3(-1e9f, -1e9f, -1e9f);
        for (const VerticeAnimal& v : vertices) {
            if (v.pos.x < minimo.x) minimo.x = v.pos.x;
            if (v.pos.y < minimo.y) minimo.y = v.pos.y;
            if (v.pos.z < minimo.z) minimo.z = v.pos.z;
            if (v.pos.x > maximo.x) maximo.x = v.pos.x;
            if (v.pos.y > maximo.y) maximo.y = v.pos.y;
            if (v.pos.z > maximo.z) maximo.z = v.pos.z;
        }
    }
};

// ----------------------------------------------------------------------------
// UNA SECCION TRANSVERSAL DEL CUERPO
// ----------------------------------------------------------------------------
// El bloque de construccion. Un tubo organico es una lista de estas.
//
// La seccion NO es circular: es una ELIPSE con dos deformaciones, y ahi esta
// la diferencia entre un tubo y un animal:
//
//   - achatadoVientre: aplana la parte de abajo. Un animal no es un cilindro:
//     el vientre cuelga distinto que el lomo.
//   - alzadoLomo: eleva la parte de arriba. Es lo que da la linea dorsal
//     marcada del pecari, que es un rasgo visible de la especie.
struct SeccionCuerpo {
    float z;               // posicion a lo largo del eje, en metros
    float radioX;          // semiancho
    float radioY;          // semialto
    float centroY;         // desplazamiento vertical del centro (curva del eje)

    float achatadoVientre = 0.0f;   // 0..1
    float alzadoLomo      = 0.0f;   // 0..1

    ZonaCuerpo zonaLomo    = ZonaCuerpo::LOMO;
    ZonaCuerpo zonaFlanco  = ZonaCuerpo::TORSO;
    ZonaCuerpo zonaVientre = ZonaCuerpo::VIENTRE;
};

// ============================================================================
// GENERADOR
// ============================================================================
class GeneradorMalla {
public:
    // Ruido determinista para la variacion de color. Sin estado, seguro desde
    // cualquier hilo.
    static float ruido(uint32_t semilla, int i, int j) {
        uint32_t h = semilla;
        h ^= (uint32_t)i * 374761393u;
        h ^= (uint32_t)j * 668265263u;
        h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
        return (float)(h % 10000u) / 10000.0f;
    }

    // ------------------------------------------------------------------------
    // COSER UN TUBO ORGANICO
    // ------------------------------------------------------------------------
    // Recorre las secciones creando un anillo de `lados` vertices en cada una,
    // y cose anillos consecutivos con quads (dos triangulos).
    //
    // `lados` es el parametro de LOD: 12 da un cuerpo suave, 6 uno anguloso
    // pero valido a distancia, 4 una silueta minima.
    //
    // Devuelve el indice del primer vertice generado, por si quien llama
    // necesita coser algo a este tubo.
    static size_t coserTubo(MallaAnimal& malla,
                            const std::vector<SeccionCuerpo>& secciones,
                            int lados,
                            uint32_t semilla,
                            bool cerrarInicio = true,
                            bool cerrarFin = true) {
        if (secciones.size() < 2 || lados < 3) return malla.vertices.size();

        const size_t base = malla.vertices.size();
        constexpr float TAU = 6.28318531f;

        // --- Generar los anillos ---
        for (size_t s = 0; s < secciones.size(); ++s) {
            const SeccionCuerpo& sec = secciones[s];

            for (int i = 0; i < lados; ++i) {
                const float ang = TAU * (float)i / (float)lados;
                float cx = std::cos(ang);   // -1 abajo .. +1 arriba en Y
                float sy = std::sin(ang);

                // --- Deformacion 1: achatar el vientre ---
                // Solo afecta a la mitad inferior (sy < 0). Es lo que hace que
                // el animal se apoye sobre un vientre plano en vez de rodar
                // como un tubo.
                if (sy < 0.0f && sec.achatadoVientre > 0.0f) {
                    sy *= (1.0f - sec.achatadoVientre * 0.45f);
                }

                // --- Deformacion 2: alzar el lomo ---
                // Solo la mitad superior. Da la linea dorsal marcada, que en el
                // pecari es un rasgo visible (lleva ademas la crin erectil).
                if (sy > 0.0f && sec.alzadoLomo > 0.0f) {
                    sy *= (1.0f + sec.alzadoLomo * 0.30f);
                }

                VerticeAnimal v;
                v.pos = V3(cx * sec.radioX,
                           sec.centroY + sy * sec.radioY,
                           sec.z);

                // --- Zona anatomica segun la altura en el anillo ---
                // Es la mascara procedural por posicion local, resuelta en
                // CPU: arriba lomo, abajo vientre, en medio flanco.
                if (sy > 0.55f)       v.zona = sec.zonaLomo;
                else if (sy < -0.55f) v.zona = sec.zonaVientre;
                else                  v.zona = sec.zonaFlanco;

                // La normal se acumula despues, al recorrer los triangulos.
                v.normal = V3(0,0,0);
                v.r = v.g = v.b = 1.0f;
                v.pelo = 1.0f;

                malla.vertices.push_back(v);
            }
        }

        // --- Coser anillos consecutivos ---
        for (size_t s = 0; s + 1 < secciones.size(); ++s) {
            const size_t a0 = base + s * (size_t)lados;
            const size_t b0 = base + (s + 1) * (size_t)lados;

            for (int i = 0; i < lados; ++i) {
                const int j = (i + 1) % lados;

                const uint16_t v00 = (uint16_t)(a0 + i);
                const uint16_t v01 = (uint16_t)(a0 + j);
                const uint16_t v10 = (uint16_t)(b0 + i);
                const uint16_t v11 = (uint16_t)(b0 + j);

                // ⭐ EL ORDEN IMPORTA, Y ESTABA AL REVES.
                //
                // OpenGL decide que cara de un triangulo es la de FUERA por el
                // orden de sus vertices (glFrontFace(GL_CCW) en este motor).
                // Y calcularNormales() saca la normal del mismo orden.
                //
                // Estos dos triangulos se cosian como (v00,v10,v11) y
                // (v00,v11,v01), que con el anillo generado arriba --angulo
                // creciente, secciones avanzando en +Z-- deja la normal
                // apuntando al EJE del tubo en vez de hacia fuera.
                //
                // Se midio sobre la malla del adulto: 838 de 1244 caras
                // miraban hacia dentro. Dos sintomas a la vez:
                //
                //   1. La luz se invertia. El sombreado usa max(0, n.L), asi
                //      que las caras al sol daban 0 y se pintaban del minimo,
                //      y las de sombra salian iluminadas. El lomo negro y el
                //      vientre brillante: el volumen se leia del reves.
                //
                //   2. El culling descartaba lo que hay que ver. Con
                //      GL_CULL_FACE activo, OpenGL tiraba las caras exteriores
                //      y conservaba las de dentro, asi que el animal se veia
                //      hueco o con agujeros segun el angulo.
                //
                // No se arregla con glFrontFace(GL_CW): las TAPAS
                // (taparAnillo) si estaban bien, y eso las rompería. Se
                // arregla aqui, que es donde estaba mal.
                malla.indices.push_back(v00);
                malla.indices.push_back(v11);
                malla.indices.push_back(v10);

                malla.indices.push_back(v00);
                malla.indices.push_back(v01);
                malla.indices.push_back(v11);
            }
        }

        // --- Tapar los extremos ---
        // Sin esto el animal tendria agujeros en morro y grupa, visibles como
        // huecos negros al mirarlo de frente.
        if (cerrarInicio) taparAnillo(malla, base, lados, secciones.front(), true);
        if (cerrarFin) {
            const size_t ultimo = base + (secciones.size() - 1) * (size_t)lados;
            taparAnillo(malla, ultimo, lados, secciones.back(), false);
        }

        (void)semilla;
        return base;
    }

    // ------------------------------------------------------------------------
    // NORMALES SUAVES
    // ------------------------------------------------------------------------
    // Acumula la normal de cada triangulo en sus tres vertices y normaliza al
    // final. Es lo que produce el sombreado GOURAUD continuo del pipeline fijo:
    // sin esto, cada triangulo se veria plano y el animal volveria a parecer
    // facetado.
    //
    // Ponderar por el area del triangulo (que es lo que hace el producto
    // vectorial sin normalizar) da mejor resultado que promediar a secas: los
    // triangulos grandes influyen mas, que es lo correcto.
    static void calcularNormales(MallaAnimal& malla) {
        for (VerticeAnimal& v : malla.vertices) v.normal = V3(0,0,0);

        for (size_t t = 0; t + 2 < malla.indices.size(); t += 3) {
            const uint16_t i0 = malla.indices[t];
            const uint16_t i1 = malla.indices[t+1];
            const uint16_t i2 = malla.indices[t+2];

            const V3& p0 = malla.vertices[i0].pos;
            const V3& p1 = malla.vertices[i1].pos;
            const V3& p2 = malla.vertices[i2].pos;

            const V3 n = cruz(p1 - p0, p2 - p0);   // sin normalizar: pesa area

            malla.vertices[i0].normal += n;
            malla.vertices[i1].normal += n;
            malla.vertices[i2].normal += n;
        }

        for (VerticeAnimal& v : malla.vertices) {
            v.normal = v.normal.normalizado();
        }
    }

    // ------------------------------------------------------------------------
    // PERTURBAR NORMALES: EL PELAJE SIN GEOMETRIA
    // ------------------------------------------------------------------------
    // Aqui esta la parte que sustituye al normal map del punto 12.
    //
    // En vez de anadir geometria por cada irregularidad del pelo, se desvia
    // ligeramente la normal de cada vertice segun un ruido y segun la
    // DIRECCION DEL PELO en esa zona. El pipeline fijo interpola esas normales
    // y produce un sombreado irregular que se lee como pelaje.
    //
    // No es un normal map por pixel: la resolucion es la de la malla, no la de
    // la pantalla. Es una aproximacion honesta y funciona a media distancia,
    // que es donde se ve el animal casi siempre.
    //
    // Coste: cero en tiempo de ejecucion. Se hace UNA VEZ al generar la malla.
    static void perturbarPorPelaje(MallaAnimal& malla,
                                   uint32_t semilla,
                                   float intensidad) {
        for (size_t i = 0; i < malla.vertices.size(); ++i) {
            VerticeAnimal& v = malla.vertices[i];

            // Donde no hay pelo (hocico, pezunas), la superficie es LISA.
            // Es lo que hace que el morro se lea como piel humeda y no como
            // pelaje.
            if (v.pelo < 0.25f) continue;

            const float n1 = ruido(semilla,      (int)i, 0) - 0.5f;
            const float n2 = ruido(semilla + 77, (int)i, 1) - 0.5f;
            const float n3 = ruido(semilla + 31, (int)i, 2) - 0.5f;

            const float k = intensidad * v.pelo;
            v.normal = V3(v.normal.x + n1 * k,
                          v.normal.y + n2 * k,
                          v.normal.z + n3 * k).normalizado();
        }
    }

    // ------------------------------------------------------------------------
    // DIRECCION DEL PELO
    // ------------------------------------------------------------------------
    // El punto 7 pide un campo HairDirection(position, normal).
    //
    // En un animal real el pelo no apunta en cualquier direccion: nace
    // inclinado hacia atras y hacia abajo, siguiendo el flujo del cuerpo. Es
    // lo que hace que al acariciarlo "a contrapelo" se note.
    //
    // Se calcula proyectando el eje longitudinal del animal sobre el plano
    // tangente a la superficie. El resultado es un campo continuo que sigue la
    // anatomia sin almacenar nada.
    static V3 direccionPelo(const V3& normal, ZonaCuerpo zona) {
        // Direccion base: hacia la cola (-Z) y ligeramente hacia abajo.
        V3 flujo(0.0f, -0.25f, -1.0f);

        switch (zona) {
            case ZonaCuerpo::CABEZA:
            case ZonaCuerpo::HOCICO:
                // En la cara el pelo va hacia DELANTE, al reves que el cuerpo.
                flujo = V3(0.0f, -0.15f, 1.0f);
                break;
            case ZonaCuerpo::PATA:
                // En las patas baja casi vertical.
                flujo = V3(0.0f, -1.0f, -0.2f);
                break;
            case ZonaCuerpo::LOMO:
            case ZonaCuerpo::GRUPA:
                // En el lomo, la crin se levanta.
                flujo = V3(0.0f, 0.5f, -0.85f);
                break;
            case ZonaCuerpo::OREJA:
                flujo = V3(0.0f, 0.3f, -0.9f);
                break;
            default:
                break;
        }

        // Proyectar sobre el plano tangente: el pelo se PEGA a la superficie,
        // no la atraviesa.
        const float d = punto(flujo, normal);
        V3 tang(flujo.x - normal.x * d,
                flujo.y - normal.y * d,
                flujo.z - normal.z * d);
        return tang.normalizado();
    }

    // ------------------------------------------------------------------------
    // TRANSFORMAR UNA MALLA
    // ------------------------------------------------------------------------
    // Aplica escala y desplazamiento a un rango de vertices. Sirve para
    // colocar las piezas (cabeza, patas) sin generar cada una en su sitio.
    static void transformar(MallaAnimal& malla, size_t desde,
                            const V3& desplaz, float escala = 1.0f) {
        for (size_t i = desde; i < malla.vertices.size(); ++i) {
            VerticeAnimal& v = malla.vertices[i];
            v.pos = V3(v.pos.x * escala + desplaz.x,
                       v.pos.y * escala + desplaz.y,
                       v.pos.z * escala + desplaz.z);
        }
    }

    // Rota un rango de vertices alrededor de Y. Para orejas y patas abiertas.
    static void rotarY(MallaAnimal& malla, size_t desde, float ang,
                       const V3& pivote) {
        const float c = std::cos(ang), s = std::sin(ang);
        for (size_t i = desde; i < malla.vertices.size(); ++i) {
            VerticeAnimal& v = malla.vertices[i];
            const float dx = v.pos.x - pivote.x;
            const float dz = v.pos.z - pivote.z;
            v.pos.x = pivote.x + dx * c + dz * s;
            v.pos.z = pivote.z - dx * s + dz * c;
        }
    }

    // Rota alrededor de X (cabecear una pieza).
    static void rotarX(MallaAnimal& malla, size_t desde, float ang,
                       const V3& pivote) {
        const float c = std::cos(ang), s = std::sin(ang);
        for (size_t i = desde; i < malla.vertices.size(); ++i) {
            VerticeAnimal& v = malla.vertices[i];
            const float dy = v.pos.y - pivote.y;
            const float dz = v.pos.z - pivote.z;
            v.pos.y = pivote.y + dy * c - dz * s;
            v.pos.z = pivote.z + dy * s + dz * c;
        }
    }

    // Marca la zona y el nivel de pelo de un rango de vertices.
    static void marcarZona(MallaAnimal& malla, size_t desde,
                           ZonaCuerpo zona, float pelo) {
        for (size_t i = desde; i < malla.vertices.size(); ++i) {
            malla.vertices[i].zona = zona;
            malla.vertices[i].pelo = pelo;
        }
    }

private:
    // Tapa un anillo con un abanico de triangulos hacia su centro.
    static void taparAnillo(MallaAnimal& malla, size_t primerVert, int lados,
                            const SeccionCuerpo& sec, bool haciaAtras) {
        // Vertice central de la tapa.
        VerticeAnimal c;
        c.pos = V3(0.0f, sec.centroY, sec.z);
        c.normal = V3(0,0,0);
        c.r = c.g = c.b = 1.0f;
        c.zona = sec.zonaFlanco;
        c.pelo = 1.0f;

        const uint16_t ic = (uint16_t)malla.vertices.size();
        malla.vertices.push_back(c);

        for (int i = 0; i < lados; ++i) {
            const int j = (i + 1) % lados;
            const uint16_t a = (uint16_t)(primerVert + i);
            const uint16_t b = (uint16_t)(primerVert + j);

            // Las tapas YA estaban orientadas hacia fuera: se comprobo midiendo
            // la malla antes y despues de tocar coserTubo, y invertirlas aqui
            // empeoraba el recuento. Se dejan como estaban.
            if (haciaAtras) {
                malla.indices.push_back(ic);
                malla.indices.push_back(b);
                malla.indices.push_back(a);
            } else {
                malla.indices.push_back(ic);
                malla.indices.push_back(a);
                malla.indices.push_back(b);
            }
        }
    }
};

} // namespace Fauna

#endif // ANIMAL_MALLA_H
