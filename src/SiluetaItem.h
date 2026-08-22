#pragma once
// ============================================================================
// SILUETA DE ITEM: modelo 3D real a partir del PNG
// ============================================================================
// Convierte la textura de un item en un modelo con volumen de verdad, del
// estilo de los items de Minecraft: la cara de delante, la de detras y un
// CANTO que las une siguiendo el contorno EXACTO del dibujo.
//
// POR QUE NO VALIA LO DE ANTES
// ----------------------------
// El camino 3D que ya existia sacaba el canto de cuatro tiras pegadas al
// BORDE de la textura. Eso funciona con un item que llena su cuadro (una
// piedra, un trozo de mineral), pero no con uno fino y diagonal.
//
// El palo ocupa el 19.5% de su textura y sus cuatro esquinas estan vacias.
// De los 64 pixeles del borde solo 5 son opacos, asi que esas tiras no
// dibujarian el canto del palo: dibujarian cinco astillas sueltas flotando
// donde no hay nada, y el palo en si quedaria como dos laminas planas con el
// hueco a la vista por el costado. Justo "perder la forma" y "tener partes
// invisibles".
//
// COMO SE HACE AQUI
// -----------------
// Se mira el alpha del PNG y se marca que pixeles son opacos. El canto se
// levanta SOLO en las aristas donde un pixel opaco toca uno transparente (o
// el filo de la imagen): es el contorno real de la figura, del derecho y del
// reves. El resultado es un solido cerrado — se mire desde donde se mire hay
// superficie, nunca un agujero.
//
// Las coordenadas de textura del canto se toman del pixel opaco al que
// pertenece la arista, asi que el color del costado es el del propio dibujo:
// no se inventa ni se recolorea nada.
//
// Se calcula UNA vez por textura y se guarda. En el dibujado solo se recorren
// vertices ya listos.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace SiluetaItem {

// Un vertice del canto: posicion en el cuadro [-1,1] y su coordenada UV.
struct Vertice {
    float x, y, z;
    float u, v;
};

struct Modelo {
    std::vector<Vertice> canto;   // quads (4 en 4) del contorno
    bool valido = false;
    int  ancho = 0, alto = 0;
    int  pixelesOpacos = 0;
};

// Decodifica solo lo necesario para saber que pixeles son opacos.
// Devuelve true y rellena 'opaco' (w*h) si se pudo leer.
//
// Usa stb_image, que ya viene con el proyecto. Se pide RGBA siempre para que
// el canal alpha exista aunque el PNG no lo traiga (en ese caso sale 255 y la
// silueta es el cuadro entero, que es lo correcto para un item macizo).
//
// OJO: aqui NO se voltea la imagen. El resto del motor carga con
// stbi_set_flip_vertically_on_load(true), asi que se guarda el valor, se pone
// a false y se restaura: si no, la silueta saldria del reves respecto a las
// UV que usa el dibujado.
inline bool leerAlpha(const char* ruta, std::vector<unsigned char>& opaco,
                      int& w, int& h, unsigned char umbral = 128) {
    if (!ruta) return false;
    int canales = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* datos = stbi_load(ruta, &w, &h, &canales, 4);
    stbi_set_flip_vertically_on_load(true);   // dejarlo como estaba
    if (!datos) return false;

    opaco.assign((size_t)w * h, 0);
    for (size_t i = 0; i < (size_t)w * h; ++i)
        opaco[i] = (datos[i * 4 + 3] >= umbral) ? 1 : 0;

    stbi_image_free(datos);
    return true;
}

// Construye el modelo del contorno. 'grosor' es el semi-espesor en el
// espacio del item ([-1,1] cubre la textura entera).
inline Modelo construir(const char* ruta, float grosor) {
    Modelo m;
    std::vector<unsigned char> opaco;
    int w = 0, h = 0;
    if (!leerAlpha(ruta, opaco, w, h)) return m;

    m.ancho = w; m.alto = h;

    auto esOpaco = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= w || y >= h) return false;  // fuera = vacio
        return opaco[(size_t)y * w + x] != 0;
    };

    // De pixel a coordenada del item. La textura se mapea a [-1,1] y la fila 0
    // del PNG es la de ARRIBA, de ahi que la Y se invierta.
    auto px = [&](int x) { return -1.0f + 2.0f * (float)x / (float)w; };
    auto py = [&](int y) { return  1.0f - 2.0f * (float)y / (float)h; };
    auto uu = [&](int x) { return (float)x / (float)w; };
    auto vv = [&](int y) { return (float)y / (float)h; };

    const float g = grosor;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!esOpaco(x, y)) continue;
            ++m.pixelesOpacos;

            // UV del centro del pixel: el color del canto sale de ESTE pixel.
            const float cu = uu(x) + 0.5f / w;
            const float cv = vv(y) + 0.5f / h;

            const float x0 = px(x),     x1 = px(x + 1);
            const float y0 = py(y),     y1 = py(y + 1);

            // Una tira de canto por cada lado que da al vacio.
            // Izquierda
            if (!esOpaco(x - 1, y)) {
                m.canto.push_back({x0, y0, -g, cu, cv});
                m.canto.push_back({x0, y1, -g, cu, cv});
                m.canto.push_back({x0, y1,  g, cu, cv});
                m.canto.push_back({x0, y0,  g, cu, cv});
            }
            // Derecha
            if (!esOpaco(x + 1, y)) {
                m.canto.push_back({x1, y0,  g, cu, cv});
                m.canto.push_back({x1, y1,  g, cu, cv});
                m.canto.push_back({x1, y1, -g, cu, cv});
                m.canto.push_back({x1, y0, -g, cu, cv});
            }
            // Arriba
            if (!esOpaco(x, y - 1)) {
                m.canto.push_back({x0, y0,  g, cu, cv});
                m.canto.push_back({x1, y0,  g, cu, cv});
                m.canto.push_back({x1, y0, -g, cu, cv});
                m.canto.push_back({x0, y0, -g, cu, cv});
            }
            // Abajo
            if (!esOpaco(x, y + 1)) {
                m.canto.push_back({x0, y1, -g, cu, cv});
                m.canto.push_back({x1, y1, -g, cu, cv});
                m.canto.push_back({x1, y1,  g, cu, cv});
                m.canto.push_back({x0, y1,  g, cu, cv});
            }
        }
    }

    m.valido = (m.pixelesOpacos > 0);
    return m;
}

// Cache por ruta: el analisis se hace una sola vez.
inline const Modelo& obtener(const std::string& ruta, float grosor) {
    static std::unordered_map<std::string, Modelo> cache;
    auto it = cache.find(ruta);
    if (it != cache.end()) return it->second;
    auto res = cache.emplace(ruta, construir(ruta.c_str(), grosor));
    return res.first->second;
}

} // namespace SiluetaItem
