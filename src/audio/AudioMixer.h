#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <cstdint>
#include <cmath>
#include <mutex>
#include <atomic>

#pragma comment(lib, "winmm.lib")

// ============================================================================
// AUDIO MIXER - Mezclador de software sobre waveOut (Windows)
// ============================================================================
// RESPONSABILIDAD UNICA: mezclar N sonidos simultaneos y entregarlos a la
// tarjeta de sonido.
//
// ----------------------------------------------------------------------------
// POR QUE NO PlaySoundA
// ----------------------------------------------------------------------------
// El motor usaba PlaySoundA, que tiene tres limitaciones fatales para pasos:
//   1. Solo UN sonido a la vez: cada paso corta el anterior, y ademas cortaria
//      la musica de fondo.
//   2. No permite variar el tono ni el volumen por reproduccion, asi que todos
//      los pasos suenan identicos y el resultado es robotico.
//   3. Lee del disco en cada llamada.
//
// Aqui se abre UN dispositivo waveOut y se mezclan las voces a mano en un
// buffer PCM. Sin dependencias externas (solo winmm, que ya viene con Windows).
//
// ----------------------------------------------------------------------------
// DISENO
// ----------------------------------------------------------------------------
//   - 22050 Hz, 16 bits, mono: suficiente para efectos y barato de mezclar.
//   - Doble buffer con callback: waveOut avisa cuando un bloque termina y se
//     rellena el siguiente. Latencia ~23 ms por bloque.
//   - Hasta MAX_VOICES sonidos solapados; si se agotan, se roba la voz mas
//     antigua (mejor que ignorar el sonido nuevo, que es el que el jugador
//     acaba de provocar).
//   - Cada voz tiene volumen y PASO DE LECTURA propio: leer la muestra a
//     velocidad != 1.0 cambia el tono, que es lo que da variacion natural
//     entre pisadas sin necesidad de N archivos distintos.
// ============================================================================

namespace Audio {

// Un sonido cargado en memoria (PCM mono 16-bit).
struct SoundBuffer {
    std::vector<int16_t> samples;
    int sampleRate = 22050;
};

class AudioMixer {
public:
    static constexpr int SAMPLE_RATE = 22050;
    static constexpr int MAX_VOICES  = 16;
    static constexpr int BLOCK_SAMPLES = 512;   // ~23 ms
    static constexpr int NUM_BLOCKS  = 4;

private:
    struct Voice {
        const SoundBuffer* buffer = nullptr;
        double position = 0.0;   // posicion de lectura (fraccionaria -> pitch)
        double step     = 1.0;   // 1.0 = tono original
        float  volume   = 1.0f;
        bool   active   = false;
        uint64_t startOrder = 0; // para robar la voz mas antigua
    };

    HWAVEOUT hWaveOut = nullptr;
    WAVEHDR  headers[NUM_BLOCKS] = {};
    std::vector<int16_t> blockData[NUM_BLOCKS];

    Voice voices[MAX_VOICES];
    std::mutex voiceMutex;
    std::atomic<uint64_t> orderCounter{0};

    std::atomic<bool> running{false};
    float masterVolume = 0.6f;

    // Callback de waveOut: se invoca al terminar cada bloque.
    static void CALLBACK waveCallback(HWAVEOUT, UINT uMsg, DWORD_PTR dwInstance,
                                      DWORD_PTR dwParam, DWORD_PTR) {
        if (uMsg != WOM_DONE) return;
        AudioMixer* self = reinterpret_cast<AudioMixer*>(dwInstance);
        if (!self || !self->running.load()) return;
        WAVEHDR* hdr = reinterpret_cast<WAVEHDR*>(dwParam);
        self->fillAndQueue(hdr);
    }

