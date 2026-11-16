// ============================================================================
// RELLENAR LOS HUECOS DE UNA TEXTURA, EN TIEMPO LINEAL
// ============================================================================
// Algunas texturas de item (el pedernal, la caliza, el polvo de tierra, las
// tunas) solo tienen dibujo en una parte de la imagen. Para que su canto en 3D
// no se vea agujereado, cada pixel vacio se pinta con el color del opaco que
// tiene mas cerca y la imagen queda maciza.
//
// ⚠️ EL BUG QUE ESTO SUSTITUYE: EL JUEGO NO ABRIA.
//
// La version anterior, por cada pixel vacio, recorria LA IMAGEN ENTERA buscando
// el opaco mas cercano. Su comentario decia "la imagen es de 16x16, asi que la
// busqueda directa cuesta nada". Con 16x16 son 65.000 pasos. Con una imagen de
// 1286x1223 -- las que se pusieron despues -- son 2,5 BILLONES: el juego se
// quedaba colgado al arrancar, antes de llegar al menu, y Windows lo daba por
// muerto ("no responde").
//
// Aqui se hace en una sola pasada: se parte de todos los pixeles opacos a la
// vez y se avanza hacia fuera, anillo a anillo (busqueda en anchura). Cada
// pixel se visita UNA vez y se queda con el color del opaco desde el que se
// llego a el, que es el mas cercano (en pasos de rey, que para pintar un canto
// es indistinguible del mas cercano en linea recta). 1,6 millones de pixeles
// son 1,6 millones de pasos: milesimas de segundo.
//
// Y por si acaso, lo que llega enorme se reduce antes (ver reducirSiEsEnorme):
// una textura de item no gana nada por encima de 256 px y cuesta 25 veces mas
// de cargar y de subir a la tarjeta.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace RellenoTextura {

// A partir de que alfa un pixel cuenta como dibujo.
constexpr int ALFA_OPACO = 250;
// El lado mayor que se le deja a una textura de item.
constexpr int LADO_MAXIMO = 256;

// Rellena los huecos de una imagen RGBA de w x h. Devuelve la imagen maciza
// (todo con alfa 255); si no hay ni un pixel opaco, la devuelve igual.
inline std::vector<unsigned char> rellenar(const unsigned char* px, int w, int h) {
    const size_t n = (size_t)w * (size_t)h;
    std::vector<unsigned char> out(px, px + n * 4);
    if (w <= 0 || h <= 0) return out;

    // `de[i]` = el pixel opaco del que toma el color el pixel i (o -1 aun).
    std::vector<int32_t> de(n, -1);
    std::vector<int32_t> cola;
    cola.reserve(n);
    for (size_t i = 0; i < n; ++i)
        if (px[i * 4 + 3] >= ALFA_OPACO) { de[i] = (int32_t)i; cola.push_back((int32_t)i); }
    if (cola.empty()) return out;           // imagen vacia entera: se deja igual

    static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
    static const int DY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    for (size_t k = 0; k < cola.size(); ++k) {
        const int32_t i = cola[k];
        const int x = (int)(i % w), y = (int)(i / w);
        for (int d = 0; d < 8; ++d) {
            const int nx = x + DX[d], ny = y + DY[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            const size_t j = (size_t)ny * (size_t)w + (size_t)nx;
            if (de[j] >= 0) continue;
            de[j] = de[(size_t)i];
            cola.push_back((int32_t)j);
        }
    }
    for (size_t i = 0; i < n; ++i) {
        if (px[i * 4 + 3] >= ALFA_OPACO) continue;
        const size_t s = (size_t)de[i] * 4;
        out[i * 4 + 0] = px[s + 0];
        out[i * 4 + 1] = px[s + 1];
        out[i * 4 + 2] = px[s + 2];
        out[i * 4 + 3] = 255;
    }
    return out;
}

// Cuanto hay que dividir una imagen de w x h para que su lado mayor no pase de
// LADO_MAXIMO. 1 = se queda como esta.
inline int divisorPara(int w, int h) {
    const int lado = w > h ? w : h;
    int d = 1;
    while (lado / d > LADO_MAXIMO) ++d;
    return d;
}

// Reduce una imagen RGBA dividiendo su lado por `d` (media de cada bloque de
// d x d, con el color pesado por su alfa para que lo transparente no manche).
inline std::vector<unsigned char> reducir(const unsigned char* px, int w, int h, int d, int& wOut, int& hOut) {
    if (d <= 1) { wOut = w; hOut = h; return std::vector<unsigned char>(px, px + (size_t)w * h * 4); }
    wOut = w / d; hOut = h / d;
    if (wOut < 1) wOut = 1;
    if (hOut < 1) hOut = 1;
    std::vector<unsigned char> out((size_t)wOut * hOut * 4);
    for (int y = 0; y < hOut; ++y)
        for (int x = 0; x < wOut; ++x) {
            uint64_t r = 0, g = 0, b = 0, a = 0;
            int cuenta = 0;
            for (int sy = y * d; sy < (y + 1) * d && sy < h; ++sy)
                for (int sx = x * d; sx < (x + 1) * d && sx < w; ++sx) {
                    const unsigned char* p = px + ((size_t)sy * w + sx) * 4;
                    r += (uint64_t)p[0] * p[3]; g += (uint64_t)p[1] * p[3]; b += (uint64_t)p[2] * p[3];
                    a += p[3];
                    ++cuenta;
                }
            unsigned char* o = out.data() + ((size_t)y * wOut + x) * 4;
            if (a > 0) { o[0] = (unsigned char)(r / a); o[1] = (unsigned char)(g / a); o[2] = (unsigned char)(b / a); }
            else { o[0] = o[1] = o[2] = 0; }
            o[3] = (unsigned char)(cuenta > 0 ? a / (uint64_t)cuenta : 0);
        }
    return out;
}

} // namespace RellenoTextura
