#include <doctest/doctest.h>
#include <cmath>
#include "BloqueCompuesto.h"

// ============================================================================
// EL NOPAL COMO MATA: VARIAS PENCAS EN UNA SOLA CASILLA
// ============================================================================
// PEDIDO: "que el nopal permita tener 3 o 4 pencas mas en su misma casilla en
// cualquier direccion o posicion -- encima, abajo, al lado, este, sur, norte,
// oeste, mil formas".
//
// El sistema viejo guardaba el apilado en IDs DEL ENUM (CLADODIO_X2, X3, Z2,
// Z3): tope de 3 pencas y solo en 2 ejes. Este lo sustituye por una familia
// compuesta, FAM_NOPAL, con el estado dentro del ID.
//
// Lo que estos tests fijan, que es lo que puede romper mundos guardados:
//
//   1. El estado sobrevive al viaje por el ID (se guarda y se carga igual).
//   2. El nopal NO invade el rango de otra familia.
//   3. La forma derivada del hash es ESTABLE: la misma casilla da siempre la
//      misma planta, o el nopal cambiaria de forma al recargar el chunk.
//   4. Las pencas extra caen DENTRO de su celda. Si se salieran, invadirian la
//      del vecino y se dibujarian dos veces.
// ============================================================================


#include <fstream>
#include <string>

// Lee src/main.cpp para comprobar invariantes del motor que no se pueden
// enlazar (main.cpp no entra en el binario de tests).
static std::string leerFuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f) return std::string();
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return std::string();
#endif
}

using namespace Compuesto;

TEST_CASE("el nopal es su propia familia y no pisa a nadie") {
    // ESTE es el test que protege el formato de guardado. Si FAM_NOPAL se
    // moviera de sitio, un nopal guardado se leeria como otra planta.
    const BlockType n = Nopal::nuevo(3);
    CHECK(esCompuesto(n));
    CHECK(familiaDe(n) == FAM_NOPAL);
    CHECK(Nopal::esNopal(n));

    // Y no se confunde con las familias vecinas.
    CHECK_FALSE(Agua::esAgua(n));
    CHECK_FALSE(esAguaVolumen(n));

    // El agua tampoco se lee como nopal.
    CHECK_FALSE(Nopal::esNopal(Agua::nuevo(4)));
}

TEST_CASE("el numero de pencas extra sobrevive al ID") {
    // El viaje que hace al guardarse: estado -> ID -> disco -> ID -> estado.
    for (uint16_t n = 0; n <= Nopal::MAX_EXTRAS; ++n) {
        const BlockType b = Nopal::nuevo(n);
        CHECK(Nopal::extrasDe(b) == n);
    }
}

TEST_CASE("el numero de pencas se acota en vez de desbordarse") {
    // Pedir mas de lo que cabe tiene que dar el tope, NO un estado corrido que
    // se saldria del campo y contaminaria los bits de al lado (la semilla y el
    // rollo viven justo encima).
    const BlockType b = Nopal::nuevo(200);
    CHECK(Nopal::extrasDe(b) == Nopal::MAX_EXTRAS);
    CHECK(familiaDe(b) == FAM_NOPAL);
}

TEST_CASE("los campos no se pisan entre si") {
    // ⭐ LOS CINCO CAMPOS SUMAN EXACTAMENTE 16 BITS, y estan pegados. Escribir
    // uno no puede tocar a los otros: es el fallo clasico del empaquetado, y
    // aqui se manifestaria como un nopal que cambia de forma al crecer.
    BlockType b = Nopal::nuevo(5, 7, true);
    b = Nopal::conDireccion(b, 0, 4);
    b = Nopal::conDireccion(b, 1, 2);
    b = Nopal::conDireccion(b, 2, 5);

    CHECK(Nopal::extrasDe(b)  == 5);
    CHECK(Nopal::semillaDe(b) == 7);
    CHECK(Nopal::tieneRollo(b));
    CHECK(Nopal::direccionDe(b, 0, 0) == 4);
    CHECK(Nopal::direccionDe(b, 1, 0) == 2);
    CHECK(Nopal::direccionDe(b, 2, 0) == 5);
}

