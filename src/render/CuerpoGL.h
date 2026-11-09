#pragma once

#include "TerrenoGL.h"
#include "../player/MallaImportada.h"
#include <cstring>
#include <iostream>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR EN LA GPU (modelo importado)
// ============================================================================
// PEDIDO: "que el modelo se vea identico a lo que descargaste, mejorando la
// tecnologia de optimizacion y carga de poligonos y texturas".
//
// LA MALLA SE SUBE UNA VEZ. Los ~34.000 vertices (posicion, normal, uv,
// tangente, sus dos huesos y el peso) van a un buffer de la GPU al cargar, y
// solo se vuelven a subir si el avatar cambia la forma de la cara o del
// cuerpo. Cada frame solo viajan las matrices de los 51 huesos (2,4 KB): el
// vertex shader hace el skinning. La CPU no toca ni un vertice.
//
// LAS TEXTURAS, COMPRIMIDAS. La de color y el mapa de normales (2048) se suben
// en DXT1 si la GPU lo tiene (la HD 4000 si): 6 veces menos memoria que en
// RGBA, y con mipmaps, asi de lejos no centellean.
//
// EL SOMBREADO: la textura original, el mapa de normales (el relieve de la
// tela, las arrugas, el pelo) y la mascara de zonas, con la que el avatar
// tine la piel, el pelo, la camisa y el short. La luz es la misma regla que
// el resto del cuerpo (la del suelo a los pies y la del pecho arriba), y la
// niebla, la del terreno.
// ============================================================================

namespace CuerpoGL {

using namespace Render;

typedef void (APIENTRY *PFN_glGenBuffersC)(GLsizei, GLuint*);
typedef void (APIENTRY *PFN_glBindBufferC)(GLenum, GLuint);
typedef void (APIENTRY *PFN_glBufferDataC)(GLenum, ptrdiff_t, const void*, GLenum);
typedef void (APIENTRY *PFN_glUniform4fvC)(GLint, GLsizei, const GLfloat*);
typedef void (APIENTRY *PFN_glUniform3fvC)(GLint, GLsizei, const GLfloat*);

constexpr GLenum ARRAY = 0x8892, ELEMENTOS = 0x8893, ESTATICO = 0x88E4;
constexpr GLenum ARRAY_ACTUAL = 0x8894, ELEMENTOS_ACTUAL = 0x8895, PROGRAMA_ACTUAL = 0x8B8D;
constexpr GLenum DXT1 = 0x83F0, TEXTURA0 = 0x84C0, TEXTURA_ACTIVA = 0x84E0;

enum Atributo { A_POS, A_NOR, A_UV, A_TAN, A_HUESO, A_PESO };
enum Textura { T_COLOR, T_NORMAL, T_MASCARA, T_NUM };

inline const char* VS = R"(#version 130
in vec3 aPos;
in vec3 aNor;
in vec2 aUV;
in vec4 aTan;
in vec2 aHueso;
in float aPeso;
uniform vec4 uHueso[153];
uniform float uBase;
uniform float uAlto;
out vec2 vUV;
out vec3 vNor;
out vec4 vTan;
out float vAltura;
out vec3 vOjo;
vec3 mover(int b, vec4 p) {
    return vec3(dot(uHueso[b * 3], p), dot(uHueso[b * 3 + 1], p), dot(uHueso[b * 3 + 2], p));
}
void main() {
    int a = int(aHueso.x + 0.5), b = int(aHueso.y + 0.5);
    float w = aPeso;
    vec3 p = mover(a, vec4(aPos, 1.0)) * w + mover(b, vec4(aPos, 1.0)) * (1.0 - w);
    vec3 n = mover(a, vec4(aNor, 0.0)) * w + mover(b, vec4(aNor, 0.0)) * (1.0 - w);
    vec3 t = mover(a, vec4(aTan.xyz, 0.0)) * w + mover(b, vec4(aTan.xyz, 0.0)) * (1.0 - w);
    vec4 ojo = gl_ModelViewMatrix * vec4(p, 1.0);
    gl_Position = gl_ProjectionMatrix * ojo;
    vUV = aUV;
    vNor = n;
    vTan = vec4(t, aTan.w);
    vAltura = clamp((p.y - uBase) / uAlto, 0.0, 1.0);
    vOjo = ojo.xyz;
}
)";

