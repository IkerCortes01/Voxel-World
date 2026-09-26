#pragma once

#include <windows.h>
#include <GL/gl.h>
#include <string>
#include <iostream>

// ============================================================================
// EL CAMINO MODERNO PARA DIBUJAR EL TERRENO: ARRAY DE TEXTURAS + SHADER
// ============================================================================
// POR QUE EXISTE ESTE ARCHIVO
// ----------------------------------------------------------------------------
// El pase opaco costaba una llamada de dibujo POR TEXTURA Y POR CHUNK: unas
// once por chunk, ~400 por frame. Medido, cada una salia a 4,5 us de CPU en
// el driver, o sea ~1,8 ms de los 2,5 que costaba el frame entero.
//
// La solucion clasica es un ATLAS: meter las 73 texturas de bloque en una
// sola imagen grande y dibujar el chunk de una vez. AQUI NO SIRVE, y conviene
// dejarlo escrito para que nadie lo intente otra vez:
//
//     El greedy meshing fusiona caras vecinas en un solo rectangulo y emite
//     UV de 0 a 7 (o lo que mida la fusion), confiando en GL_REPEAT para
//     TESELAR la textura a lo largo del quad. Con un sub-rectangulo de atlas,
//     esas UV se salen de su casilla y barren el atlas entero: basura.
//
// La solucion que SI vale es un ARRAY DE TEXTURAS (GL_TEXTURE_2D_ARRAY): N
// capas de 16x16, cada textura en la suya. El repetido se aplica a s y t
// DENTRO de cada capa, asi que el greedy sigue funcionando exactamente igual,
// y un chunk entero se dibuja con tres llamadas (macizo, recortado, agua).
//
// Encaja porque TODAS las texturas que el mesher puede pedir son de 16x16:
// los bloques, las cinco de objetos que usan algunos bloques, y los cuadros
// de animacion del agua y del horno (que ya se cortan en trozos de 16x16 al
// cargarlos).
//
// Muestrear un array de texturas exige un shader, y por eso este archivo trae
// tambien el programa de terreno. El resto del motor (HUD, menus, la mano)
// sigue en fixed-function sin enterarse: el driver de esta maquina entrega un
// contexto 4.6 en modo COMPATIBILIDAD aunque el juego pida 2.1, asi que las
// dos cosas conviven.
//
// Si algo de esto falta en la maquina del jugador, `disponible()` devuelve
// false y el motor sigue por el camino de siempre. Nadie se queda sin jugar
// por no tener shaders.
// ============================================================================