TEST_CASE("anadir pencas respeta el tope") {
    BlockType b = Nopal::nuevo(0);
    for (int i = 0; i < (int)Nopal::MAX_EXTRAS; ++i) {
        const BlockType antes = b;
        b = Nopal::anadirPenca(b, i % 6);
        CHECK(b != antes);                       // cabia: el bloque cambia
        CHECK(Nopal::extrasDe(b) == i + 1);
    }
    // Lleno: anadir otra devuelve EL MISMO bloque, que es como el llamador
    // distingue "cabia" de "no cabia".
    const BlockType lleno = b;
    CHECK(Nopal::anadirPenca(b, 0) == lleno);
}

TEST_CASE("la forma derivada del hash es ESTABLE") {
    // ⭐⭐ EL TEST QUE IMPORTA DE VERDAD.
    //
    // La geometria NO se guarda: se deriva del hash de la posicion. Si esa
    // derivacion no fuera determinista, el nopal cambiaria de forma cada vez
    // que el chunk se remalla -- las pencas bailarian solas.
    const BlockType b = Nopal::nuevo(4, 3);

    for (int i = 0; i < 4; ++i) {
        const Nopal::Extra a = Nopal::colocar(b, i, 100, 64, -40);
        const Nopal::Extra c = Nopal::colocar(b, i, 100, 64, -40);
        CHECK(a.cx      == doctest::Approx(c.cx));
        CHECK(a.cy      == doctest::Approx(c.cy));
        CHECK(a.cz      == doctest::Approx(c.cz));
        CHECK(a.anguloY == doctest::Approx(c.anguloY));
        CHECK(a.escala  == doctest::Approx(c.escala));
        CHECK(a.dir     == c.dir);
    }
}

TEST_CASE("las pencas extra caen DENTRO de su celda") {
    // Si una penca se saliera de su casilla invadiria la del vecino, y como
    // cada celda se dibuja desde si misma acabaria dibujada dos veces --
    // z-fighting y geometria duplicada.
    //
    // Se barren muchas posiciones y semillas porque el desplazamiento sale del
    // hash: un solo caso no prueba nada.
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        for (int x = -3; x <= 3; ++x)
        for (int z = -3; z <= 3; ++z)
        for (int i = 0; i < (int)Nopal::MAX_EXTRAS; ++i) {
            const Nopal::Extra e = Nopal::colocar(b, i, x * 17, 70, z * 13);
            CHECK(e.cx >= 0.0f); CHECK(e.cx <= 1.0f);
            CHECK(e.cy >= 0.0f); CHECK(e.cy <= 1.0f);
            CHECK(e.cz >= 0.0f); CHECK(e.cz <= 1.0f);
        }
    }
}

TEST_CASE("la escala y la inclinacion se quedan en su rango") {
    // La escala alimenta el tamano de los quads y la inclinacion los tuerce.
    // Un valor fuera de rango daria una penca gigante o del reves.
    //
    // ⚠️ LOS LIMITES CAMBIARON AL ENCADENAR LAS PENCAS, y conviene dejar
    // escrito por que en vez de solo ajustar los numeros:
    //
    //   ESCALA. Antes era 0.55..1.00 porque cada penca se sorteaba suelta.
    //   Ahora se AFINA con la profundidad de la cadena (`merma`), que es lo que
    //   hace que la mata se estreche hacia la punta como un nopal real. El
    //   suelo duro es 0.40, puesto en `colocar`.
    //
    //   ANGULO. Antes era un giro libre 0..2pi. Ahora cada penca HEREDA el
    //   rumbo de la anterior y solo se le suma ruido corto, asi que el primer
    //   eslabon puede quedar ligeramente NEGATIVO (-0.45 en el peor caso). Eso
    //   es correcto: un angulo es ciclico y -0.3 rad es un rumbo perfectamente
    //   valido. Lo que importa es que no pegue saltos, y eso lo fija el test
    //   "el giro de la cadena no da saltos bruscos".
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        for (int x = -4; x <= 4; ++x)
        for (int i = 0; i < (int)Nopal::MAX_EXTRAS; ++i) {
            const Nopal::Extra e = Nopal::colocar(b, i, x * 23, 66, x * 7);
            CHECK(e.escala >= 0.40f);
            CHECK(e.escala <= 1.00f);
            CHECK(e.inclina >= -0.45f);
            CHECK(e.inclina <=  0.45f);
            // Acotado, no positivo: el giro es ciclico (ver la nota de arriba).
            CHECK(e.anguloY >= -1.0f);
            CHECK(e.anguloY <= 6.2831853f);
            CHECK(e.dir >= 0); CHECK(e.dir < 6);
        }
    }
}