inline const char* FS = R"(#version 130
uniform sampler2D uColor;
uniform sampler2D uNormal;
uniform sampler2D uMascara;
uniform vec3 uTinte[4];
uniform vec4 uMezcla;
uniform vec3 uLuzDir;
uniform float uLuzPies;
uniform float uLuzPecho;
uniform float uUsaNormal;
uniform int uNieblaModo;
uniform float uNieblaIni;
uniform float uNieblaFin;
uniform float uNieblaDensidad;
uniform vec3 uNieblaColor;
in vec2 vUV;
in vec3 vNor;
in vec4 vTan;
in float vAltura;
in vec3 vOjo;
out vec4 colorSalida;
void main() {
    vec3 c = texture(uColor, vUV).rgb;
    vec4 m = texture(uMascara, vUV);
    // Cada zona toma el color del avatar con la luminancia de la textura
    // (uTinte ya viene dividido por la luminancia media de la zona).
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(c, lum * uTinte[0], m.r * uMezcla.x);
    c = mix(c, lum * uTinte[1], m.g * uMezcla.y);
    c = mix(c, lum * uTinte[2], m.b * uMezcla.z);
    c = mix(c, lum * uTinte[3], m.a * uMezcla.w);
    vec3 N = normalize(vNor);
    if (uUsaNormal > 0.5) {
        vec3 T = normalize(vTan.xyz - N * dot(N, vTan.xyz));
        vec3 B = cross(N, T) * vTan.w;
        vec3 nm = texture(uNormal, vUV).xyz * 2.0 - 1.0;
        N = normalize(T * nm.x + B * nm.y + N * max(nm.z, 0.2));
    }
    float d = max(dot(N, uLuzDir), 0.0);
    float d2 = d * d;
    float luz = mix(uLuzPies, uLuzPecho, vAltura) * (0.58 + 0.40 * d + d2 * d2 * d2 * 0.06);
    c *= luz;
    float dist = length(vOjo), f = 0.0;
    if (uNieblaModo == 0) f = (dist - uNieblaIni) / max(uNieblaFin - uNieblaIni, 0.001);
    else if (uNieblaModo == 1) f = 1.0 - exp(-uNieblaDensidad * dist);
    else if (uNieblaModo == 2) { float k = uNieblaDensidad * dist; f = 1.0 - exp(-k * k); }
    colorSalida = vec4(mix(c, uNieblaColor, clamp(f, 0.0, 1.0)), 1.0);
}
)";

struct Recursos {
    bool probado = false, listo = false;
    GLuint prog = 0, vbo = 0, ibo[2] = { 0, 0 };
    GLsizei n[2] = { 0, 0 };
    GLuint tex[T_NUM] = {};
    int versionSubida = -1;
    GLint uHueso = -1, uBase = -1, uAlto = -1, uColor = -1, uNormal = -1, uMascara = -1, uTinte = -1;
    GLint uMezcla = -1;
    GLint uLuzDir = -1, uLuzPies = -1, uLuzPecho = -1, uUsaNormal = -1;
    GLint uNieblaModo = -1, uNieblaIni = -1, uNieblaFin = -1, uNieblaDensidad = -1, uNieblaColor = -1;
    PFN_glGenBuffersC genBuffers = nullptr;
    PFN_glBindBufferC bindBuffer = nullptr;
    PFN_glBufferDataC bufferData = nullptr;
    PFN_glUniform4fvC uniform4fv = nullptr;
    PFN_glUniform3fvC uniform3fv = nullptr;
    bool dxt1 = false;
};
inline Recursos& R() { static Recursos r; return r; }