namespace Render {

// Constantes que el gl.h de Windows (que se quedo en OpenGL 1.1) no declara.
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

typedef void (APIENTRY *PFN_glTexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
typedef void (APIENTRY *PFN_glTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void*);
typedef void (APIENTRY *PFN_glActiveTexture)(GLenum);
typedef GLuint (APIENTRY *PFN_glCreateShader)(GLenum);
typedef void (APIENTRY *PFN_glShaderSource)(GLuint, GLsizei, const char* const*, const GLint*);
typedef void (APIENTRY *PFN_glCompileShader)(GLuint);
typedef void (APIENTRY *PFN_glGetShaderiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef GLuint (APIENTRY *PFN_glCreateProgram)(void);
typedef void (APIENTRY *PFN_glAttachShader)(GLuint, GLuint);
typedef void (APIENTRY *PFN_glLinkProgram)(GLuint);
typedef void (APIENTRY *PFN_glGetProgramiv)(GLuint, GLenum, GLint*);
typedef void (APIENTRY *PFN_glGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, char*);
typedef void (APIENTRY *PFN_glUseProgram)(GLuint);
typedef void (APIENTRY *PFN_glDeleteShader)(GLuint);
typedef GLint (APIENTRY *PFN_glGetUniformLocation)(GLuint, const char*);
typedef void (APIENTRY *PFN_glUniform1i)(GLint, GLint);
typedef void (APIENTRY *PFN_glUniform1f)(GLint, GLfloat);
typedef void (APIENTRY *PFN_glUniform2f)(GLint, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glUniform3f)(GLint, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_glBindAttribLocation)(GLuint, GLuint, const char*);
typedef void (APIENTRY *PFN_glVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
typedef void (APIENTRY *PFN_glEnableVertexAttribArray)(GLuint);
typedef void (APIENTRY *PFN_glDisableVertexAttribArray)(GLuint);

// ⭐ LOS MIPMAPS, QUE ESTE MOTOR NO TENIA.
//
// Sin ellos, una textura mas grande que el area que ocupa en pantalla CENTELLEA
// al moverse: cada frame se muestrea un texel distinto de los muchos que caen
// dentro de un mismo pixel. Con 16x16 apenas se notaba; con una textura HD de
// 2048x2048 el suelo entero hierve.
//
// `glGenerateMipmap` es de GL 3.0, asi que va por wglGetProcAddress como todo
// lo demas de este header. Si la GPU no lo trae, se queda en nullptr y el motor
// sigue como siempre -- sin mipmaps, pero funcionando.
typedef void (APIENTRY *PFN_glGenerateMipmap)(GLenum);

inline PFN_glTexImage3D              pglTexImage3D = nullptr;
inline PFN_glTexSubImage3D           pglTexSubImage3D = nullptr;
inline PFN_glGenerateMipmap          pglGenerateMipmap = nullptr;
inline PFN_glActiveTexture           pglActiveTexture = nullptr;
inline PFN_glCreateShader            pglCreateShader = nullptr;
inline PFN_glShaderSource            pglShaderSource = nullptr;
inline PFN_glCompileShader           pglCompileShader = nullptr;
inline PFN_glGetShaderiv             pglGetShaderiv = nullptr;
inline PFN_glGetShaderInfoLog        pglGetShaderInfoLog = nullptr;
inline PFN_glCreateProgram           pglCreateProgram = nullptr;
inline PFN_glAttachShader            pglAttachShader = nullptr;
inline PFN_glLinkProgram             pglLinkProgram = nullptr;
inline PFN_glGetProgramiv            pglGetProgramiv = nullptr;
inline PFN_glGetProgramInfoLog       pglGetProgramInfoLog = nullptr;
inline PFN_glUseProgram              pglUseProgram = nullptr;
inline PFN_glDeleteShader            pglDeleteShader = nullptr;
inline PFN_glGetUniformLocation      pglGetUniformLocation = nullptr;
inline PFN_glUniform1i               pglUniform1i = nullptr;
inline PFN_glUniform1f               pglUniform1f = nullptr;
inline PFN_glUniform2f               pglUniform2f = nullptr;
inline PFN_glUniform3f               pglUniform3f = nullptr;
inline PFN_glBindAttribLocation      pglBindAttribLocation = nullptr;
inline PFN_glVertexAttribPointer     pglVertexAttribPointer = nullptr;
inline PFN_glEnableVertexAttribArray pglEnableVertexAttribArray = nullptr;
inline PFN_glDisableVertexAttribArray pglDisableVertexAttribArray = nullptr;

// ----------------------------------------------------------------------------
// EL VERTICE
// ----------------------------------------------------------------------------
// 24 bytes, los mismos que ocupaba el formato fijo GL_T2F_C4UB_V3F, pero el
// byte que alli era el alfa del color (siempre 1, nunca se leyo) pasa a ser
// la CAPA del array de texturas. O sea: se gana el array sin engordar ni un
// byte el trafico de vertices, que en una grafica integrada es lo que manda.
//
//     0  x,y,z    3 floats
//    12  u,v      2 floats   (pueden pasar de 1: el greedy tesela)
//    20  r,g,b    3 bytes    (la luz horneada de la cara)
//    23  capa     1 byte     (hasta 256 texturas distintas)
constexpr int ATRIB_POS   = 0;
constexpr int ATRIB_UV    = 1;
constexpr int ATRIB_COLOR = 2;
constexpr int ATRIB_CAPA  = 3;

inline const char* VERTEX_SHADER = R"(#version 130
in vec3  aPos;
in vec2  aUV;
in vec3  aColor;
in float aCapa;

out vec2 vUV;
out vec3 vColor;
flat out float vCapa;
out float vDist;

void main() {
    // Se usan las matrices de la tuberia fija: el resto del motor las sigue
    // moviendo con glTranslatef/glRotatef y asi no hay dos verdades sobre
    // donde esta la camara. En un contexto de compatibilidad siguen ahi.
    vec4 ojo = gl_ModelViewMatrix * vec4(aPos, 1.0);
    gl_Position = gl_ProjectionMatrix * ojo;
    vUV    = aUV;
    vColor = aColor;
    vCapa  = aCapa;
    vDist  = length(ojo.xyz);
}
)";

inline const char* FRAGMENT_SHADER = R"(#version 130
uniform sampler2DArray uTexturas;
uniform vec2  uScroll;        // desplazamiento del agua
uniform float uRecorte;       // >0.5: descartar lo casi transparente
uniform vec3  uNieblaColor;
uniform float uNieblaIni;
uniform float uNieblaFin;
// 0 = lineal, 1 = exponencial, 2 = exponencial al cuadrado, 3 = sin niebla.
// Son los mismos modos que GL_FOG_MODE, y se copian del estado de OpenGL
// para que el terreno se difumine EXACTAMENTE igual que el resto del motor.
uniform int   uNieblaModo;
uniform float uNieblaDensidad;

in vec2 vUV;
in vec3 vColor;
flat in float vCapa;
in float vDist;

out vec4 colorSalida;

void main() {
    // El repetido (GL_REPEAT) actua sobre s y t DENTRO de la capa, que es lo
    // que permite conservar el greedy meshing con UV de 0 a 7.
    vec4 t = texture(uTexturas, vec3(vUV + uScroll, vCapa));

    // Equivale al glAlphaFunc(GL_GREATER, 0.1) de la tuberia fija. Solo se
    // enciende para hierba y hojas: el terreno macizo se dibuja sin el para
    // no estorbar al early-Z.
    if (uRecorte > 0.5 && t.a <= 0.1) discard;

    vec3 c = t.rgb * vColor;

    // La misma niebla que aplicaba GL_FOG, en sus tres modos.
    float f = 0.0;
    if (uNieblaModo == 0) {
        f = (vDist - uNieblaIni) / max(uNieblaFin - uNieblaIni, 0.001);
    } else if (uNieblaModo == 1) {
        f = 1.0 - exp(-uNieblaDensidad * vDist);
    } else if (uNieblaModo == 2) {
        float d = uNieblaDensidad * vDist;
        f = 1.0 - exp(-d * d);
    }
    f = clamp(f, 0.0, 1.0);
    colorSalida = vec4(mix(c, uNieblaColor, f), t.a);
}
)";

// ----------------------------------------------------------------------------
// EL PROGRAMA DE TERRENO
// ----------------------------------------------------------------------------
struct ProgramaTerreno {
    GLuint id = 0;
    GLint uTexturas = -1, uScroll = -1, uRecorte = -1;
    GLint uNieblaColor = -1, uNieblaIni = -1, uNieblaFin = -1;
    GLint uNieblaModo = -1, uNieblaDensidad = -1;

    bool valido() const { return id != 0; }
};

// Carga los punteros. Devuelve false si falta algo: entonces el motor se
// queda en el camino de siempre y no pasa nada.
inline bool cargarFunciones() {
    auto get = [](const char* n) { return (void*)wglGetProcAddress(n); };
    pglTexImage3D    = (PFN_glTexImage3D)get("glTexImage3D");
    pglTexSubImage3D = (PFN_glTexSubImage3D)get("glTexSubImage3D");
    // ⚠️ NO entra en el `return` de abajo: los mipmaps son una MEJORA, no un
    // requisito. Sin ellos el motor dibuja igual (con centelleo en las
    // texturas grandes), asi que no puede tumbar el camino del shader.
    pglGenerateMipmap = (PFN_glGenerateMipmap)get("glGenerateMipmap");
    pglActiveTexture = (PFN_glActiveTexture)get("glActiveTexture");
    pglCreateShader  = (PFN_glCreateShader)get("glCreateShader");
    pglShaderSource  = (PFN_glShaderSource)get("glShaderSource");
    pglCompileShader = (PFN_glCompileShader)get("glCompileShader");
    pglGetShaderiv   = (PFN_glGetShaderiv)get("glGetShaderiv");
    pglGetShaderInfoLog = (PFN_glGetShaderInfoLog)get("glGetShaderInfoLog");
    pglCreateProgram = (PFN_glCreateProgram)get("glCreateProgram");
    pglAttachShader  = (PFN_glAttachShader)get("glAttachShader");
    pglLinkProgram   = (PFN_glLinkProgram)get("glLinkProgram");
    pglGetProgramiv  = (PFN_glGetProgramiv)get("glGetProgramiv");
    pglGetProgramInfoLog = (PFN_glGetProgramInfoLog)get("glGetProgramInfoLog");
    pglUseProgram    = (PFN_glUseProgram)get("glUseProgram");
    pglDeleteShader  = (PFN_glDeleteShader)get("glDeleteShader");
    pglGetUniformLocation = (PFN_glGetUniformLocation)get("glGetUniformLocation");
    pglUniform1i     = (PFN_glUniform1i)get("glUniform1i");
    pglUniform1f     = (PFN_glUniform1f)get("glUniform1f");
    pglUniform2f     = (PFN_glUniform2f)get("glUniform2f");
    pglUniform3f     = (PFN_glUniform3f)get("glUniform3f");
    pglBindAttribLocation = (PFN_glBindAttribLocation)get("glBindAttribLocation");
    pglVertexAttribPointer = (PFN_glVertexAttribPointer)get("glVertexAttribPointer");
    pglEnableVertexAttribArray = (PFN_glEnableVertexAttribArray)get("glEnableVertexAttribArray");
    pglDisableVertexAttribArray = (PFN_glDisableVertexAttribArray)get("glDisableVertexAttribArray");

    return pglTexImage3D && pglTexSubImage3D && pglActiveTexture &&
           pglCreateShader && pglShaderSource && pglCompileShader &&
           pglGetShaderiv && pglCreateProgram && pglAttachShader &&
           pglLinkProgram && pglGetProgramiv && pglUseProgram &&
           pglGetUniformLocation && pglUniform1i && pglUniform1f &&
           pglUniform2f && pglUniform3f && pglBindAttribLocation &&
           pglVertexAttribPointer && pglEnableVertexAttribArray &&
           pglDisableVertexAttribArray;
}

inline GLuint compilar(GLenum tipo, const char* fuente, const char* nombre) {
    GLuint s = pglCreateShader(tipo);
    if (!s) return 0;
    pglShaderSource(s, 1, &fuente, nullptr);
    pglCompileShader(s);
    GLint ok = 0;
    pglGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        if (pglGetShaderInfoLog) pglGetShaderInfoLog(s, sizeof(log) - 1, nullptr, log);
        std::cout << "[TERRENO] No compila el shader " << nombre << ": "
                  << log << std::endl;
        pglDeleteShader(s);
        return 0;
    }
    return s;
}

// Compila y enlaza. Devuelve un programa invalido si algo falla; el llamante
// se queda entonces con el camino de siempre.
inline ProgramaTerreno crearPrograma() {
    ProgramaTerreno p;
    GLuint vs = compilar(GL_VERTEX_SHADER, VERTEX_SHADER, "vertices");
    if (!vs) return p;
    GLuint fs = compilar(GL_FRAGMENT_SHADER, FRAGMENT_SHADER, "fragmentos");
    if (!fs) { pglDeleteShader(vs); return p; }

    GLuint prog = pglCreateProgram();
    if (!prog) { pglDeleteShader(vs); pglDeleteShader(fs); return p; }
    pglAttachShader(prog, vs);
    pglAttachShader(prog, fs);

    // Los indices se fijan ANTES de enlazar, que es la forma que funciona en
    // GLSL 130 (los `layout(location=)` son de 330 en adelante).
    pglBindAttribLocation(prog, ATRIB_POS,   "aPos");
    pglBindAttribLocation(prog, ATRIB_UV,    "aUV");
    pglBindAttribLocation(prog, ATRIB_COLOR, "aColor");
    pglBindAttribLocation(prog, ATRIB_CAPA,  "aCapa");

    pglLinkProgram(prog);
    pglDeleteShader(vs);
    pglDeleteShader(fs);

    GLint ok = 0;
    pglGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048] = {0};
        if (pglGetProgramInfoLog) pglGetProgramInfoLog(prog, sizeof(log) - 1, nullptr, log);
        std::cout << "[TERRENO] No enlaza el programa: " << log << std::endl;
        return p;
    }

    p.id           = prog;
    p.uTexturas    = pglGetUniformLocation(prog, "uTexturas");
    p.uScroll      = pglGetUniformLocation(prog, "uScroll");
    p.uRecorte     = pglGetUniformLocation(prog, "uRecorte");
    p.uNieblaColor = pglGetUniformLocation(prog, "uNieblaColor");
    p.uNieblaIni   = pglGetUniformLocation(prog, "uNieblaIni");
    p.uNieblaFin   = pglGetUniformLocation(prog, "uNieblaFin");
    p.uNieblaModo  = pglGetUniformLocation(prog, "uNieblaModo");
    p.uNieblaDensidad = pglGetUniformLocation(prog, "uNieblaDensidad");
    return p;
}

} // namespace Render