TEST_CASE("dos casillas distintas dan plantas distintas") {
    // Si el hash no separara por posicion, una nopalera entera saldria clonada
    // -- que es el defecto que este sistema existe para evitar.
    const BlockType b = Nopal::nuevo(4, 2);

    int distintas = 0;
    const Nopal::Extra ref = Nopal::colocar(b, 0, 0, 64, 0);
    for (int x = 1; x <= 40; ++x) {
        const Nopal::Extra e = Nopal::colocar(b, 0, x, 64, 0);
        if (e.anguloY != doctest::Approx(ref.anguloY)) ++distintas;
    }
    // No se exige que TODAS difieran (el hash puede repetir), pero la inmensa
    // mayoria si: con 40 casillas, menos de 30 distintas seria un hash malo.
    CHECK(distintas >= 30);
}

TEST_CASE("las tres primeras direcciones mandan sobre el hash") {
    // Las tres primeras van guardadas de verdad; de la cuarta en adelante el
    // hash decide. Es el reparto que permite "mil formas" sin gastar 3 bits por
    // penca, y conviene que quede fijado.
    BlockType b = Nopal::nuevo(6, 1);
    b = Nopal::conDireccion(b, 0, 2);   // arriba
    b = Nopal::conDireccion(b, 1, 3);   // abajo
    b = Nopal::conDireccion(b, 2, 0);   // este

    // Las guardadas no dependen del hash que se les pase.
    const unsigned HASHES[] = { 0u, 12345u, 999999u };
    for (unsigned h : HASHES) {
        CHECK(Nopal::direccionDe(b, 0, h) == 2);
        CHECK(Nopal::direccionDe(b, 1, h) == 3);
        CHECK(Nopal::direccionDe(b, 2, h) == 0);
    }

    // La cuarta SI cambia con el hash.
    CHECK(Nopal::direccionDe(b, 3, 0u) != Nopal::direccionDe(b, 3, 7777u));
}

TEST_CASE("las pencas forman una CADENA: cada una toca a la anterior") {
    // ⭐⭐ ESTE ES EL TEST DEL ENCARGO.
    //
    // PEDIDO: que las pencas esten "apiladas, pegadas y enlazadas
    // naturalmente". La version anterior las sorteaba de forma independiente y
    // quedaban desperdigadas: un ramillete de piezas sueltas, no una planta.
    //
    // La propiedad que lo demuestra es la DISTANCIA entre pencas consecutivas:
    // si estan encadenadas, la `i` y la `i-1` tienen que estar lo bastante
    // cerca como para tocarse. Si cada una se colocara por su cuenta, esa
    // distancia seria arbitraria.
    //
    // ⭐ EL TOPE SALE DE LA GEOMETRIA, no de un numero a ojo.
    //
    // Dos pencas que se tocan por el borde tienen sus centros separados por
    // media suma de sus altos, menos el solape, mas el desvio lateral. Con
    // alto = 2*SEMIEJE*escala y escala <= 1:
    //
    //     avance maximo = 2*SEMIEJE - solape + desvio  <  2*SEMIEJE * 1.2
    //
    // Se deja ese 20% de margen y nada mas. Un tope generoso (0.55, media
    // celda) habria pasado con las pencas DESPERDIGADAS, que es exactamente el
    // defecto que este test existe para cazar -- y de hecho paso con el, hasta
    // que el diagnostico mostro que el 85% estaba contra el techo.
    const float TOPE = 2.0f * Nopal::SEMIEJE * 1.2f;
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        for (int x = -2; x <= 2; ++x)
        for (int z = -2; z <= 2; ++z) {
            Nopal::Extra prev = Nopal::colocar(b, 0, x * 31, 68, z * 19);
            for (int i = 1; i < (int)Nopal::MAX_EXTRAS; ++i) {
                const Nopal::Extra cur =
                    Nopal::colocar(b, i, x * 31, 68, z * 19);
                const float dx = cur.cx - prev.cx;
                const float dy = cur.cy - prev.cy;
                const float dz = cur.cz - prev.cz;
                const float d  = std::sqrt(dx*dx + dy*dy + dz*dz);
                INFO("penca ", i, " respecto a la ", i - 1, ": dist=", d,
                     " tope=", TOPE);
                CHECK(d <= TOPE);
                prev = cur;
            }
        }
    }
}