// Compila el shader y carga lo que falta. Si algo falla, `listo` queda en
// false y el juego dibuja el modelo en la CPU (MallaImportada::deformar).
inline bool iniciar() {
    Recursos& r = R();
    if (r.probado) return r.listo;
    r.probado = true;
    if (!cargarFunciones()) return false;
    auto get = [](const char* n) { return (void*)wglGetProcAddress(n); };
    r.genBuffers = (PFN_glGenBuffersC)get("glGenBuffers");
    r.bindBuffer = (PFN_glBindBufferC)get("glBindBuffer");
    r.bufferData = (PFN_glBufferDataC)get("glBufferData");
    r.uniform4fv = (PFN_glUniform4fvC)get("glUniform4fv");
    r.uniform3fv = (PFN_glUniform3fvC)get("glUniform3fv");
    if (!r.genBuffers || !r.bindBuffer || !r.bufferData || !r.uniform4fv || !r.uniform3fv) return false;
    const char* ext = (const char*)glGetString(GL_EXTENSIONS);
    r.dxt1 = ext && std::strstr(ext, "GL_EXT_texture_compression_s3tc");
    const GLuint vs = compilar(GL_VERTEX_SHADER, VS, "cuerpo (vertices)");
    const GLuint fs = compilar(GL_FRAGMENT_SHADER, FS, "cuerpo (fragmentos)");
    if (!vs || !fs) return false;
    r.prog = pglCreateProgram();
    pglAttachShader(r.prog, vs);
    pglAttachShader(r.prog, fs);
    pglBindAttribLocation(r.prog, A_POS, "aPos");
    pglBindAttribLocation(r.prog, A_NOR, "aNor");
    pglBindAttribLocation(r.prog, A_UV, "aUV");
    pglBindAttribLocation(r.prog, A_TAN, "aTan");
    pglBindAttribLocation(r.prog, A_HUESO, "aHueso");
    pglBindAttribLocation(r.prog, A_PESO, "aPeso");
    pglLinkProgram(r.prog);
    GLint ok = 0;
    pglGetProgramiv(r.prog, GL_LINK_STATUS, &ok);
    pglDeleteShader(vs);
    pglDeleteShader(fs);
    if (!ok) {
        char log[2048] = {};
        if (pglGetProgramInfoLog) pglGetProgramInfoLog(r.prog, sizeof(log) - 1, nullptr, log);
        std::cout << "[CUERPO] No enlaza el shader: " << log << std::endl;
        return false;
    }
    auto U = [&](const char* n) { return pglGetUniformLocation(r.prog, n); };
    r.uHueso = U("uHueso"); r.uBase = U("uBase"); r.uAlto = U("uAlto");
    r.uColor = U("uColor"); r.uNormal = U("uNormal"); r.uMascara = U("uMascara"); r.uTinte = U("uTinte"); r.uMezcla = U("uMezcla");
    r.uLuzDir = U("uLuzDir"); r.uLuzPies = U("uLuzPies"); r.uLuzPecho = U("uLuzPecho"); r.uUsaNormal = U("uUsaNormal");
    r.uNieblaModo = U("uNieblaModo"); r.uNieblaIni = U("uNieblaIni"); r.uNieblaFin = U("uNieblaFin");
    r.uNieblaDensidad = U("uNieblaDensidad"); r.uNieblaColor = U("uNieblaColor");
    r.genBuffers(1, &r.vbo);
    r.genBuffers(2, r.ibo);
    glGenTextures(T_NUM, r.tex);
    r.listo = r.uHueso >= 0;
    std::cout << "[CUERPO] Shader del modelo importado listo" << (r.dxt1 ? " (texturas DXT1)" : "") << std::endl;
    return r.listo;
}

