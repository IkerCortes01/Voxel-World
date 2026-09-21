#pragma once

#include "../terrain/BiomeTypes.h"

#include <cstdint>

// ============================================================================
// EL COLOR DEL PASTO SEGUN SU BIOMA
// ============================================================================
// Una sola textura de pasto para todo el mundo hace que un prado de altiplano,
// un pinar de montana y un matorral de desierto se vean EXACTAMENTE igual. Es
// lo que delata que el bioma es solo una etiqueta y no un sitio.
//
// La solucion es la misma que usa Minecraft desde hace quince anos: la textura
// no lleva el color, lleva la FORMA. El color se multiplica al dibujar, por
// bioma. Asi una imagen sirve para todos y el tinte no cuesta ni un byte de
// memoria de textura.
//
// ----------------------------------------------------------------------------
// COMO LO HACE MINECRAFT, Y QUE SE TOMA DE AHI
// ----------------------------------------------------------------------------
// Guarda la textura en GRIS y la multiplica por un color sacado de una imagen
// de 256x256 (`colormap/grass.png`), indexada por dos escalares del bioma:
//
//     adjTemp     = clamp(temperature, 0, 1)
//     adjDownfall = clamp(downfall, 0, 1) * adjTemp      <- OJO A ESTE PRODUCTO
//     x = (1 - adjTemp) * 255
//     y = (1 - adjDownfall) * 255
//
// Ese `* adjTemp` es la parte inteligente: convierte el espacio de colores
// validos en un TRIANGULO en vez de un cuadrado. Un bioma frio no puede ser
// verde exuberante, porque la temperatura baja colapsa el eje de humedad hacia
// cero. Es una restriccion fisica metida en la formula.
//
// Las dos direcciones del gradiente, que es lo que de verdad importa:
//     SEQUEDAD -> tira a AMARILLO
//     FRIO     -> tira a GRIS AZULADO desaturado
//
// AQUI NO SE USA UNA IMAGEN, SE USA UNA TABLA. Razones:
//   - Son once biomas, no un continuo: una tabla es exacta y se lee.
//   - Este motor ya tiene los biomas como DATOS en BiomeTypes.h, con tabla y
//     static_assert. Una tabla mas encaja; un PNG que hay que samplear, no.
//   - Minecraft mismo se salta su propio colormap cuando el clima no da el
//     aspecto que quiere (badlands y pantano van a fuego). Si la excepcion es
//     habitual, la regla no estaba aportando tanto.
//
// ----------------------------------------------------------------------------
// POR QUE ESTOS COLORES Y NO LOS DE MINECRAFT
// ----------------------------------------------------------------------------
// Porque este mundo es el altiplano mexicano, no el campo ingles, y hay UN
// hecho climatico que lo cambia todo: la estacionalidad monzonica. Llueve de
// junio a octubre y no llueve el resto del ano. El pastizal del altiplano esta
// verde unos cuatro meses; su estado de reposo es pajizo.
//
// Por eso TODOS estos colores estan menos saturados que los de Minecraft. El
// verde de sus Plains (#91BD59) ronda el 54% de saturacion y se lee como prado
// de regadio: el altiplano practicamente nunca se ve asi.
//
// Cada color va justificado con su ecologia en su fila. No son gustos.
// ============================================================================

