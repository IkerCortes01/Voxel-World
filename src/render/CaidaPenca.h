#pragma once

// ============================================================================
// LA CAIDA DE LA PENCA TIENE QUE ACABAR DONDE EMPIEZA EL REPOSO
// ============================================================================
// EL BUG: al colocar una penca de nopal "se estiraba mucho y luego volvia a su
// forma". Era literal, y el salto estaba medido en el propio codigo sin que
// nadie cruzara los dos numeros:
//
//     la animacion terminaba en   X = 15 px   (el LARGO de la penca)
//     el reposo la dibuja en      X =  8 px   (el ANCHO)
//
// O sea que durante los 0,40 s de la caida la penca crecia hasta casi el doble
// de ancha y, al acabar la animacion, pegaba un tiron de vuelta a su tamano
// real. Lo mismo en Y: la animacion la dejaba apoyada en el suelo del voxel y
// el reposo la pone centrada.
//
// ----------------------------------------------------------------------------
// LA CAUSA, QUE ES LO QUE CONVIENE NO REPETIR
// ----------------------------------------------------------------------------
// Las dos formas se escribieron POR SEPARADO. La animacion interpolaba hacia
// "una penca tumbada" imaginaria --recalculada a mano con las constantes-- en
// vez de hacia LA penca tumbada que el mesher dibuja un instante despues.
//
// Mientras las dos cajas se calculen en sitios distintos, nada impide que
// vuelvan a separarse: es exactamente el mismo patron que el de las tunas (la
// precarga cargaba unas imagenes y el mesher pedia otras) y el de la penca que
// se caia y se volvia a levantar (dos listas de "que es suelo firme").
//
// Aqui vive la regla, y un test la comprueba.
// ============================================================================

namespace Render {

// Una caja en coordenadas de voxel (0..1 en cada eje).
struct CajaPenca {
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
};

// ----------------------------------------------------------------------------
// LA CAJA DE REPOSO DE UNA PENCA TUMBADA
// ----------------------------------------------------------------------------
// Es la que dibuja `case 4` del mesher (losa horizontal).
//
//   ancho  -> A0..A1 en X y Z
//   grosor -> c0..c1 en Y, CENTRADO en el voxel
//
// `bajada` es cuanto desciende por apoyarse en un nivel parcial: media losa
// llega a 0.5, no a 1.0, asi que la penca tiene que bajar 0.5 o quedaria
// flotando. Vale 0 sobre un bloque entero.
inline CajaPenca cajaReposoTumbada(float ancho, float grosor, float bajada) {
    const float m  = 0.5f - ancho * 0.5f;
    const float a0 = m, a1 = 1.0f - m;
    const float c0 = 0.5f - grosor * 0.5f;
    const float c1 = 0.5f + grosor * 0.5f;
    return CajaPenca{ a0, a1, c0 - bajada, c1 - bajada, a0, a1 };
}

// ----------------------------------------------------------------------------
// LA CAJA DE UNA PENCA DE PIE
// ----------------------------------------------------------------------------
// El punto de PARTIDA de la caida: larga en Y, fina en los otros dos ejes.
inline CajaPenca cajaDePie(float largo, float ancho, float grueso) {
    return CajaPenca{
        0.5f - ancho  * 0.5f, 0.5f + ancho  * 0.5f,
        0.5f - largo  * 0.5f, 0.5f + largo  * 0.5f,
        0.5f - grueso * 0.5f, 0.5f + grueso * 0.5f
    };
}

// ----------------------------------------------------------------------------
// LA CAJA A MITAD DE CAIDA
// ----------------------------------------------------------------------------
// `t` es el progreso, 0..1. La curva es t*t y no lineal: arranca despacio y
// acelera, que es como cae algo por gravedad. Una interpolacion recta se ve
// mecanica.
//
// ⭐ EL DESTINO ES LA CAJA DE REPOSO, no una recalculada aparte. Es lo unico
// que garantiza que en t=1 no haya salto.
inline CajaPenca cajaCayendo(const CajaPenca& dePie,
                             const CajaPenca& reposo,
                             float progreso) {
    const float t = progreso * progreso;
    auto mez = [t](float a, float b) { return a + (b - a) * t; };
    return CajaPenca{
        mez(dePie.minX, reposo.minX), mez(dePie.maxX, reposo.maxX),
        mez(dePie.minY, reposo.minY), mez(dePie.maxY, reposo.maxY),
        mez(dePie.minZ, reposo.minZ), mez(dePie.maxZ, reposo.maxZ)
    };
}

} // namespace Render