TEST_CASE("la mata crece HACIA ARRIBA, no hacia el suelo") {
    // Una Opuntia crece contra la gravedad: las pencas nuevas salen del filo
    // superior. Sin el sesgo vertical la mata se derramaba hacia abajo tantas
    // veces como hacia arriba.
    //
    // Se comprueba en conjunto, no penca a penca: alguna puede salir de lado
    // (eso es lo que da la silueta arbustiva), pero la ULTIMA de la cadena
    // tiene que quedar por encima de la primera en la mayoria de los casos.
    int suben = 0, total = 0;
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        for (int x = -3; x <= 3; ++x) {
            const Nopal::Extra primera = Nopal::colocar(b, 0, x * 41, 70, 0);
            const Nopal::Extra ultima  =
                Nopal::colocar(b, (int)Nopal::MAX_EXTRAS - 1, x * 41, 70, 0);
            if (ultima.cy >= primera.cy) ++suben;
            ++total;
        }
    }
    // Con el sesgo del 45% hacia +Y mas la primera forzada a subir, la inmensa
    // mayoria tiene que crecer hacia arriba.
    INFO("suben ", suben, " de ", total);
    CHECK(suben * 100 >= total * 70);
}

TEST_CASE("la primera penca siempre arranca subiendo") {
    // La que enlaza con la penca base. Si arrancara de lado, toda la mata
    // quedaria tumbada desde el primer eslabon.
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(4, sem);
        for (int x = -3; x <= 3; ++x) {
            const Nopal::Extra e = Nopal::colocar(b, 0, x * 29, 64, x * 11);
            CHECK(e.dir == 2);          // +Y
            CHECK(e.cy >= 0.5f);        // por encima del centro de la celda
        }
    }
}

TEST_CASE("las pencas se afinan hacia la punta de la cadena") {
    // Un nopal real se estrecha hacia arriba: los cladodios jovenes del extremo
    // son menores que los de la base. Sin esto la mata parece un monton de
    // piezas del mismo tamano.
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        const Nopal::Extra primera = Nopal::colocar(b, 0, 13, 66, -7);
        const Nopal::Extra ultima  =
            Nopal::colocar(b, (int)Nopal::MAX_EXTRAS - 1, 13, 66, -7);
        INFO("semilla ", sem, ": primera=", primera.escala,
             " ultima=", ultima.escala);
        CHECK(ultima.escala <= primera.escala);
    }
}

TEST_CASE("el giro de la cadena no da saltos bruscos") {
    // Dos pencas encadenadas no pueden quedar cruzadas en angulo recto: la
    // union se veria como un aspa en vez de una planta. El giro hereda el de la
    // madre y solo se le suma un ruido corto, asi que el salto entre
    // consecutivas esta acotado.
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        Nopal::Extra prev = Nopal::colocar(b, 0, 5, 72, 5);
        for (int i = 1; i < (int)Nopal::MAX_EXTRAS; ++i) {
            const Nopal::Extra cur = Nopal::colocar(b, i, 5, 72, 5);
            const float salto = std::fabs(cur.anguloY - prev.anguloY);
            INFO("salto de giro entre ", i - 1, " y ", i, ": ", salto);
            // 0.6 de apertura programada + hasta 0.9 de ruido = 1.5 de tope.
            CHECK(salto <= 1.55f);
            prev = cur;
        }
    }
}

TEST_CASE("un nopal sin pencas extra sigue siendo valido") {
    // El caso base: una penca sola. No es un estado invalido, es el punto de
    // partida de la mata.
    const BlockType b = Nopal::nuevo(0);
    CHECK(Nopal::esNopal(b));
    CHECK(Nopal::extrasDe(b) == 0);
    CHECK(puedeDibujarse(b));
}


