// ============================================================================
// EXTRAER LOS CUADROS DE UN VIDEO A UNA TIRA PNG
// ============================================================================
// Lo usa el corazon del HUD (resourcepacks/HUD/corazon.mp4): el juego no
// reproduce video, asi que se sacan N cuadros del ciclo, se reducen y se
// ponen en fila en una sola imagen. El juego los recorre a la velocidad del
// pulso.
//
//     extraer_video entrada.mp4 salida.png lado cuadros
//
// Usa Media Foundation (viene con Windows): decodifica el mp4 con los codecs
// del sistema a RGB32. Sin librerias externas.
// ============================================================================
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/glfw/deps/stb_image_write.h"

int main(int argc, char** argv) {
    if (argc < 5) { std::printf("uso: extraer_video entrada.mp4 salida.png lado cuadros\n"); return 1; }
    const int lado = std::atoi(argv[3]), quiero = std::atoi(argv[4]);
    std::wstring ruta(argv[1], argv[1] + std::strlen(argv[1]));
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) || FAILED(MFStartup(MF_VERSION))) return 1;
    IMFAttributes* at = nullptr;
    MFCreateAttributes(&at, 1);
    at->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    IMFSourceReader* lector = nullptr;
    if (FAILED(MFCreateSourceReaderFromURL(ruta.c_str(), at, &lector))) { std::printf("no abre el video\n"); return 1; }
    IMFMediaType* tipo = nullptr;
    MFCreateMediaType(&tipo);
    tipo->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    tipo->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(lector->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, tipo))) { std::printf("sin RGB32\n"); return 1; }
    IMFMediaType* actual = nullptr;
    lector->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &actual);
    UINT32 w = 0, h = 0;
    MFGetAttributeSize(actual, MF_MT_FRAME_SIZE, &w, &h);
    UINT32 fn = 0, fd = 1;
    MFGetAttributeRatio(actual, MF_MT_FRAME_RATE, &fn, &fd);
    LONG paso = 0;
    if (FAILED(actual->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&paso))) paso = (LONG)w * 4;
    std::printf("video %ux%u a %.2f fps\n", w, h, fd ? (double)fn / fd : 0.0);

    // Todos los cuadros, ya reducidos al lado pedido (recortando a cuadrado).
    std::vector<std::vector<unsigned char>> cuadros;
    for (;;) {
        DWORD flags = 0;
        IMFSample* m = nullptr;
        if (FAILED(lector->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, &flags, nullptr, &m))) break;
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) { if (m) m->Release(); break; }
        if (!m) continue;
        IMFMediaBuffer* b = nullptr;
        m->ConvertToContiguousBuffer(&b);
        BYTE* d = nullptr;
        DWORD len = 0;
        b->Lock(&d, nullptr, &len);
        const int c = (int)(w < h ? w : h), x0 = ((int)w - c) / 2, y0 = ((int)h - c) / 2;
        std::vector<unsigned char> img((size_t)lado * lado * 3);
        for (int y = 0; y < lado; ++y)
            for (int x = 0; x < lado; ++x) {
                // Media de la caja de pixeles que cae en este (sin aliasing).
                const int sx0 = x0 + x * c / lado, sx1 = x0 + (x + 1) * c / lado;
                const int sy0 = y0 + y * c / lado, sy1 = y0 + (y + 1) * c / lado;
                unsigned acc[3] = {}, n = 0;
                for (int sy = sy0; sy < sy1; ++sy) {
                    const BYTE* fila = paso >= 0 ? d + (size_t)sy * paso : d + (size_t)(h - 1 - sy) * (-paso);
                    for (int sx = sx0; sx < sx1; ++sx) {
                        const BYTE* px = fila + (size_t)sx * 4;   // BGRA
                        acc[0] += px[2]; acc[1] += px[1]; acc[2] += px[0]; ++n;
                    }
                }
                for (int k = 0; k < 3; ++k) img[((size_t)y * lado + x) * 3 + k] = (unsigned char)(n ? acc[k] / n : 0);
            }
        b->Unlock();
        b->Release();
        m->Release();
        cuadros.push_back(std::move(img));
    }
    std::printf("%zu cuadros leidos\n", cuadros.size());
    if (cuadros.empty()) return 1;
    const int n = quiero < (int)cuadros.size() ? quiero : (int)cuadros.size();
    std::vector<unsigned char> tira((size_t)lado * n * lado * 3);
    for (int k = 0; k < n; ++k) {
        const auto& src = cuadros[(size_t)k * cuadros.size() / n];
        for (int y = 0; y < lado; ++y)
            std::memcpy(&tira[((size_t)y * lado * n + (size_t)k * lado) * 3], &src[(size_t)y * lado * 3], (size_t)lado * 3);
    }
    stbi_write_png(argv[2], lado * n, lado, 3, tira.data(), lado * n * 3);
    std::printf("tira de %d cuadros de %dx%d -> %s\n", n, lado, lado, argv[2]);
    MFShutdown();
    return 0;
}
