#pragma once
// ============================================================================
// EL DIBUJO DE LAS NUBES: IMPOSTORES DE ESFERA
// ============================================================================
// Cada bola de Nubes.h es un cuadro de cuatro vertices que mira a la camara.
// El shader de fragmentos hace el resto: dentro del circulo inscrito calcula
// la normal de la esfera (x, y, sqrt(1 - x2 - y2)) y la ilumina.
//
//   LUZ DEL SOL   envuelta (la mitad oscura no se queda negra: una nube deja
//                 pasar luz), del color de luzSol: blanca a mediodia, dorada
//                 al atardecer, casi nada de noche.
//   CIELO         lo que mira arriba recibe el azul del cenit; lo que mira
//                 abajo, el del horizonte, mas apagado. Eso es lo que da a la
//                 base del cumulo su gris azulado.
//   CONTRALUZ     mirando hacia el sol los bordes se encienden: el "borde de
//                 plata" de las nubes de verdad.
//   LEJANIA       las del horizonte se funden con su color.
//   BORDE SUAVE   la opacidad cae hacia el borde del circulo, asi que las bolas
//                 que se solapan se funden en una sola nube.
//   DESHILACHADO  dos octavas de un ruido de valor (cuatro senos por octava)
//                 comen el borde de cada bola de forma irregular y abollan su
//                 normal: el contorno deja de ser un circulo y la superficie
//                 tiene "coliflor". Cada bola lleva su semilla, asi que no hay
//                 dos iguales.
//
// Sin profundidad, como el cielo: van al fondo y el terreno las tapa. Si el
// driver no tiene GLSL, `listo()` da false y simplemente no hay nubes.
// ============================================================================

#include "TerrenoGL.h"
#include "Nubes.h"

namespace Render {

struct DibujoNubes {
    GLuint prog = 0;
    GLint uSol = -1, uArriba = -1, uLuz = -1, uCielo = -1, uSuelo = -1, uHorizonte = -1;
    bool intentado = false, ok = false;

