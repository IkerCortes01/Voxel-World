#pragma once
// ============================================================================
// ⭐⭐ LOS NIVELES CUBIERTOS POR UN BLOQUE ENTERO
// ============================================================================
// PEDIDO: "que realmente no lo reemplace, o sea se guarde dentro pero no se
// vea, y al romper el bloque completo se volvera a ver el nivel que fue
// cubierto".
//
// Un bloque entero puesto sobre un nivel parcial ocupa la celda entera (ver
// enteroQueSustituyeNivel en main.cpp), pero el nivel NO se borra: se apunta
// aqui, con el bloque que lo tapo. Mientras el bloque esta, el nivel no se ve
// ni choca con nada. Al romper ese bloque, la celda vuelve a ser el nivel.
//
// Es la misma idea que las celdas de capas (romper la de arriba deja ver las
// de debajo), pero con un registro aparte: un bloque entero ya ocupa los ocho
// octavos de la celda y no queda sitio en el ID para guardar nada mas. Y como
// con g_nivelesDelJugador, un registro aparte no cambia el formato de los
// chunks ni rompe los mundos guardados.
//
// POR QUE SE GUARDA TAMBIEN EL BLOQUE QUE LO TAPO: si el bloque sale de la
// celda por otro camino (cae, lo arrastra el agua, un arbol que se derrumba),
// la entrada queda vieja. Solo se destapa si lo que se rompe es EXACTAMENTE
// el bloque que lo cubrio; si no, la entrada se descarta y la celda queda
// vacia, como cualquier otra rotura. Equivocarse asi es el lado seguro: nunca
// aparece un nivel donde no lo hubo.
//
// Se guarda en niveles_cubiertos.dat del mundo, como nopales.dat.

#include "BlockType.h"
#include <cstdint>
#include <istream>
#include <map>
#include <ostream>
#include <tuple>

namespace NivelesCubiertos {

struct Celda {
    int x, y, z;
    bool operator<(const Celda& o) const {
        return std::tie(x, y, z) < std::tie(o.x, o.y, o.z);
    }
};

struct Cubierto {
    BlockType nivel;      // lo que habia en la celda (un nivel o celda de capas)
    BlockType tapa;       // el bloque entero que lo tapo
};

// Tope de seguridad, como MAX_NIVELES_JUGADOR: acotado aunque se tape medio
// mundo. Pasado el tope el nivel se pierde al taparlo, que es como era antes.
constexpr size_t MAX_CUBIERTOS = 200000;

class Registro {
public:
    // El bloque `tapa` acaba de ocupar la celda donde estaba `nivel`.
    void cubrir(int x, int y, int z, BlockType nivel, BlockType tapa) {
        if (nivel == BLOCK_AIR || tapa == BLOCK_AIR) return;
        const Celda c{x, y, z};
        if (celdas_.size() >= MAX_CUBIERTOS && celdas_.find(c) == celdas_.end()) return;
        celdas_[c] = {nivel, tapa};
    }

    // Se rompe `roto` en la celda. Devuelve lo que tiene que quedar en ella:
    // el nivel tapado si `roto` es el bloque que lo tapo, o BLOCK_AIR. La
    // entrada se borra siempre: una vez rota la celda ya no vale para nada.
    BlockType destapar(int x, int y, int z, BlockType roto) {
        auto it = celdas_.find(Celda{x, y, z});
        if (it == celdas_.end()) return BLOCK_AIR;
        const Cubierto c = it->second;
        celdas_.erase(it);
        return c.tapa == roto ? c.nivel : BLOCK_AIR;
    }

    bool tiene(int x, int y, int z) const { return celdas_.count(Celda{x, y, z}) != 0; }
    size_t size() const { return celdas_.size(); }
    void clear() { celdas_.clear(); }

    void guardar(std::ostream& f) const {
        const uint32_t n = (uint32_t)celdas_.size();
        f.write(reinterpret_cast<const char*>(&n), sizeof(n));
        for (const auto& [c, v] : celdas_) {
            const int32_t d[5] = {c.x, c.y, c.z, (int32_t)v.nivel, (int32_t)v.tapa};
            f.write(reinterpret_cast<const char*>(d), sizeof(d));
        }
    }

    // Lee lo que escribio `guardar`. Un archivo corto o roto deja lo que se
    // haya leido entero hasta ahi.
    void cargar(std::istream& f) {
        celdas_.clear();
        uint32_t n = 0;
        if (!f.read(reinterpret_cast<char*>(&n), sizeof(n)) || n > MAX_CUBIERTOS) return;
        for (uint32_t i = 0; i < n; ++i) {
            int32_t d[5];
            if (!f.read(reinterpret_cast<char*>(d), sizeof(d))) break;
            if (d[3] <= 0 || d[4] <= 0) continue;
            celdas_[Celda{d[0], d[1], d[2]}] = {(BlockType)d[3], (BlockType)d[4]};
        }
    }

private:
    std::map<Celda, Cubierto> celdas_;
};

} // namespace NivelesCubiertos