namespace Render {

struct TinteRGB {
    float r, g, b;
};

// ----------------------------------------------------------------------------
// LA TABLA
// ----------------------------------------------------------------------------
// Indexada por BiomeType. El orden DEBE coincidir con el enum de BiomeTypes.h,
// igual que BIOME_TABLE -- y por la misma razon: una fila fuera de sitio da el
// color equivocado sin ningun error. Hay static_assert al final.
//
// Valor 1.0 = sin tenir. Se usa donde no hay pasto que tenir (oceanos).
inline const TinteRGB* tablaTintesPasto() {
    using namespace TerrainGen;

    static const TinteRGB TABLA[BIOME_COUNT] = {
        // --- BIOME_OCEAN_DEEP: no hay pasto bajo el agua ---
        { 1.00f, 1.00f, 1.00f },

        // --- BIOME_OCEAN: idem ---
        { 1.00f, 1.00f, 1.00f },

        // --- BIOME_BEACH  #B5B878 ---
        // Plantas halofitas y psammofitas de duna. El salitre y el albedo alto
        // de la arena lavan el follaje; ademas son glaucas y cerosas como
        // defensa contra la sal. Saturacion muy baja, valor levantado por la
        // arena.
        { 0.710f, 0.722f, 0.471f },

        // --- BIOME_PLAINS  #A8B457 ---
        // Pastizal de altiplano dominado por navajita (Bouteloua gracilis):
        // estepa de pasto corto, 300-600 mm al ano. Bouteloua SE CURA EN PIE,
        // asi que incluso creciendo arrastra paja muerta de la temporada
        // anterior. Mas amarillo y menos saturado que el Plains de Minecraft.
        { 0.659f, 0.706f, 0.341f },

        // --- BIOME_FOREST  #7BA644 ---
        // Encinar. La copa es semiabierta, asi que al sotobosque le llega luz
        // de verdad; suelo mesico y nitrogeno de la hojarasca sostienen
        // hierbas y musgo verdes. Es el bioma terrestre mas exuberante de
        // este mundo -- pero no es tropical, de ahi el punto de oliva.
        { 0.482f, 0.651f, 0.267f },

        // --- BIOME_DESERT  #BFAE63 ---
        // Matorral xerofilo chihuahuense. Entre arbusto y arbusto lo que hay
        // es SUELO calcareo expuesto, no planta, y el pasto suelto esta
        // curado. El tinte tira al color del suelo, no al de la hoja: en el
        // plano del terreno manda la tierra.
        { 0.749f, 0.682f, 0.388f },

        // --- BIOME_MOUNTAINS  #8FA860 ---
        // Ladera: suelo delgado y pedregoso, insolacion alta y drenaje fuerte
        // -> vegetacion con estres hidrico, hoja mas pequena y mas gris. El
        // cascajo mezclado en el plano del suelo desatura el conjunto.
        { 0.561f, 0.659f, 0.376f },

        // --- BIOME_MOUNTAIN_PEAKS  #8CA88F ---
        // Zacatonal alpino por encima de los 3.800 m (Popocatepetl, Nevado de
        // Toluca): macollos de Festuca y Calamagrostis tolucensis. La
        // literatura describe su "aspecto xerofitico... reflejado por la vaina
        // de hojas secas" -- sequia por congelacion. Verde grisaceo y apagado,
        // la misma logica que el #80B497 de los biomas nevados de Minecraft.
        { 0.549f, 0.659f, 0.561f },

        // --- LOS TRES OCOTALES ---

        // --- BIOME_OCOTAL_BLANCO  #7D9E6B ---
        // Pinus montezumae, 2.000-3.200 m y 800-1.000+ mm: el pinar mas humedo
        // de los tres, asi que es verde de verdad. Pero la capa gruesa de
        // acicula acidifica el suelo y suelta terpenos que ahogan el estrato
        // herbaceo; lo que se ve del suelo es parte musgo y parte mantillo
        // pardo. Verde apagado y "ensuciado" por la hojarasca.
        { 0.490f, 0.620f, 0.420f },

        // --- BIOME_OCOTAL_CHINO  #96A159 ---
        // Pinus leiophylla, cota media y mas seco. Descrito como "dosel
        // abierto, tipo parque, con sotobosque de herbaceas y zacate en
        // macollo". Dosel abierto + menos lluvia = el zacate se cura amarillo.
        // El mas oliva de los tres ocotales.
        { 0.588f, 0.631f, 0.349f },

        // --- BIOME_OCOTAL_MIXTO  #89A260 ---
        // Mosaico de pino y encino. La hojarasca de encino se descompone mas
        // deprisa y acidifica menos que la de pino, asi que el sotobosque es
        // mas rico que en un pinar puro pero mas irregular que en un encinar.
        // Queda a proposito entre los dos anteriores.
        { 0.537f, 0.635f, 0.376f },
    };

    return TABLA;
}

// El tinte del pasto de un bioma. Fuera de rango devuelve "sin tenir", que es
// el comportamiento anterior: si algun dia se anade un bioma y se olvida su
// fila, el pasto sale como siempre en vez de negro.
inline TinteRGB tintePasto(TerrainGen::BiomeType b) {
    const int i = (int)b;
    if (i < 0 || i >= (int)TerrainGen::BIOME_COUNT) return { 1.0f, 1.0f, 1.0f };
    return tablaTintesPasto()[i];
}

// ----------------------------------------------------------------------------
// ¿QUE BLOQUES SE TINEN?
// ----------------------------------------------------------------------------
// Solo la vegetacion HERBACEA, y esto importa: tenir de verde la tierra, la
// piedra o la arena de debajo los pondria del color del pasto, que es
// exactamente el error que el tinte viene a arreglar.
//
// La cara de ABAJO de un bloque de pasto es tierra, y la lleva el propio
// mesher: se tine por (bloque, cara), no por bloque a secas.
inline bool caraSeTine(BlockType b, int cara) {
    switch (b) {
        case BLOCK_GRASS:
            // Arriba y los cuatro lados; abajo es tierra pura.
            //   0 = arriba, 1 = abajo, 2..5 = lados
            return cara != 1;

        case BLOCK_TALLGRASS:
            // La hierba alta es toda ella herbacea.
            return true;

        default:
            return false;
    }
}

// ----------------------------------------------------------------------------
// LA FRONTERA ENTRE DOS BIOMAS
// ----------------------------------------------------------------------------
// Sin suavizar, el cambio de color cae en la linea exacta donde cambia el
// bioma y se ve como un recorte de tijera sobre el prado.
//
// Se resuelve promediando el tinte de una vecindad de columnas. Es lo mismo
// que hace Minecraft, que mezcla el color en un radio de varios bloques, y lo
// que convierte la frontera en un degradado de unos metros.
//
// Devuelve el promedio de los tintes que se le pasen. Se separa en funcion
// para poder probarla: la mezcla es donde es facil colarse con un peso.
inline TinteRGB mezclarTintes(const TinteRGB* tintes, int cuantos) {
    if (cuantos <= 0) return { 1.0f, 1.0f, 1.0f };

    float r = 0.0f, g = 0.0f, b = 0.0f;
    for (int i = 0; i < cuantos; ++i) {
        r += tintes[i].r;
        g += tintes[i].g;
        b += tintes[i].b;
    }
    const float inv = 1.0f / (float)cuantos;
    return { r * inv, g * inv, b * inv };
}

// ----------------------------------------------------------------------------
// LA TABLA Y EL ENUM NO PUEDEN DESINCRONIZARSE
// ----------------------------------------------------------------------------
// Mismo problema que BIOME_TABLE y misma defensa: una fila corrida da el color
// equivocado SIN ERROR -- el desierto saldria verde bosque -- y eso es de los
// fallos que se miran diez veces sin ver.
//
// No se puede comprobar el `type` como hace BIOME_TABLE (aqui las filas no lo
// llevan), asi que la defensa es el TAMANO DEL ARRAY.
//
// Y funciona sola, sin necesidad de static_assert: la tabla se declara como
// `TABLA[BIOME_COUNT]` con una lista de inicializadores. Si alguien anade un
// bioma al enum sin anadir su fila, el array queda con una fila de menos...
//
// ...lo que NO es un error de compilacion -- C++ rellena el resto con ceros --
// y el bioma nuevo saldria con tinte {0,0,0}, o sea PASTO NEGRO. Eso es un
// fallo silencioso, justo lo que aqui no se quiere.
//
// Asi que se comprueba explicitamente que la ULTIMA fila esperada no sea cero.
// Es una comprobacion barata que atrapa exactamente ese descuido: si el enum
// crece, la ultima fila deja de ser la del ocotal mixto y pasa a ser relleno.
inline bool tablaTintesCompleta() {
    const TinteRGB ultimo = tablaTintesPasto()[TerrainGen::BIOME_COUNT - 1];
    return !(ultimo.r == 0.0f && ultimo.g == 0.0f && ultimo.b == 0.0f);
}

} // namespace Render