TEST_CASE("la cadena CABE en la celda sin aplastarse contra el techo") {
    // ⭐⭐ ESTE TEST NACE DE UN FALLO QUE LOS DEMAS NO VIERON.
    //
    // La primera version de la cadena usaba el semieje del mesher (0.42) como
    // si fuera el alto COMPLETO de la penca. El alto real es el doble, asi que
    // siete eslabones median mas de tres celdas: el acotado final los aplastaba
    // todos contra el techo y la mata se veia como un amasijo pegado arriba.
    //
    // Lo grave es que los otros tests PASABAN. "Caen dentro de la celda" pasaba
    // precisamente PORQUE el acotado los metia a la fuerza, y la distancia entre
    // consecutivas pasaba porque todas acababan en el mismo sitio. Un test que
    // mide el sintoma equivocado da falsa seguridad.
    //
    // Lo que se vigila aqui es la SATURACION: cuantas pencas acaban pegadas al
    // limite del acotado. Si la cadena cabe de verdad, casi ninguna.
    int saturadas = 0, total = 0;
    for (uint16_t sem = 0; sem < 8; ++sem) {
        const BlockType b = Nopal::nuevo(Nopal::MAX_EXTRAS, sem);
        for (int x = -3; x <= 3; ++x)
        for (int z = -3; z <= 3; ++z)
        for (int i = 0; i < (int)Nopal::MAX_EXTRAS; ++i) {
            const Nopal::Extra e = Nopal::colocar(b, i, x * 31, 68, z * 19);
            ++total;
            const bool pegada =
                (e.cy >= 0.879f) || (e.cy <= 0.121f) ||
                (e.cx >= 0.879f) || (e.cx <= 0.121f) ||
                (e.cz >= 0.879f) || (e.cz <= 0.121f);
            if (pegada) ++saturadas;
        }
    }
    INFO("saturadas ", saturadas, " de ", total);
    // Menos del 10%. Antes del arreglo era el 85%.
    CHECK(saturadas * 10 <= total);
}


// ============================================================================
// EL ITEM DE LA PENCA USA EL DIBUJO DE ITEM, NO EL DE LA PLANTA
// ============================================================================
// PEDIDO: "que el bloque de penca que crece en el nopal completo solo de el
// item de Items/...suelto.png y no el de Blocks/Penca de Nopal de Castilla.png".
//
// Son DOS dibujos distintos del resourcepack y es facil cruzarlos:
//
//   Blocks/Penca de Nopal de Castilla.png          la penca VIVA, en la mata
//   Items/Penca de Nopal de Castilla suelto.png    la penca CORTADA, recortada
//
// Se comprueba leyendo el propio main.cpp, porque el motor no se enlaza con los
// tests (ver el hallazgo I3 de la auditoria).
// ============================================================================

TEST_CASE("la mata de nopal suelta el item de penca suelta") {
    const std::string src = leerFuenteMotor();
    REQUIRE_FALSE(src.empty());

    // getDroppedItem tiene que desviar FAM_NOPAL al cladodio ANTES del switch,
    // que solo conoce IDs del enum. Sin eso la mata no soltaba nada util.
    CHECK(src.find("Compuesto::Nopal::esNopal(brokenBlock)") != std::string::npos);
}

TEST_CASE("el cladodio tiene textura de ITEM registrada") {
    const std::string src = leerFuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Si esta fila desaparece, el icono vuelve a caer en getBlockTexture y se
    // dibuja con la textura de la PLANTA: justo el bug que esto cierra.
    CHECK(src.find("BLOCK_NOPAL_CLADODIO, \"Penca de Nopal de Castilla suelto.png\"")
          != std::string::npos);
}