    bool listo() {
        if (intentado) return ok;
        intentado = true;
        if (!pglCreateProgram || !pglUseProgram || !pglUniform3f) return false;
        static const char* VS =
            "#version 120\n"
            "varying vec2 vUV;\n"
            "varying vec4 vDatos;\n"   // sombra, lejos, semilla, alfa
            "void main() {\n"
            "    vUV = gl_MultiTexCoord0.xy;\n"
            "    vDatos = gl_Color;\n"
            "    vec4 c = gl_ModelViewMatrix * gl_Vertex;\n"
            "    c.xy += gl_MultiTexCoord0.xy * gl_MultiTexCoord0.z;\n"
            "    gl_Position = gl_ProjectionMatrix * c;\n"
            "}\n";
        static const char* FS =
            "#version 120\n"
            "uniform vec3 uSol, uArriba, uLuz, uCielo, uSuelo, uHorizonte;\n"
            "varying vec2 vUV;\n"
            "varying vec4 vDatos;\n"
            "float azar(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }\n"
            "float ruido(vec2 p) {\n"
            "    vec2 i = floor(p), f = fract(p);\n"
            "    f = f * f * (3.0 - 2.0 * f);\n"
            "    return mix(mix(azar(i), azar(i + vec2(1.0, 0.0)), f.x),\n"
            "               mix(azar(i + vec2(0.0, 1.0)), azar(i + vec2(1.0, 1.0)), f.x), f.y);\n"
            "}\n"
            "void main() {\n"
            "    float r2 = dot(vUV, vUV);\n"
            "    if (r2 >= 1.0) discard;\n"
            "    vec2 q = vUV + vDatos.z * 37.0;\n"
            "    float nz = ruido(q * 2.6) * 0.65 + ruido(q * 5.8 + 3.1) * 0.35;\n"
            "    float cuerpo = (1.0 - r2) - (nz - 0.5) * 0.6;\n"
            "    float a = vDatos.w * smoothstep(0.0, 0.5, cuerpo);\n"
            "    if (a < 0.004) discard;\n"
            "    vec3 n = normalize(vec3(vUV + (nz - 0.5) * 0.35, sqrt(1.0 - r2)));\n"
            "    float envuelta = clamp(dot(n, uSol) * 0.6 + 0.4, 0.0, 1.0);\n"
            "    float arriba = dot(n, uArriba) * 0.5 + 0.5;\n"
            "    vec3 cielo = mix(uSuelo, uCielo, arriba);\n"
            "    float contra = max(-uSol.z, 0.0);\n"
            "    float plata = pow(1.0 - n.z, 3.0) * contra * contra;\n"
            "    vec3 c = cielo * 0.55\n"
            "           + uLuz * (0.30 + 0.70 * envuelta) * mix(0.75, 1.0, vDatos.x)\n"
            "           + uLuz * (plata * 0.9 + contra * contra * contra * 0.15);\n"
            "    c = mix(c, uHorizonte, vDatos.y * 0.75);\n"
            "    gl_FragColor = vec4(clamp(c, 0.0, 1.0), a);\n"
            "}\n";
        const GLuint vs = compilar(GL_VERTEX_SHADER, VS, "nubes/vertices");
        if (!vs) return false;
        const GLuint fs = compilar(GL_FRAGMENT_SHADER, FS, "nubes/fragmentos");
        if (!fs) { pglDeleteShader(vs); return false; }
        prog = pglCreateProgram();
        pglAttachShader(prog, vs);
        pglAttachShader(prog, fs);
        pglLinkProgram(prog);
        pglDeleteShader(vs);
        pglDeleteShader(fs);
        GLint enlazado = 0;
        pglGetProgramiv(prog, GL_LINK_STATUS, &enlazado);
        if (!enlazado) { prog = 0; return false; }
        uSol = pglGetUniformLocation(prog, "uSol");
        uArriba = pglGetUniformLocation(prog, "uArriba");
        uLuz = pglGetUniformLocation(prog, "uLuz");
        uCielo = pglGetUniformLocation(prog, "uCielo");
        uSuelo = pglGetUniformLocation(prog, "uSuelo");
        uHorizonte = pglGetUniformLocation(prog, "uHorizonte");
        ok = true;
        return ok;
    }

    // Dibuja `n` bolas ya ordenadas de atras hacia delante. Las direcciones
    // van en espacio de ojo y normalizadas; los colores, en 0..1.
    void dibujar(const Nubes::Bola* b, int n, const float sol[3], const float arriba[3],
                 const float luz[3], const float cielo[3], const float suelo[3],
                 const float horizonte[3]) {
        if (n <= 0 || !listo()) return;
        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_FOG);
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        pglUseProgram(prog);
        pglUniform3f(uSol, sol[0], sol[1], sol[2]);
        pglUniform3f(uArriba, arriba[0], arriba[1], arriba[2]);
        pglUniform3f(uLuz, luz[0], luz[1], luz[2]);
        pglUniform3f(uCielo, cielo[0], cielo[1], cielo[2]);
        pglUniform3f(uSuelo, suelo[0], suelo[1], suelo[2]);
        pglUniform3f(uHorizonte, horizonte[0], horizonte[1], horizonte[2]);

        glBegin(GL_QUADS);
        for (int i = 0; i < n; ++i) {
            const Nubes::Bola& o = b[i];
            glColor4f(o.sombra, o.lejos, o.semilla, o.alfa);
            glTexCoord3f(-1.0f, -1.0f, o.radio); glVertex3f(o.x, o.y, o.z);
            glTexCoord3f( 1.0f, -1.0f, o.radio); glVertex3f(o.x, o.y, o.z);
            glTexCoord3f( 1.0f,  1.0f, o.radio); glVertex3f(o.x, o.y, o.z);
            glTexCoord3f(-1.0f,  1.0f, o.radio); glVertex3f(o.x, o.y, o.z);
        }
        glEnd();
        pglUseProgram(0);
        glPopAttrib();
        glDepthMask(GL_FALSE);
    }
};

} // namespace Render
