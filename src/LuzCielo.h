#pragma once

// ============================================================================
// LUZ DEL CIELO QUE ENTRA EN UNA COLUMNA DESDE ARRIBA
// ============================================================================
// La unica luz del motor es la del cielo. Ningun bloque emite luz: no hay
// antorchas ni torchlight, y el lector de luz (Chunk::getLightLevel) solo mira
// el sunlight.
//
// Esta funcion responde a una pregunta concreta de `recalcularLuzLocal`: si la
// caja que se recalcula empieza en `yDesde`, ¿con cuanta luz vertical entra la
// columna por arriba?
//
//   - 0 si entre yDesde y el techo de la zona hay algo opaco;
//   - 18 menos la atenuacion de cada capa de follaje, si no.
//
// EL BUG QUE LA MOTIVA: la caja empezaba SIEMPRE con 18 ("cielo pleno"),
// suponiendo que su borde de arriba quedaba sobre el techo de la zona. Pero
// ese borde se recorta a `cy + 18` para no recorrer roca de mas, y colocando
// bajo un tejado, en una cueva o al pie de una montana cae DENTRO de lo
// tapado. La celda recibia 18, el flood-fill lo derramaba, y aparecia un
// parche de luz en plena oscuridad cada vez que se colocaba un bloque.
//
// Es una plantilla sobre dos funciones para poder probarla sin el motor:
//
//     esOpaco(y)    -> bool   la celda corta la luz del cielo
//     esFollaje(y)  -> bool   la celda la deja pasar atenuada
//
// Recorre de yDesde hacia arriba hasta yTecho (incluido) y se corta en el
// primer opaco, asi que bajo tierra cuesta una o dos celdas.

constexpr int LUZ_CIELO_PLENA = 18;

// ============================================================================
// ⭐⭐ DE NIVEL DE LUZ A BRILLO EN PANTALLA
// ============================================================================
// La UNICA curva del motor. La usan el terreno, los items sueltos, los bloques
// que caen y los animales, para que la misma luz se vea igual en todo.
//
//   raw   luz del cielo que llega a la celda, de 0 a 1 (nivel / 18)
//   sol   hora del dia: 1 pleno dia, 0 noche cerrada (luzSolar)
//
// EL BUG QUE CORRIGE: "de noche, las paredes de un cuarto cerrado se ven
// iluminadas". La curva vieja, copiada en cuatro sitios, era
//
//     f = max(raw, 0.15)
//     brillo = f*(1-raw) + f*raw*(luna + (1-luna)*sol)
//
// y trataba la parte `f*(1-raw)` como luz "propia" que no depende del sol
// --la de una antorcha--. Pero aqui NO hay antorchas: toda la luz es del
// cielo, asi que esa parte era luz saliendo de la nada. De noche la curva ni
// siquiera era creciente:
//
//     cielo abierto  (18)  -> 0.12
//     cuarto a media luz (10) -> 0.28   <- mas del DOBLE que afuera
//     cuarto cerrado  (0)  -> 0.15   <- tambien mas que afuera
//
// LA CURVA NUEVA: un minimo fijo para no ver negro puro, y por encima de el
// SOLO luz del cielo, que de noche se reduce a la de la luna:
//
//     brillo = MINIMO + (1 - MINIMO) * raw * (luna + (1 - luna) * sol)
//
// Es creciente en raw y en sol: mas cielo o mas sol nunca da menos brillo, y
// ningun interior puede verse mas claro que la intemperie a la misma hora. De
// dia y a cielo abierto da 1.0, igual que antes.

// Lo que se ve sin ninguna luz: negro casi total, pero con siluetas. Por
// debajo de la intemperie nocturna, que es lo que garantiza que un cuarto
// cerrado de noche sea lo mas oscuro de la escena.
constexpr float LUZ_AMBIENTE_MINIMA = 0.10f;

// Fraccion del sol que queda de noche: luna y estrellas.
constexpr float LUZ_DE_LUNA_CIELO = 0.12f;

inline float brilloDeLuz(float raw, float sol) {
    if (raw < 0.0f) raw = 0.0f;
    if (raw > 1.0f) raw = 1.0f;
    if (sol < 0.0f) sol = 0.0f;
    if (sol > 1.0f) sol = 1.0f;
    const float cielo = LUZ_DE_LUNA_CIELO + (1.0f - LUZ_DE_LUNA_CIELO) * sol;
    return LUZ_AMBIENTE_MINIMA + (1.0f - LUZ_AMBIENTE_MINIMA) * raw * cielo;
}

// Lo mismo desde el nivel crudo del motor (0..18).
inline float brilloDeNivel(int nivel, float sol) {
    return brilloDeLuz((float)nivel / (float)LUZ_CIELO_PLENA, sol);
}

template <class EsOpaco, class EsFollaje>
int luzQueEntraPorArriba(EsOpaco esOpaco, EsFollaje esFollaje,
                         int yDesde, int yTecho, int atenuacionHoja) {
    int luz = LUZ_CIELO_PLENA;
    for (int y = yDesde; y <= yTecho; ++y) {
        if (esOpaco(y)) return 0;
        if (esFollaje(y)) {
            luz -= atenuacionHoja;
            if (luz <= 0) return 0;
        }
    }
    return luz;
}