    // Rellena un bloque mezclando las voces activas y lo reencola.
    void fillAndQueue(WAVEHDR* hdr) {
        int16_t* out = reinterpret_cast<int16_t*>(hdr->lpData);

        // Acumulador en float para no saturar durante la suma.
        static thread_local std::vector<float> acc;
        acc.assign(BLOCK_SAMPLES, 0.0f);

        {
            std::lock_guard<std::mutex> lock(voiceMutex);
            for (int v = 0; v < MAX_VOICES; ++v) {
                Voice& voice = voices[v];
                if (!voice.active || !voice.buffer) continue;

                const std::vector<int16_t>& src = voice.buffer->samples;
                const size_t n = src.size();
                if (n == 0) { voice.active = false; continue; }

                for (int i = 0; i < BLOCK_SAMPLES; ++i) {
                    const size_t idx = (size_t)voice.position;
                    if (idx + 1 >= n) { voice.active = false; break; }

                    // Interpolacion lineal: sin ella, cambiar el tono
                    // introduce un aliasing muy audible ("crujido digital").
                    const double frac = voice.position - (double)idx;
                    const float s0 = (float)src[idx];
                    const float s1 = (float)src[idx + 1];
                    const float sample = (float)(s0 + (s1 - s0) * frac);

                    acc[i] += sample * voice.volume;
                    voice.position += voice.step;
                }
            }
        }

        // Volcado con clipping suave.
        const float mv = masterVolume;
        for (int i = 0; i < BLOCK_SAMPLES; ++i) {
            float s = acc[i] * mv;
            if (s >  32767.0f) s =  32767.0f;
            if (s < -32768.0f) s = -32768.0f;
            out[i] = (int16_t)s;
        }

        waveOutWrite(hWaveOut, hdr, sizeof(WAVEHDR));
    }

public:
    AudioMixer() = default;
    ~AudioMixer() { shutdown(); }

    bool init() {
        WAVEFORMATEX fmt = {};
        fmt.wFormatTag      = WAVE_FORMAT_PCM;
        fmt.nChannels       = 1;
        fmt.nSamplesPerSec  = SAMPLE_RATE;
        fmt.wBitsPerSample  = 16;
        fmt.nBlockAlign     = fmt.nChannels * fmt.wBitsPerSample / 8;
        fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
        fmt.cbSize          = 0;

        MMRESULT r = waveOutOpen(&hWaveOut, WAVE_MAPPER, &fmt,
                                 (DWORD_PTR)waveCallback,
                                 (DWORD_PTR)this,
                                 CALLBACK_FUNCTION);
        if (r != MMSYSERR_NOERROR) {
            hWaveOut = nullptr;
            return false;   // sin audio, pero el juego sigue
        }

        running.store(true);

        for (int b = 0; b < NUM_BLOCKS; ++b) {
            blockData[b].assign(BLOCK_SAMPLES, 0);
            headers[b] = {};
            headers[b].lpData         = reinterpret_cast<LPSTR>(blockData[b].data());
            headers[b].dwBufferLength = BLOCK_SAMPLES * sizeof(int16_t);
            waveOutPrepareHeader(hWaveOut, &headers[b], sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &headers[b], sizeof(WAVEHDR));
        }
        return true;
    }

    void shutdown() {
        if (!hWaveOut) return;
        running.store(false);

        waveOutReset(hWaveOut);
        for (int b = 0; b < NUM_BLOCKS; ++b) {
            waveOutUnprepareHeader(hWaveOut, &headers[b], sizeof(WAVEHDR));
        }
        waveOutClose(hWaveOut);
        hWaveOut = nullptr;
    }

    bool isReady() const { return hWaveOut != nullptr; }

    void setMasterVolume(float v) {
        masterVolume = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }

    // ------------------------------------------------------------------------
    // REPRODUCIR
    // ------------------------------------------------------------------------
    // pitch: 1.0 = tono original. 1.1 = 10% mas agudo y corto.
    void play(const SoundBuffer* buf, float volume = 1.0f, float pitch = 1.0f) {
        if (!hWaveOut || !buf || buf->samples.empty()) return;

        std::lock_guard<std::mutex> lock(voiceMutex);

        int slot = -1;
        for (int v = 0; v < MAX_VOICES; ++v) {
            if (!voices[v].active) { slot = v; break; }
        }
        if (slot < 0) {
            // Todas ocupadas: robar la mas antigua.
            uint64_t oldest = UINT64_MAX;
            for (int v = 0; v < MAX_VOICES; ++v) {
                if (voices[v].startOrder < oldest) {
                    oldest = voices[v].startOrder;
                    slot = v;
                }
            }
        }
        if (slot < 0) return;

        Voice& voice = voices[slot];
        voice.buffer     = buf;
        voice.position   = 0.0;
        voice.step       = (pitch > 0.01f) ? (double)pitch : 1.0;
        voice.volume     = volume;
        voice.active     = true;
        voice.startOrder = orderCounter.fetch_add(1);
    }

    void stopAll() {
        std::lock_guard<std::mutex> lock(voiceMutex);
        for (int v = 0; v < MAX_VOICES; ++v) voices[v].active = false;
    }
};

} // namespace Audio

#endif // AUDIO_MIXER_H