// Sube una textura RGB o RGBA con mipmaps (DXT1 si se pide y se puede).
inline void subirTextura(Textura cual, int w, int h, int canales, const unsigned char* px, bool comprimir) {
    Recursos& r = R();
    if (!r.listo || !px) return;
    glBindTexture(GL_TEXTURE_2D, r.tex[cual]);
    const GLenum fmt = canales == 4 ? GL_RGBA : GL_RGB;
    const GLint interno = (comprimir && r.dxt1 && canales == 3) ? (GLint)DXT1 : (GLint)fmt;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, interno, w, h, 0, fmt, GL_UNSIGNED_BYTE, px);
    if (pglGenerateMipmap) {
        pglGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Sube la malla (o la vuelve a subir si el avatar cambio su forma).
inline void subirMalla(const ModeloJugador::MallaImportada& m) {
    Recursos& r = R();
    if (!r.listo || m.formato != 2 || r.versionSubida == m.versionForma) return;
    const size_t n = m.base.size();
    std::vector<float> v(n * 15);
    for (size_t i = 0; i < n; ++i) {
        float* q = &v[i * 15];
        const ModeloJugador::VerticeJ& b = m.base[i];
        q[0] = b.p.x; q[1] = b.p.y; q[2] = b.p.z;
        q[3] = b.n.x; q[4] = b.n.y; q[5] = b.n.z;
        q[6] = m.uv[i * 2]; q[7] = m.uv[i * 2 + 1];
        for (int k = 0; k < 4; ++k) q[8 + k] = m.tangente[i * 4 + k];
        q[12] = (float)m.h0[i]; q[13] = (float)m.h1[i]; q[14] = m.w0[i];
    }
    GLint antes = 0, antesE = 0;
    glGetIntegerv(ARRAY_ACTUAL, &antes);
    glGetIntegerv(ELEMENTOS_ACTUAL, &antesE);
    r.bindBuffer(ARRAY, r.vbo);
    r.bufferData(ARRAY, (ptrdiff_t)(v.size() * sizeof(float)), v.data(), ESTATICO);
    if (r.versionSubida < 0 || r.n[0] != (GLsizei)m.indices.size()) {
        for (int k = 0; k < 2; ++k) {
            const std::vector<uint32_t>& idx = m.indicesDe(k == 0);
            r.bindBuffer(ELEMENTOS, r.ibo[k]);
            r.bufferData(ELEMENTOS, (ptrdiff_t)(idx.size() * 4), idx.data(), ESTATICO);
            r.n[k] = (GLsizei)idx.size();
        }
    }
    r.bindBuffer(ARRAY, (GLuint)antes);
    r.bindBuffer(ELEMENTOS, (GLuint)antesE);
    r.versionSubida = m.versionForma;
}

// La luz: hacia donde (en el espacio del personaje), cuanta a los pies y al
// pecho, y desde que altura se mide.
struct Luz {
    float dir[3] = { 0.35f, 0.90f, 0.25f };
    float pies = 1.0f, pecho = 1.0f;
    float base = 0.0f, alto = 1.62f;
    bool niebla = true;
};

// Dibuja el modelo con el esqueleto `s`. La matriz de modelo (el personaje
// en el mundo) ya tiene que estar en GL_MODELVIEW.
inline bool dibujar(const ModeloJugador::MallaImportada& m, const ModeloJugador::Esqueleto& s, bool conCabeza,
                    const Luz& luz) {
    Recursos& r = R();
    if (!r.listo || m.formato != 2) return false;
    subirMalla(m);
    if (r.versionSubida != m.versionForma) return false;
    float filas[ModeloJugador::NUM_HUESOS * 12];
    m.matrices(s, filas);

    GLint progAntes = 0, arrayAntes = 0, elemAntes = 0, texActiva = 0;
    glGetIntegerv(PROGRAMA_ACTUAL, &progAntes);
    glGetIntegerv(ARRAY_ACTUAL, &arrayAntes);
    glGetIntegerv(ELEMENTOS_ACTUAL, &elemAntes);
    glGetIntegerv(TEXTURA_ACTIVA, &texActiva);

    // Los arrays de la tuberia fija que deje encendidos el terreno no deben
    // leer de nuestro buffer: fuera mientras se dibuja, y luego como estaban.
    const GLenum FIJOS[4] = { GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_TEXTURE_COORD_ARRAY, GL_NORMAL_ARRAY };
    GLboolean fijo[4];
    for (int i = 0; i < 4; ++i) { fijo[i] = glIsEnabled(FIJOS[i]); if (fijo[i]) glDisableClientState(FIJOS[i]); }

    pglUseProgram(r.prog);
    r.uniform4fv(r.uHueso, ModeloJugador::NUM_HUESOS * 3, filas);
    pglUniform1f(r.uBase, luz.base);
    pglUniform1f(r.uAlto, luz.alto);
    pglUniform3f(r.uLuzDir, luz.dir[0], luz.dir[1], luz.dir[2]);
    pglUniform1f(r.uLuzPies, luz.pies);
    pglUniform1f(r.uLuzPecho, luz.pecho);
    pglUniform1f(r.uUsaNormal, 1.0f);
    const ModeloJugador::AjustesMalla& a = m.ajustes;
    const float* pedido[4] = { a.piel, a.pelo, a.camisa, a.shortC };
    float tintes[12];
    for (int k = 0; k < 4; ++k) {
        const float* ref = m.colorZona(k);
        const float lum = std::fmax(0.02f, ref[0] * 0.299f + ref[1] * 0.587f + ref[2] * 0.114f);
        for (int c = 0; c < 3; ++c) tintes[k * 3 + c] = pedido[k][c] / lum;
    }
    r.uniform3fv(r.uTinte, 4, tintes);
    r.uniform4fv(r.uMezcla, 1, a.mezcla);
    // La niebla, copiada del estado de OpenGL como hace el terreno.
    int modo = 3;
    if (luz.niebla && glIsEnabled(GL_FOG)) {
        GLint fm = GL_LINEAR;
        glGetIntegerv(GL_FOG_MODE, &fm);
        modo = fm == GL_EXP ? 1 : (fm == GL_EXP2 ? 2 : 0);
    }
    GLfloat ini = 0, fin = 1, dens = 0, col[4] = {};
    glGetFloatv(GL_FOG_START, &ini);
    glGetFloatv(GL_FOG_END, &fin);
    glGetFloatv(GL_FOG_DENSITY, &dens);
    glGetFloatv(GL_FOG_COLOR, col);
    pglUniform1i(r.uNieblaModo, modo);
    pglUniform1f(r.uNieblaIni, ini);
    pglUniform1f(r.uNieblaFin, fin);
    pglUniform1f(r.uNieblaDensidad, dens);
    pglUniform3f(r.uNieblaColor, col[0], col[1], col[2]);
    for (int t = 0; t < T_NUM; ++t) {
        pglActiveTexture(TEXTURA0 + t);
        glBindTexture(GL_TEXTURE_2D, r.tex[t]);
    }
    pglUniform1i(r.uColor, T_COLOR);
    pglUniform1i(r.uNormal, T_NORMAL);
    pglUniform1i(r.uMascara, T_MASCARA);

    r.bindBuffer(ARRAY, r.vbo);
    const GLsizei paso = 15 * sizeof(float);
    auto atr = [&](int i, int n, int desde) {
        pglEnableVertexAttribArray(i);
        pglVertexAttribPointer(i, n, GL_FLOAT, GL_FALSE, paso, (const void*)(size_t)(desde * sizeof(float)));
    };
    atr(A_POS, 3, 0); atr(A_NOR, 3, 3); atr(A_UV, 2, 6); atr(A_TAN, 4, 8); atr(A_HUESO, 2, 12); atr(A_PESO, 1, 14);
    const int k = conCabeza ? 0 : 1;
    r.bindBuffer(ELEMENTOS, r.ibo[k]);
    glDrawElements(GL_TRIANGLES, r.n[k], GL_UNSIGNED_INT, nullptr);
    for (int i = A_POS; i <= A_PESO; ++i) pglDisableVertexAttribArray(i);

    for (int t = T_NUM - 1; t >= 0; --t) {
        pglActiveTexture(TEXTURA0 + t);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    pglActiveTexture((GLenum)texActiva);
    r.bindBuffer(ARRAY, (GLuint)arrayAntes);
    r.bindBuffer(ELEMENTOS, (GLuint)elemAntes);
    pglUseProgram((GLuint)progAntes);
    for (int i = 0; i < 4; ++i) if (fijo[i]) glEnableClientState(FIJOS[i]);
    return true;
}

} // namespace CuerpoGL