TEST_CASE("el BLOQUE usa piel teselable y el ITEM su dibujo recortado") {
    // ⭐ SON DOS TEXTURAS DISTINTAS, Y CADA UNA DONDE LE TOCA.
    //
    //   BLOQUE colocado  "Tallo de Nopal de Castilla.png"
    //       Piel de cactus teselable: 256x256 y 100% OPACA. Es lo que necesita
    //       un volumen -- se repite por las seis caras sin costura y no hay
    //       nada que recortar, asi que no puede salir con agujeros ni con el
    //       borde dentado.
    //
    //   ITEM en la mano  "Penca de Nopal de Castilla suelto.png"
    //       La foto de una penca entera sobre fondo transparente. Sirve para
    //       una silueta plana; sobre un bloque 3D su contorno difuminado lo
    //       cortaba el alpha test en seco y dejaba el borde roto.
    //
    // Cruzarlas es el error facil, y por eso se fija aqui.
    const std::string src = leerFuenteMotor();
    REQUIRE_FALSE(src.empty());

    // El bloque pide la piel teselable.
    CHECK(src.find("getTexture(\"Tallo de Nopal de Castilla.png\")")
          != std::string::npos);

    // Y el item conserva su dibujo (la fila de la tabla de items).
    CHECK(src.find("BLOCK_NOPAL_CLADODIO, \"Penca de Nopal de Castilla suelto.png\"")
          != std::string::npos);
}


// ============================================================================
// LAS CINCO PENCAS SE DIBUJAN IGUAL DE GRANDES
// ============================================================================
// PEDIDO: "hazlo eso a todos los bloques e items de penca de nopal".
//
// El cladodio, la penca mojada, la seca, la desbabada y la cortada en tiras son
// LA MISMA PIEZA en distintos momentos de su vida. Lavar una penca no puede
// cambiar lo que mide: si una se quedara fuera del grupo, encogeria a la mitad
// delante del jugador al cambiar de estado.
// ============================================================================

TEST_CASE("las cinco formas de la penca cuentan como penca") {
    CHECK(esPencaDeNopal(BLOCK_NOPAL_CLADODIO));
    CHECK(esPencaDeNopal(BLOCK_NOPAL_MOJADO));
    CHECK(esPencaDeNopal(BLOCK_NOPAL_SECO));
    CHECK(esPencaDeNopal(BLOCK_NOPAL_SIN_BABA));
    CHECK(esPencaDeNopal(BLOCK_NOPAL_TIRAS));
}

TEST_CASE("lo que NO es penca se queda fuera") {
    // La baba, las espinas y las tunas comparten el grosor carnoso pero NO el
    // tamano: una tuna del tamano de la casilla seria una sandia.
    CHECK_FALSE(esPencaDeNopal(BLOCK_NOPAL_BABA));
    CHECK_FALSE(esPencaDeNopal(BLOCK_ESPINAS_NOPAL));
    CHECK_FALSE(esPencaDeNopal(BLOCK_TUNA));
    CHECK_FALSE(esPencaDeNopal(BLOCK_TUNA_ROJA));
    CHECK_FALSE(esPencaDeNopal(BLOCK_TUNA_AMARILLA));
    // Y nada ajeno al nopal.
    CHECK_FALSE(esPencaDeNopal(BLOCK_STONE));
    CHECK_FALSE(esPencaDeNopal(BLOCK_MAGUEY_PUNTA));
}

TEST_CASE("toda penca es ademas cactus suelto") {
    // `esCactusSuelto` es quien decide el GROSOR carnoso. Si una penca se
    // saliera de ese grupo se dibujaria fina como una lamina de pedernal,
    // aunque conservara su tamano.
    const BlockType PENCAS[] = {
        BLOCK_NOPAL_CLADODIO, BLOCK_NOPAL_MOJADO, BLOCK_NOPAL_SECO,
        BLOCK_NOPAL_SIN_BABA, BLOCK_NOPAL_TIRAS
    };
    for (BlockType b : PENCAS) {
        CHECK(esPencaDeNopal(b));
        CHECK(esCactusSuelto(b));
    }
}

TEST_CASE("las cinco pencas reciben el tamano grande en el motor") {
    // Se lee main.cpp porque no se enlaza con los tests. Lo que se vigila es
    // que el tamano lo decida `esPencaDeNopal` y no una lista escrita a mano,
    // que es como se olvida una al anadir un estado nuevo.
    const std::string src = leerFuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("esPencaDeNopal(item.blockType)) scale =") != std::string::npos);
}
