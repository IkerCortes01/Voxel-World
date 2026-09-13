#pragma once

#include <cmath>
#include <cstdint>

// ============================================================================
// HASTA DONDE SE VE, Y COMO SE DESVANECE
// ============================================================================
// RESPONSABILIDAD UNICA: convertir "la barra esta en N chunks" en los numeros
// concretos que necesitan la niebla y el mesher.
//
// Sin OpenGL, sin World, sin estado global: funciones puras. Se prueban sin
// arrancar el juego, igual que PrioridadChunk.h o VigilanteChunk.h.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA: 100 CHUNKS NO CABEN
// ----------------------------------------------------------------------------
// La barra llega a 100 porque asi se pidio, pero cargar 100 chunks de radio con
// geometria completa no lo hace ningun motor:
//
//     radio 100 -> (2*100+1)^2 = 40.401 columnas de chunk
//     a ~200 KB por columna    -> ~8 GB solo de bloques
//
// Para comparar, el valor por defecto es 4 (81 columnas, ~16 MB).
//
// ----------------------------------------------------------------------------
// LA SOLUCION: EL OJO TAMPOCO VE TODO IGUAL
// ----------------------------------------------------------------------------
// La vista humana no tiene un corte nitido: la profundidad de campo y la
// perspectiva aerea (la bruma azulada de la distancia) hacen que lo lejano se
// vea con menos detalle y menos contraste. Un paisaje real a 10 km no es un
// paisaje a 10 m dibujado mas pequeño: es una mancha suave.
//
// Asi que la distancia NO es un solo numero, son dos:
//
//   NITIDO    hasta aqui el terreno se malla y se dibuja entero.
//   VISIBLE   hasta aqui se sigue viendo algo, pero cada vez mas difuminado
//             por la niebla, hasta desaparecer del todo.
//
// La banda entre los dos es donde vive el truco: el terreno se simplifica y la
// niebla lo tapa progresivamente, asi que el jugador percibe "veo lejisimos"
// sin que el motor pague por ello.
//
// Los tramos salen de lo que se pidio:
//     2-16    detalle completo
//     17-40   completo, con la niebla cerrando al fondo
//     41-70   lo lejano simplificado, difuminado leve
//     71-100  lo lejano muy simplificado, difuminado fuerte
// ============================================================================

