#pragma once
// ============================================================================
// ⭐⭐ CELDAS JUNTAS: VARIAS COSAS EN LA MISMA CASILLA, CADA UNA EN SU HUECO
// ============================================================================
// PEDIDO: "haz que todos los bloques modificados en la mesa de refineria no se
// repelen de los niveles de bloques y se junten sin predominar uno; quiero que
// cada nivel tenga su propia casilla, o sea, que si pones uno del nivel minimo
// de bloques no cuenta que la casilla esta totalmente ocupada, y que deje
// espacio para demas niveles, sin limite de combinarse y apilarse".
//
// Hasta ahora una casilla del mundo era UNA cosa: un nivel de tierra de tres
// pixeles la "ocupaba" entera, y una pieza de la mesa de refineria (un piso,
// una pared, una escalera) no podia entrar en ella -- saltaba a la casilla de
// al lado y quedaba un hueco entre las dos.
//
// Aqui una casilla puede tener ADEMAS otras cosas, mientras no se pisen: lo
// que hay en el mundo (el bloque de siempre, que sigue donde estaba) y, en
// este registro, las que se le han JUNTADO. Cada una ocupa su hueco:
//
//   - una PIEZA de la mesa va con su forma, tal cual;
//   - un NIVEL es una losa de su alto, y si abajo ya hay algo se APILA encima;
//   - entra todo lo que quepa sin atravesar a lo que ya esta. No manda nadie:
//     ni la pieza borra al nivel ni el nivel a la pieza.
//
// Este archivo es la parte sin mundo: las cajas, buscar el hueco y el
// registro que se guarda con la partida (celdas_juntas.dat). Que caja tiene
// cada bloque, dibujarlas y romperlas lo hace main.cpp.
#include "BlockType.h"

#include <cstdint>
#include <istream>
#include <map>
#include <ostream>
#include <tuple>
#include <vector>

namespace CeldasJuntas {

// Una caja dentro de la casilla, de 0 a 1 en cada eje.
struct Caja { float x0, y0, z0, x1, y1, z1; };

// ¿Se atraviesan? Tocarse por una cara no cuenta: dos losas apiladas se tocan.
inline bool seCruzan(const Caja& a, const Caja& b) {
    const float E = 0.001f;
    return a.x0 < b.x1 - E && b.x0 < a.x1 - E &&
           a.y0 < b.y1 - E && b.y0 < a.y1 - E &&
           a.z0 < b.z1 - E && b.z0 < a.z1 - E;
}

// Buscar a que ALTURA cabe lo nuevo (`nuevas`, medidas desde su propio suelo,
// `alto` lo que mide) entre lo que ya hay (`puestas`). Se prueba en el suelo
// de la casilla y, si no, apoyado encima de cada cosa que ya esta, de abajo
// arriba: asi lo nuevo se APILA. Devuelve la altura, o un numero negativo si
// no cabe a ninguna sin salirse de la casilla o atravesar algo.
inline float buscarHueco(const Caja* puestas, int nPuestas, const Caja* nuevas, int nNuevas, float alto) {
    float candidatas[1 + 64];
    int nc = 0;
    candidatas[nc++] = 0.0f;
    for (int i = 0; i < nPuestas && nc < 64; ++i) {
        const float y = puestas[i].y1;
        // en orden de abajo arriba, sin repetir
        int k = nc;
        bool repetida = false;
        for (int j = 0; j < nc; ++j) if (candidatas[j] > y - 0.0005f && candidatas[j] < y + 0.0005f) repetida = true;
        if (repetida) continue;
        while (k > 0 && candidatas[k - 1] > y) { candidatas[k] = candidatas[k - 1]; --k; }
        candidatas[k] = y;
        ++nc;
    }
    for (int c = 0; c < nc; ++c) {
        const float y = candidatas[c];
        if (y + alto > 1.001f) continue;            // se saldria por arriba
        bool libre = true;
        for (int n = 0; n < nNuevas && libre; ++n) {
            Caja m = nuevas[n];
            m.y0 += y;
            m.y1 += y;
            for (int p = 0; p < nPuestas && libre; ++p)
                if (seCruzan(m, puestas[p])) libre = false;
        }
        if (libre) return y;
    }
    return -1.0f;
}

struct Celda {
    int x, y, z;
    bool operator<(const Celda& o) const { return std::tie(x, y, z) < std::tie(o.x, o.y, o.z); }
};

// Una cosa juntada a la casilla: que es y a que altura se apoyo.
struct Junto {
    BlockType tipo;
    float y0;
};

// "Sin limite": el unico tope es que quepa. Estos dos son seguros contra un
// archivo roto, no reglas del juego.
constexpr size_t MAX_POR_CELDA = 32;
constexpr size_t MAX_CELDAS = 200000;

class Registro {
public:
    // Lo juntado a una casilla (vacio si no hay nada).
    const std::vector<Junto>& de(int x, int y, int z) const {
        static const std::vector<Junto> NADA;
        auto it = celdas_.find(Celda{ x, y, z });
        return it == celdas_.end() ? NADA : it->second;
    }
    bool tiene(int x, int y, int z) const { return celdas_.count(Celda{ x, y, z }) != 0; }

