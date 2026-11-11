#ifndef MUSICA_H
#define MUSICA_H

// ============================================================================
// LA MUSICA: PIEZAS AL AZAR, EN MOMENTOS AL AZAR
// ============================================================================
// PEDIDO: "anade estas musicas que aparezcan en momentos aleatorios y sin
// orden" (las de sounds/Music).
//
// Como en los juegos de mundo abierto, la musica no suena todo el rato: hay
// SILENCIOS largos (de 2 a 6 minutos; el primero mas corto, de 45 s a 2 min)
// y de pronto entra una pieza, que sube despacio (3 s), suena entera y se
// apaga sola. Luego otro silencio.
//
// SIN ORDEN pero sin repetir: se barajan todas las piezas, se tocan en ese
// orden al azar y, cuando se acaban, se vuelven a barajar (sin que la primera
// del mazo nuevo sea la ultima que sono). Asi no suena dos veces la misma
// seguida y todas salen.
//
// Sirve cualquier .mp3 o .wav que se deje en la carpeta: se leen al arrancar.
//
// LA REPRODUCCION va por MCI de Windows (winmm, que el juego ya usa): abre el
// mp3 con los codecs del sistema y deja subir y bajar el volumen. No hace falta
// decodificar nada aqui ni meter otra libreria.
//
// La logica (cuando suena que) esta separada de MCI para poder probarla.
// ============================================================================

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace Audio {

// ----------------------------------------------------------------------------
// LA LOGICA: el mazo barajado y los silencios
// ----------------------------------------------------------------------------
class SelectorMusica {
public:
    static constexpr float SILENCIO_MIN = 120.0f, SILENCIO_MAX = 360.0f;
    static constexpr float PRIMERO_MIN = 45.0f, PRIMERO_MAX = 120.0f;

    explicit SelectorMusica(uint32_t semilla = 0x2545F491u) : rng(semilla ? semilla : 1u) {}

    void poner(int piezas) {
        n = piezas;
        mazo.clear();
        ultima = -1;
        espera = rango(PRIMERO_MIN, PRIMERO_MAX);
    }
    int piezas() const { return n; }

    // Cuenta el silencio. Devuelve la pieza que toca empezar, o -1.
    int avanzar(float dt) {
        if (n <= 0) return -1;
        espera -= dt;
        if (espera > 0.0f) return -1;
        return siguiente();
    }
    // Al acabar una pieza: el silencio de despues.
    void acabo() { espera = rango(SILENCIO_MIN, SILENCIO_MAX); }
    float silencioRestante() const { return espera; }

private:
    int siguiente() {
        if (mazo.empty()) {
            for (int i = 0; i < n; ++i) mazo.push_back(i);
            for (int i = n - 1; i > 0; --i) {
                const int j = (int)(azar() * (float)(i + 1)) % (i + 1);
                std::swap(mazo[i], mazo[j]);
            }
            // La primera del mazo nuevo no repite la ultima que sono.
            if (n > 1 && mazo.back() == ultima) std::swap(mazo.back(), mazo.front());
        }
        const int p = mazo.back();
        mazo.pop_back();
        ultima = p;
        espera = 1e9f;   // hasta que acabe (ver acabo)
        return p;
    }
    float azar() {
        rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        return (float)(rng & 0xFFFFFFu) / 16777216.0f;
    }
    float rango(float a, float b) { return a + (b - a) * azar(); }

    uint32_t rng;
    int n = 0, ultima = -1;
    std::vector<int> mazo;
    float espera = 0.0f;
};

} // namespace Audio

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <filesystem>
#pragma comment(lib, "winmm.lib")

namespace Audio {

// ----------------------------------------------------------------------------
// LA REPRODUCCION (MCI)
// ----------------------------------------------------------------------------
class Musica {
public:
    // Lee la carpeta. `semilla`: para que cada partida tenga su orden.
    void iniciar(const std::string& carpeta, uint32_t semilla) {
        piezas.clear();
        std::error_code ec;
        if (std::filesystem::is_directory(carpeta, ec)) {
            for (const auto& e : std::filesystem::directory_iterator(carpeta, ec)) {
                std::wstring ext = e.path().extension().wstring();
                for (wchar_t& c : ext) c = (wchar_t)towlower(c);
                if (ext == L".mp3" || ext == L".wav") piezas.push_back(e.path().wstring());
            }
        }
        selector = SelectorMusica(semilla);
        selector.poner((int)piezas.size());
    }
    int cuantas() const { return (int)piezas.size(); }