namespace Render {

// Los topes de la barra.
constexpr int DISTANCIA_BARRA_MIN = 2;
constexpr int DISTANCIA_BARRA_MAX = 100;

inline int acotarBarra(int n) {
    if (n < DISTANCIA_BARRA_MIN) return DISTANCIA_BARRA_MIN;
    if (n > DISTANCIA_BARRA_MAX) return DISTANCIA_BARRA_MAX;
    return n;
}

// ----------------------------------------------------------------------------
// CUANTOS CHUNKS SE CARGAN DE VERDAD
// ----------------------------------------------------------------------------
// Este es el numero que ve el streaming, y es el que protege la memoria.
//
// Hasta 16 es la identidad: lo que pides es lo que se carga. A partir de ahi
// crece MUCHO mas despacio que la barra, porque el area crece al cuadrado y
// duplicar el radio cuadruplica el coste.
//
// La curva es una raiz: de 16 en adelante, el radio real avanza con la raiz del
// exceso. Asi la barra sigue significando algo en todo su recorrido (mover de
// 70 a 100 SI carga mas mundo) sin que el coste se dispare.
//
//     barra    radio real    columnas
//       2          2            25
//       8          8           289
//      16         16         1.089
//      40         21         1.849
//      70         25         2.601
//     100         28         3.249
//
// El tope de 28 no es arbitrario: 3.249 columnas a ~200 KB son ~650 MB, que es
// el limite de lo razonable en una maquina de 8 GB con el resto del juego
// dentro. Mas alla, lo que se gana es niebla, no mundo.
inline int radioCargado(int barra) {
    const int b = acotarBarra(barra);
    if (b <= 16) return b;

    // sqrt del exceso sobre 16, escalado para llegar a 28 en la barra 100.
    // (100-16) = 84; sqrt(84) ~ 9.17; 12/9.17 ~ 1.31
    const float exceso = (float)(b - 16);
    const int extra = (int)(std::sqrt(exceso) * 1.31f + 0.5f);
    int r = 16 + extra;
    if (r > 28) r = 28;
    return r;
}

// ----------------------------------------------------------------------------
// DONDE EMPIEZA Y DONDE ACABA LA NIEBLA
// ----------------------------------------------------------------------------
// En BLOQUES, que es como los quiere OpenGL.
//
// `fogEnd` es donde la niebla es opaca del todo: mas alla no se ve nada, asi
// que es tambien el limite util de dibujado.
//
// ⭐ LA CLAVE: fogEnd se calcula desde el radio REAL, no desde la barra.
//
// Si la niebla se calibrara con la barra, a 100 se abriria hasta 1.600 bloques
// mientras el mundo solo llega a 448 -- y se veria el borde de lo cargado, que
// es exactamente lo que la niebla existe para tapar.
inline float nieblaFin(int barra, int radioReal) {
    (void)barra;
    // El 0.98 deja un margen para que el ultimo anillo se desvanezca dentro de
    // la niebla en vez de cortarse en seco.
    return (float)radioReal * 16.0f * 0.98f;
}

// Donde EMPIEZA a notarse la niebla, como fraccion de fogEnd.
//
// Aqui es donde se implementa lo que se pidio: que a 40 difumine "un poco pero
// poco notorio", y que de 70 en adelante sea "totalmente difuminado".
//
// Cuanto mas lejos quiere ver el jugador, ANTES empieza la bruma -- que es
// justo como se comporta la atmosfera real: a 50 km de visibilidad teorica, lo
// que hay a 20 km ya se ve lavado.
//
//     barra     inicio      efecto
//       2-16     0.75       niebla solo en el ultimo cuarto: casi no se ve
//       40       0.55       bruma suave en el fondo
//       70       0.38       claramente difuminado a media distancia
//      100       0.28       el fondo es una mancha: se intuye, no se lee
inline float nieblaInicioFraccion(int barra) {
    const int b = acotarBarra(barra);
    if (b <= 16) return 0.75f;

    // Interpolacion lineal de 0.75 (barra 16) a 0.28 (barra 100).
    const float t = (float)(b - 16) / (float)(DISTANCIA_BARRA_MAX - 16);
    return 0.75f - t * (0.75f - 0.28f);
}

inline float nieblaInicio(int barra, int radioReal) {
    return nieblaFin(barra, radioReal) * nieblaInicioFraccion(barra);
}

// ----------------------------------------------------------------------------
// ¿DESDE QUE DISTANCIA SE SIMPLIFICA EL TERRENO?
// ----------------------------------------------------------------------------
// En chunks. Un chunk mas alla de esto no necesita su geometria fina: la niebla
// ya lo esta tapando, asi que dibujar cada hoja y cada guijarro es pagar por
// pixeles que salen del color de la niebla.
//
// Devuelve un numero mayor que el radio cargado cuando la barra es baja, o sea
// "no simplifiques nada" -- que es lo correcto hasta 40.
inline int distanciaSimplificado(int barra) {
    const int b = acotarBarra(barra);
    if (b <= 40) return 9999;          // nada se simplifica

    // De 41 en adelante, se simplifica lo que quede pasado el 60% del radio
    // cargado. Con barra 70 eso son ~15 chunks; con 100, ~17.
    const int r = radioCargado(b);
    int d = (int)((float)r * 0.60f);
    if (d < 8) d = 8;                  // nunca simplificar lo cercano
    return d;
}

// ¿Hay que simplificar la vegetacion de un chunk a esta distancia?
//
// La vegetacion es el mejor candidato con diferencia: las aciculas de un ocote
// son cientos de quads diminutos por chunk, y a 200 bloques con niebla encima
// no se distingue un pino de una mancha verde.
inline bool simplificarVegetacion(int barra, float distanciaChunks) {
    return distanciaChunks > (float)distanciaSimplificado(barra);
}

// ----------------------------------------------------------------------------
// UNA ETIQUETA PARA LA UI
// ----------------------------------------------------------------------------
// Lo que se escribe junto a la barra, para que el numero signifique algo.
inline const char* nombreCalidad(int barra) {
    const int b = acotarBarra(barra);
    if (b <= 4)  return "Cerca";
    if (b <= 16) return "Normal";
    if (b <= 40) return "Lejos";
    if (b <= 70) return "Muy lejos";
    return "Extremo";
}

} // namespace Render
