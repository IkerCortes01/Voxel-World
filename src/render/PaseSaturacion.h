#pragma once
// ============================================================================
// LA SATURACION, SOBRE TODO LO QUE SE VE
// ============================================================================
// PEDIDO: "cuando se baja la saturacion al minimo del juego, afecte cualquier
// cosa a la hotbar e inventario".
//
// La opcion SATURACION solo la aplicaba el shader del terreno: el mundo se
// quedaba en gris y la hotbar, el inventario, los items, el cuerpo del
// jugador, los animales y el cielo seguian a todo color. Aqui se aplica al
// final del frame, sobre la imagen ya entera: se copia el buffer a una
// textura y se vuelve a pintar mezclada con su gris (luminancia Rec. 601).
// Por encima de 100 la misma mezcla satura de mas.
//
// Cuando este pase funciona, el terreno deja de aplicarla por su cuenta (se
// le pasa 1.0) para no hacerlo dos veces.
//
// Usa las funciones de shader que ya carga TerrenoGL.h. Si algo falla (un
// driver sin GLSL), `listo()` da false y el terreno sigue con lo suyo.
// ============================================================================

#include "TerrenoGL.h"

namespace Render {

struct PaseSaturacion {
    GLuint prog = 0, tex = 0;
    GLint uImg = -1, uSat = -1;
    int tw = 0, th = 0;
    bool intentado = false, ok = false;

    bool listo() {
        if (intentado) return ok;
        intentado = true;
        if (!pglCreateProgram || !pglUseProgram || !pglUniform1f || !pglUniform1i) return false;
        static const char* VS =
            "#version 120\n"
            "void main() { gl_TexCoord[0] = gl_MultiTexCoord0; gl_Position = gl_Vertex; }\n";
        static const char* FS =
            "#version 120\n"
            "uniform sampler2D uImg;\n"
            "uniform float uSat;\n"
            "void main() {\n"
            "    vec3 c = texture2D(uImg, gl_TexCoord[0].xy).rgb;\n"
            "    float g = dot(c, vec3(0.299, 0.587, 0.114));\n"
            "    gl_FragColor = vec4(clamp(mix(vec3(g), c, uSat), 0.0, 1.0), 1.0);\n"
            "}\n";
        const GLuint vs = compilar(GL_VERTEX_SHADER, VS, "saturacion/vertices");
        if (!vs) return false;
        const GLuint fs = compilar(GL_FRAGMENT_SHADER, FS, "saturacion/fragmentos");
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
        uImg = pglGetUniformLocation(prog, "uImg");
        uSat = pglGetUniformLocation(prog, "uSat");
        glGenTextures(1, &tex);
        ok = tex != 0;
        return ok;
    }

    // Aplica la saturacion `sat` (1 = sin cambio) a la imagen de w x h.
    bool aplicar(int w, int h, float sat) {
        if (w <= 0 || h <= 0 || !listo()) return false;
        constexpr GLenum BORDE = 0x812F;          // GL_CLAMP_TO_EDGE
        if (pglActiveTexture) pglActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        if (w != tw || h != th) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, BORDE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, BORDE);
            tw = w; th = h;
        }
        glReadBuffer(GL_BACK);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);

        glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
        glViewport(0, 0, w, h);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_BLEND);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_FOG);
        glEnable(GL_TEXTURE_2D);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);  glPushMatrix(); glLoadIdentity();

        pglUseProgram(prog);
        if (uImg >= 0) pglUniform1i(uImg, 0);
        if (uSat >= 0) pglUniform1f(uSat, sat);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(1, 0); glVertex2f( 1, -1);
        glTexCoord2f(1, 1); glVertex2f( 1,  1);
        glTexCoord2f(0, 1); glVertex2f(-1,  1);
        glEnd();
        pglUseProgram(0);

        glMatrixMode(GL_PROJECTION); glPopMatrix();
        glMatrixMode(GL_MODELVIEW);  glPopMatrix();
        glPopAttrib();
        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }
};

} // namespace Render