    // Cada frame. `enPartida`: solo suena jugando. `volumen` 0..1.
    void actualizar(float dt, bool enPartida, float volumen) {
        if (dt < 0.0f) dt = 0.0f;
        if (dt > 0.25f) dt = 0.25f;
        if (!abierta) {
            if (!enPartida) return;
            const int p = selector.avanzar(dt);
            if (p >= 0) abrir(p);
            return;
        }
        // Sube al entrar (3 s) y baja al salir de la partida (1,5 s).
        if (enPartida) nivel = nivel + dt / 3.0f > 1.0f ? 1.0f : nivel + dt / 3.0f;
        else nivel -= dt / 1.5f;
        if (nivel <= 0.0f && !enPartida) { cerrar(); selector.acabo(); return; }
        // Los ultimos 4 s se apaga sola.
        consulta -= dt;
        if (consulta <= 0.0f) {
            consulta = 0.5f;
            const long pos = estado(L"position"), lon = estado(L"length");
            wchar_t modo[32] = {};
            mciSendStringW(L"status vw_musica mode", modo, 32, nullptr);
            if ((lon > 0 && pos >= lon - 50) || std::wstring(modo) == L"stopped") { cerrar(); selector.acabo(); return; }
            restante = lon > 0 ? (float)(lon - pos) / 1000.0f : 999.0f;
        } else {
            restante -= dt;
        }
        float k = nivel;
        if (restante < 4.0f) k *= restante > 0.0f ? restante / 4.0f : 0.0f;
        fijarVolumen(k * volumen);
    }

    void cerrar() {
        if (!abierta) return;
        mciSendStringW(L"stop vw_musica", nullptr, 0, nullptr);
        mciSendStringW(L"close vw_musica", nullptr, 0, nullptr);
        abierta = false;
    }
    // Sin MCI en el destructor: en la salida del proceso puede colgarse. Se\n    // cierra a mano (ver el final de main).\n    ~Musica() = default;

private:
    void abrir(int p) {
        const std::wstring ruta = piezas[(size_t)p];
        const std::wstring tipo = ruta.size() > 4 && (ruta.substr(ruta.size() - 4) == L".wav" || ruta.substr(ruta.size() - 4) == L".WAV")
                                      ? L"waveaudio" : L"mpegvideo";
        const std::wstring abrirCmd = L"open \"" + ruta + L"\" type " + tipo + L" alias vw_musica";
        if (mciSendStringW(abrirCmd.c_str(), nullptr, 0, nullptr) != 0) { selector.acabo(); return; }
        mciSendStringW(L"set vw_musica time format milliseconds", nullptr, 0, nullptr);
        abierta = true;
        nivel = 0.0f;
        consulta = 0.0f;
        restante = 999.0f;
        ultimoVolumen = -1;
        fijarVolumen(0.0f);
        mciSendStringW(L"play vw_musica", nullptr, 0, nullptr);
    }
    long estado(const wchar_t* que) {
        wchar_t r[64] = {};
        const std::wstring cmd = std::wstring(L"status vw_musica ") + que;
        if (mciSendStringW(cmd.c_str(), r, 64, nullptr) != 0) return -1;
        return wcstol(r, nullptr, 10);
    }
    void fijarVolumen(float v) {
        const int mil = (int)(v < 0.0f ? 0.0f : (v > 1.0f ? 1000.0f : v * 1000.0f));
        if (mil == ultimoVolumen) return;
        ultimoVolumen = mil;
        const std::wstring cmd = L"setaudio vw_musica volume to " + std::to_wstring(mil);
        mciSendStringW(cmd.c_str(), nullptr, 0, nullptr);
    }

    std::vector<std::wstring> piezas;
    SelectorMusica selector;
    bool abierta = false;
    float nivel = 0.0f, consulta = 0.0f, restante = 999.0f;
    int ultimoVolumen = -1;
};

} // namespace Audio
#endif // _WIN32

#endif // MUSICA_H