    bool juntar(int x, int y, int z, BlockType tipo, float y0) {
        if (tipo == BLOCK_AIR) return false;
        const Celda c{ x, y, z };
        auto it = celdas_.find(c);
        if (it == celdas_.end()) {
            if (celdas_.size() >= MAX_CELDAS) return false;
            celdas_[c].push_back({ tipo, y0 });
            return true;
        }
        if (it->second.size() >= MAX_POR_CELDA) return false;
        it->second.push_back({ tipo, y0 });
        return true;
    }
    // Quita lo ULTIMO que se junto (lo de mas arriba del monton) y lo
    // devuelve. BLOCK_AIR si no habia nada.
    Junto quitarUltimo(int x, int y, int z) {
        auto it = celdas_.find(Celda{ x, y, z });
        if (it == celdas_.end() || it->second.empty()) return { BLOCK_AIR, 0.0f };
        const Junto j = it->second.back();
        it->second.pop_back();
        if (it->second.empty()) celdas_.erase(it);
        return j;
    }
    // Quita el PRIMERO (para cuando el bloque del mundo desaparece y uno de
    // los juntados pasa a ocupar su sitio).
    Junto quitarPrimero(int x, int y, int z) {
        auto it = celdas_.find(Celda{ x, y, z });
        if (it == celdas_.end() || it->second.empty()) return { BLOCK_AIR, 0.0f };
        const Junto j = it->second.front();
        it->second.erase(it->second.begin());
        if (it->second.empty()) celdas_.erase(it);
        return j;
    }
    void olvidar(int x, int y, int z) { celdas_.erase(Celda{ x, y, z }); }

    size_t celdas() const { return celdas_.size(); }
    void clear() { celdas_.clear(); }
    const std::map<Celda, std::vector<Junto>>& todas() const { return celdas_; }

    void guardar(std::ostream& f) const {
        const uint32_t n = (uint32_t)celdas_.size();
        f.write(reinterpret_cast<const char*>(&n), sizeof(n));
        for (const auto& [c, v] : celdas_) {
            const int32_t cab[4] = { c.x, c.y, c.z, (int32_t)v.size() };
            f.write(reinterpret_cast<const char*>(cab), sizeof(cab));
            for (const Junto& j : v) {
                const int32_t t = (int32_t)j.tipo;
                f.write(reinterpret_cast<const char*>(&t), sizeof(t));
                f.write(reinterpret_cast<const char*>(&j.y0), sizeof(float));
            }
        }
    }
    // Lee lo que escribio `guardar`. Un archivo corto o roto deja lo que se
    // haya leido entero hasta ahi.
    void cargar(std::istream& f) {
        celdas_.clear();
        uint32_t n = 0;
        if (!f.read(reinterpret_cast<char*>(&n), sizeof(n)) || n > MAX_CELDAS) return;
        for (uint32_t i = 0; i < n; ++i) {
            int32_t cab[4];
            if (!f.read(reinterpret_cast<char*>(cab), sizeof(cab))) break;
            if (cab[3] <= 0 || (size_t)cab[3] > MAX_POR_CELDA) break;
            std::vector<Junto> v;
            bool entera = true;
            for (int k = 0; k < cab[3]; ++k) {
                int32_t t = 0;
                float y0 = 0.0f;
                if (!f.read(reinterpret_cast<char*>(&t), sizeof(t)) ||
                    !f.read(reinterpret_cast<char*>(&y0), sizeof(float))) { entera = false; break; }
                if (t > 0 && y0 >= 0.0f && y0 < 1.0f) v.push_back({ (BlockType)t, y0 });
            }
            if (!entera) break;
            if (!v.empty()) celdas_[Celda{ cab[0], cab[1], cab[2] }] = v;
        }
    }

private:
    std::map<Celda, std::vector<Junto>> celdas_;
};

} // namespace CeldasJuntas
