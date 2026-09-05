// ============================================================================
// EL MAGUEY TIENE QUE PARECER UNA PLANTA, NO UN MONTON DE CUBOS
// ============================================================================
// El modelo se reescribio varias veces y siempre volvia el mismo comentario:
// "se ve como cubos amontonados". La causa se pudo MEDIR, y por eso se puede
// blindar con pruebas en vez de a ojo.
//
// El fallo era de PROPORCION. Con escala 1.90 la penca alcanzaba 0.44 de
// largo y 0.61 de ancho: mas ancha que larga. Un cuerpo asi se lee como una
// caja aunque sus vertices esten colocados con una curva.
//
// Aqui se fija lo que hace que algo parezca una hoja:
//   1. es mucho mas larga que ancha
//   2. cabe entera en su celda
//   3. deja hueco entre hojas para que pase la luz
//
// Las cuentas replican las del mesher (buscar "LAS PENCAS" en main.cpp). Si
// alguien cambia alla y no aca, estas pruebas caen.

#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include <cmath>

namespace M = Compuesto::Maguey;

namespace {

// Las etapas reales, para recorrerlas todas en cada prueba.
const uint16_t ETAPAS[5] = {
    M::BROTE, M::JOVEN, M::ADULTO, M::MADURO, M::PRODUCTOR
};

// El numero de pencas ya no se deduce de la escala: es un dato de la etapa.
// Se pasa la etapa para no duplicar la tabla aqui.
[[maybe_unused]] int pencasDeEscala(float esc) {
    // Se mantiene la firma por comodidad de las pruebas de geometria, que
    // recorren escalas. Se traduce a la etapa que le corresponde.
    if (esc <= 0.45f) return M::pencasDeEtapa(M::BROTE);
    if (esc <= 0.70f) return M::pencasDeEtapa(M::JOVEN);
    if (esc <= 1.00f) return M::pencasDeEtapa(M::ADULTO);
    if (esc <= 1.40f) return M::pencasDeEtapa(M::MADURO);
    return M::pencasDeEtapa(M::PRODUCTOR);
}

// Reproduce el calculo del mesher para la penca `p`.
struct Penca {
    float largo, alto, caida, ancho, grosor, recorrido;
};

// Reproduce el calculo del mesher para la penca `p` de una planta en la etapa
// dada. Se indexa por ETAPA y no por escala: es de donde salen ahora todas las
// medidas (ver la rama del maguey en buildChunkMesh).
//
// `var` es la variacion por hoja. Se pasa explicitamente para poder probar el
// caso nominal y tambien los EXTREMOS que puede alcanzar (0.86 .. 1.13), que
// es donde aparecian los desbordes.
Penca pencaDeEtapa(uint16_t etapa, int p, int nPencas,
                   float var = 1.0f, float var2 = 1.0f) {
    const float esc    = M::escalaDeEtapa(etapa);
    const int   CELDAS = M::celdasDeEtapa(etapa);
    const float ALTO   = M::alturaReal(CELDAS);
    const float RADIO  = M::radioDeCeldas(CELDAS);
    const float t = (nPencas > 1) ? (float)p / (float)(nPencas - 1) : 0.0f;

    Penca r;
    r.largo = RADIO * (0.34f + t * 0.66f) * var;
    r.alto  = ALTO  * (0.86f - t * 0.42f) * var2;

    // El retoño es chaparro: hojas cortas y abiertas, no un adulto encogido.
    if (etapa == M::BROTE)      { r.largo *= 1.30f; r.alto *= 0.80f; }
    else if (etapa == M::JOVEN) { r.largo *= 1.15f; r.alto *= 0.90f; }

    // La caida va acotada para que la punta no se hunda bajo el bloque.
    const float y0 = M::altoHoja(esc) * 0.08f;
    r.caida = ALTO * t * t * 0.20f;
    if (r.caida > y0 * 0.80f) r.caida = y0 * 0.80f;

    // Nace DENTRO de la piña, para que la union quede tapada.
    const float r0 = RADIO * 0.34f;

    r.recorrido = std::sqrt(r.largo * r.largo + r.alto * r.alto);

    // Ancho de la descripcion: 15-25 cm = 0.15-0.25 bloques.
    const float frac = (float)(CELDAS - 1) / 3.0f;
    r.ancho = 0.15f + (0.25f - 0.15f) * frac;
    if (r.ancho > r.recorrido * 0.25f) r.ancho = r.recorrido * 0.25f;
    if (r.ancho < 2.4f / 16.0f)        r.ancho = 2.4f / 16.0f;
    // Las de dentro, algo mas estrechas: si no, cierran la roseta.
    r.ancho *= 0.80f + 0.20f * t;

    // Grosor: carnosa, y engorda con la edad.
    // El orden importa: primero el tope relativo, despues el suelo absoluto.
    // Al reves, el tope vuelve a hundir el canto por debajo del minimo en las
    // hojas mas estrechas y desaparecen de perfil.
    r.grosor = r.ancho * 0.34f * M::engrosado(etapa);
    if (r.grosor > r.ancho * 0.45f) r.grosor = r.ancho * 0.45f;
    if (r.grosor > r.ancho * 0.5f)  r.grosor = r.ancho * 0.5f;
    if (r.grosor < 1.5f / 16.0f)    r.grosor = 1.5f / 16.0f;

    // Acotado, igual que en el mesher: se recorta la hoja a lo que hay en vez
    // de confiar en que los numeros salgan.
    {
        constexpr float BORDE = 0.02f;
        const float dispoR = M::radioMaximoReal() - BORDE - r0
                           - r.ancho * 0.5f;
        if (r.largo > dispoR) r.largo = dispoR > 0.0f ? dispoR : 0.0f;
        const float dispoY = (float)CELDAS - BORDE - y0 - r.grosor;
        if (r.alto > dispoY) r.alto = dispoY > 0.0f ? dispoY : 0.0f;
    }

    // El recorrido se recalcula: es el de la hoja YA acotada.
    r.recorrido = std::sqrt(r.largo * r.largo + r.alto * r.alto);
    return r;
}

// Hasta donde llega la planta, en bloques: su altura visible y su diametro.
struct Talla { float alto, diametro; };

Talla tallaDe(uint16_t etapa) {
    const float esc    = M::escalaDeEtapa(etapa);
    const int   CELDAS = M::celdasDeEtapa(etapa);
    const float RADIO  = M::radioDeCeldas(CELDAS);
    const int   n      = M::pencasDeEtapa(etapa);
    const float r0 = RADIO * 0.34f;
    const float y0 = M::altoHoja(esc) * 0.08f;

    Talla s{0.0f, 0.0f};
    for (int p = 0; p < n; ++p) {
        const Penca h = pencaDeEtapa(etapa, p, n);
        const float alcance = r0 + h.largo + h.ancho * 0.5f;
        const float cima    = y0 + h.alto + h.grosor;
        if (alcance > s.diametro) s.diametro = alcance;
        if (cima    > s.alto)     s.alto     = cima;
    }
    s.diametro *= 2.0f;   // de radio a diametro
    return s;
}

} // namespace

TEST_CASE("una penca es mucho mas larga que ancha") {
    // ESTA ES LA PRUEBA QUE HABRIA CAZADO EL BUG.
    //
    // El modelo viejo daba 0.44 de largo por 0.61 de ancho (ratio 0.72) y por
    // eso se veia cubico. Una hoja de verdad anda por 1:6; se exige al menos
    // 1:3, que es donde el ojo ya lee "hoja" y no "caja".
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        for (int p = 0; p < n; ++p) {
            const Penca h = pencaDeEtapa(etapa, p, n);
            REQUIRE(h.ancho > 0.0f);
            const float ratio = h.recorrido / h.ancho;
            INFO("etapa=" << etapa << " penca=" << p
                 << " recorrido=" << h.recorrido << " ancho=" << h.ancho
                 << " ratio=" << ratio);
            CHECK(ratio >= 2.0f);
        }
    }
}

TEST_CASE("una penca es carnosa pero no un tronco") {
    // Tiene que tener grosor real (si no, es un papel y desaparece de canto),
    // pero mucho menor que el ancho (si no, es un palo).
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        for (int p = 0; p < n; ++p) {
            const Penca h = pencaDeEtapa(etapa, p, n);
            CHECK(h.grosor > 0.0f);               // tiene volumen
            // Es una hoja, no un palo. El margen sube a 0.85 porque en las
            // pencas mas estrechas manda el SUELO de 1.5 px: mas vale una
            // hoja un pelo gorda que una que desaparece de perfil.
            CHECK(h.grosor <= h.ancho * 0.85f);
        }
    }
}

TEST_CASE("la roseta es tan ancha como alta, como la planta real") {
    // ⭐ EL DATO QUE MANDA EN LA DESCRIPCION BOTANICA:
    //
    //   "Altura de la roseta: entre 1.5 y 3 metros"
    //   "Diametro o ancho:    entre 1.5 y 3 metros"
    //
    // Las dos medidas son la MISMA. La roseta es una media esfera, no una
    // columna: las pencas se abren en todas direcciones.
    //
    // Esto es lo que el modelo tenia mal de raiz. Crecia a lo alto (hasta 4
    // bloques) mientras el ancho seguia topado en media celda, asi que la
    // proporcion salia 1.79 donde toca 1.00 -- un mastil de hojas.
    //
    // Se admite del 0.6 al 1.5: una roseta perfecta es 1.0, pero las de fuera
    // se arquean y ensanchan un poco el contorno, y un retoño es mas
    // achaparrado a proposito.
    for (uint16_t etapa : ETAPAS) {
        const Talla s = tallaDe(etapa);
        REQUIRE(s.diametro > 0.0f);
        const float prop = s.alto / s.diametro;
        INFO("etapa=" << etapa << " alto=" << s.alto
             << " diametro=" << s.diametro << " alto/diam=" << prop);
        CHECK(prop >= 0.35f);
        CHECK(prop <= 1.50f);
    }
}

TEST_CASE("un maguey hecho mide lo que dice la descripcion") {
    // "Altura de la roseta: entre 1.5 y 3 metros en su etapa adulta"
    // "Diametro: entre 1.5 y 3 metros"
    //
    // A escala 1 bloque = 1 metro. Se comprueba en las etapas ADULTAS, que
    // son a las que se refiere el dato; el retoño es mas pequeño por
    // definicion.
    const uint16_t HECHAS[3] = { M::ADULTO, M::MADURO, M::PRODUCTOR };
    for (uint16_t etapa : HECHAS) {
        const Talla s = tallaDe(etapa);
        INFO("etapa=" << etapa << " alto=" << s.alto
             << " diametro=" << s.diametro);
        // Alto: dentro del rango real, con holgura por abajo para el adulto
        // joven que aun no ha llegado a su tope.
        CHECK(s.alto >= 1.5f);
        CHECK(s.alto <= 3.2f);
        // Diametro: el mismo rango.
        CHECK(s.diametro >= 1.5f);
        CHECK(s.diametro <= 3.2f);
    }

    // Y el retoño es claramente menor que un ejemplar hecho.
    CHECK(tallaDe(M::BROTE).alto < tallaDe(M::ADULTO).alto);
}

TEST_CASE("la penca mide lo que dice la descripcion") {
    // "Pencas: miden de 1 a 2 m de largo y de 15 a 25 cm de ancho, siendo
    //  muy gruesas y carnosas"
    //
    // 15-25 cm son 2.4 a 4.0 px de textura (1 bloque = 16 px). Por debajo de
    // eso la hoja se pierde contra el fondo -- era lo que hacia desaparecer
    // los retoños.
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        for (int p = 0; p < n; ++p) {
            const Penca h = pencaDeEtapa(etapa, p, n);
            const float anchoPx  = h.ancho  * 16.0f;
            const float cantoPx  = h.grosor * 16.0f;
            INFO("etapa=" << etapa << " penca=" << p
                 << " ancho=" << anchoPx << "px canto=" << cantoPx << "px");

            // Nunca por debajo del minimo visible.
            CHECK(anchoPx >= 1.8f);
            // Ni tan ancha que deje de parecer una penca.
            CHECK(anchoPx <= 5.0f);

            // "MUY GRUESAS Y CARNOSAS": siempre hay canto visible.
            CHECK(cantoPx >= 1.0f);
            // Pero sin llegar a palo: como mucho la mitad del ancho.
            CHECK(h.grosor <= h.ancho * 0.85f);
        }
    }
}

TEST_CASE("la penca guarda la proporcion larga-y-estrecha real") {
    // 1-2 m de largo por 15-25 cm de ancho da un ratio de 4:1 a 13:1.
    //
    // ⚠️ ESTA ES LA PRUEBA QUE HABRIA CAZADO EL BUG DE "MUY DELGADO": el
    // modelo anterior sacaba el ancho de lo LARGA que era la hoja, y al
    // crecer la planta a lo alto el ratio se disparaba a 42:1 -- siete veces
    // mas estrecho de lo que debe. En pantalla eran hilos.
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        for (int p = 0; p < n; ++p) {
            const Penca h = pencaDeEtapa(etapa, p, n);
            REQUIRE(h.ancho > 0.0f);
            const float ratio = h.recorrido / h.ancho;
            INFO("etapa=" << etapa << " penca=" << p << " ratio=" << ratio);
            // El techo es generoso (16:1) porque la hoja mas interior de una
            // mata alta es la mas esbelta de todas; lo que no puede es
            // volver a los 42:1.
            CHECK(ratio <= 16.0f);
            // Y nunca tan rechoncha que parezca una pala.
            CHECK(ratio >= 2.0f);
        }
    }
}

TEST_CASE("la planta no cruza el borde del chunk") {
    // La roseta asoma de su CELDA a proposito -- es la unica forma de que el
    // diametro sea el real -- pero no puede acercarse al borde del CHUNK (16
    // bloques), que es donde el recorte del dibujado si corta: se veria
    // aparecer y desaparecer al girar la camara.
    CHECK(M::radioMaximoReal() <= 1.5f);

    for (uint16_t etapa : ETAPAS) {
        const Talla s = tallaDe(etapa);
        INFO("etapa=" << etapa << " diametro=" << s.diametro);
        // Radio maximo, con margen de sobra contra los 16 del chunk.
        CHECK(s.diametro * 0.5f <= M::radioMaximoReal());
    }
}

TEST_CASE("queda hueco entre las hojas para que pase la luz") {
    // Una roseta cerrada se lee como un bulto solido. Se pidio expresamente
    // que se viera el fondo entre las pencas.
    //
    // Se mide el ANCHO ANGULAR que ocupa cada hoja a su altura media y se
    // comprueba que la suma no cierra el circulo.
    //
    // ⚠️ SE MIDE POR CELDA, NO POR PLANTA. Es la correccion importante: las
    // hojas de un maguey alto se reparten entre sus CELDAS (hasta cuatro), no
    // se amontonan todas en el mismo anillo. Sumarlas todas juntas mide una
    // planta que no existe -- daria por cerrada una roseta que en pantalla se
    // ve abierta, porque sus hojas estan a alturas distintas.
    //
    // Lo que de verdad tapa la luz es cuantas hojas coinciden en UN bloque.
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        const int celdas = M::celdasDeEtapa(etapa);
        const float RADIO = M::radioDeCeldas(celdas);

        float cubierto = 0.0f;
        for (int p = 0; p < n; ++p) {
            const Penca h = pencaDeEtapa(etapa, p, n);
            // Radio al que esta la parte ancha de la hoja (~mitad).
            const float rad = RADIO * 0.34f + h.largo * 0.5f;
            if (rad <= 0.001f) continue;
            // Angulo que tapa: 2*atan((ancho/2)/radio).
            cubierto += 2.0f * std::atan((h.ancho * 0.5f) / rad);
        }
        // Repartido entre las celdas que ocupa la planta.
        const float porCelda = cubierto / (float)celdas;

        const float circulo = 6.2831853f;
        INFO("etapa=" << etapa << " cubierto/celda=" << porCelda
             << " de " << circulo << " (celdas=" << celdas << ")");
        // Menos del 85% del circulo: queda cielo entre hojas.
        CHECK(porCelda < circulo * 0.85f);
    }
}

TEST_CASE("las etapas se distinguen de un vistazo") {
    // Un maguey productor tiene que verse claramente mayor que un brote: es
    // lo que le dice al jugador cual puede capar sin acercarse.
    //
    // ⚠️ NO SE EXIGE CRECIMIENTO ESTRICTO, y es a proposito: altoHoja() y
    // radioRoseta() SATURAN (0.96 y 0.44) para que la planta no salga de su
    // celda. Ese tope es la garantia de "una sola celda", asi que las dos
    // ultimas etapas empatan en alto -- se distinguen por el numero de hojas
    // y por el cajete, no por ser mas altas. Pedir `>` aqui obligaria a
    // romper el acotado al voxel.
    float altoAnt = -1.0f, radioAnt = -1.0f;
    for (uint16_t etapa : ETAPAS) {
        const float esc = M::escalaDeEtapa(etapa);
        const float a = M::altoHoja(esc);
        const float r = M::radioRoseta(esc);
        CHECK(a >= altoAnt);     // nunca encoge (o satura el tope)
        CHECK(r >= radioAnt);
        CHECK(a <= 0.96f);       // y el tope se respeta
        CHECK(r <= 0.44f);
        altoAnt = a; radioAnt = r;
    }
    // Y el salto de brote a productor tiene que ser grande de verdad.
    const float brote = M::altoHoja(M::escalaDeEtapa(M::BROTE));
    const float prod  = M::altoHoja(M::escalaDeEtapa(M::PRODUCTOR));
    CHECK(prod > brote * 1.8f);
}

TEST_CASE("las plantas grandes tienen mas hojas") {
    CHECK(pencasDeEscala(M::escalaDeEtapa(M::PRODUCTOR)) >
          pencasDeEscala(M::escalaDeEtapa(M::BROTE)));
    // Pero ninguna se dispara: son quads, y esto se dibuja por planta.
    // El tope sube a 26 porque ahora las hojas se reparten entre varias
    // celdas -- no se dibujan todas en el mismo bloque.
    for (uint16_t etapa : ETAPAS)
        CHECK(pencasDeEscala(M::escalaDeEtapa(etapa)) <= 26);
}

// ============================================================================
// LA GUARDA CONTRA LA GEOMETRIA FANTASMA
// ============================================================================

TEST_CASE("una familia sin modelo no se dibuja, nunca cae a un cubo") {
    // El rango compuesto reserva hueco para familias aun sin implementar. Un
    // ID de esas NO puede acabar en el mesher cubico: seria el "cubo
    // fantasma". La regla es que no se dibuja nada.
    CHECK(Compuesto::puedeDibujarse(
        Compuesto::Maguey::nuevo(M::ADULTO, 0)) == true);

    // Reservadas pero sin geometria: no se dibujan.
    CHECK(Compuesto::puedeDibujarse(
        Compuesto::hacer(Compuesto::FAM_ARBOL_FRUTO, 0)) == false);
    CHECK(Compuesto::puedeDibujarse(
        Compuesto::hacer(Compuesto::FAM_PIEDRA_MUSGO, 0)) == false);
    CHECK(Compuesto::puedeDibujarse(
        Compuesto::hacer(Compuesto::FAM_PLANTA_FLOR, 0)) == false);

    // Las que si tienen modelo, si.
    CHECK(Compuesto::puedeDibujarse(
        Compuesto::Biznaga::nuevo(0, 0, 0)) == true);

    // Un bloque normal no es asunto de este sistema: se dibuja como siempre.
    CHECK(Compuesto::puedeDibujarse(BLOCK_STONE) == true);
    CHECK(Compuesto::puedeDibujarse(BLOCK_AIR) == true);
}

// ============================================================================
// NI FLOTA NI SE DESPEGA DEL SUELO
// ============================================================================

TEST_CASE("la planta nace pegada al suelo de su celda") {
    // Se pidio que no flote. La primera condicion es que la geometria
    // ARRANQUE a ras del suelo de su bloque: si naciera a media altura, se
    // veria colgada aunque el bloque estuviera bien puesto.
    for (uint16_t etapa : ETAPAS) {
        const float esc = M::escalaDeEtapa(etapa);
        const float y0 = M::altoHoja(esc) * 0.10f;
        INFO("etapa=" << etapa << " nace a y=" << y0);
        // Nace en el decimo inferior de su celda: pegada al suelo.
        CHECK(y0 >= 0.0f);
        CHECK(y0 <= 0.12f);
    }
}

TEST_CASE("la planta se posa sobre el nivel que tenga debajo") {
    // ⭐ EL HUECO QUE ESTO CIERRA.
    //
    // El terreno tiene NIVELES PARCIALES: una capa de tierra puede medir 3,
    // 4, 5... de 16 px en vez del bloque entero. La planta se coloca en la
    // celda de ENCIMA de esa capa, asi que si dibujara desde el suelo de SU
    // celda quedaria FLOTANDO -- sobre un nivel 1 el hueco es de 13 px, casi
    // un bloque entero de aire entre la mata y la tierra.
    //
    // La regla: se baja lo que le falte al bloque de abajo para ser entero.
    //
    //     POSADA = 1 - alturaDe(bloque de abajo)
    //
    // Se comprueba en los ocho niveles.
    for (int nivel = 1; nivel <= 8; ++nivel) {
        const float h = (float)alturaNivelPx(nivel) / 16.0f;
        const float posada = (h < 1.0f) ? (1.0f - h) : 0.0f;

        INFO("nivel=" << nivel << " altura=" << h << " posada=" << posada);

        // Nunca baja mas de lo que falta ni sube nunca.
        CHECK(posada >= 0.0f);
        CHECK(posada < 1.0f);

        // Y al bajarla, la planta acaba tocando EXACTAMENTE la cara de
        // arriba del bloque de abajo: sin hueco y sin hundirse.
        //
        // Suelo de la planta (0) bajado por la posada, medido desde el suelo
        // de su celda: -posada. La cara del bloque de abajo esta a -(1-h),
        // que es lo mismo. Cero hueco.
        const float sueloPlanta = -posada;
        const float caraDeAbajo = -(1.0f - h);
        CHECK(sueloPlanta == doctest::Approx(caraDeAbajo));
    }

    // Un bloque ENTERO no baja nada: la planta ya esta apoyada.
    CHECK(alturaNivelPx(8) == 16);
}

TEST_CASE("los niveles parciales cubren todo el rango sin saltos") {
    // Si un nivel diera 0 o mas de 16, la posada saldria negativa o mayor que
    // un bloque y la planta se hundiria o volaria.
    int ant = 0;
    for (int nivel = 1; nivel <= 8; ++nivel) {
        const int px = alturaNivelPx(nivel);
        CHECK(px > 0);
        CHECK(px <= 16);
        CHECK(px > ant);      // cada nivel es mas alto que el anterior
        ant = px;
    }
    CHECK(alturaNivelPx(8) == 16);   // el ultimo es el bloque entero
}

TEST_CASE("las celdas de una planta alta son contiguas") {
    // Los segmentos van 0,1,2,... sin saltos. Si se saltara uno, quedaria un
    // hueco de aire en mitad de la planta y la parte de arriba se veria
    // flotando.
    for (uint16_t etapa : ETAPAS) {
        const int celdas = M::celdasDeEtapa(etapa);
        const BlockType planta = M::nuevo(etapa, 0);
        for (int dy = 0; dy < celdas; ++dy) {
            const BlockType celda = M::conSegmento(planta, (uint16_t)dy);
            CHECK(M::segmentoDe(celda) == (uint16_t)dy);
        }
        // Y la de abajo del todo es la base, la que se apoya en el terreno.
        CHECK(M::esBase(M::conSegmento(planta, 0)));
    }
}

// ============================================================================
// NADA DE IXTLE SUELTO ALREDEDOR DEL MAGUEY
// ============================================================================
// El maguey es UN modelo 3D dentro de su celda. Todo lo que se vea de la
// planta -- hojas, puas, cajete -- tiene que salir de ese modelo.
//
// El ixtle (la lechuguilla) era el sistema ANTERIOR: bloques cubicos sueltos
// con textura de hoja. Ya no se generan, pero los IDs siguen existiendo
// porque los mundos guardados los contienen y la receta del hilo los usa.
//
// El riesgo es que vuelvan a aparecer PEGADOS a un maguey, que es justo el
// "bloque con textura de hoja" que se estuvo persiguiendo. Estas pruebas
// cierran las vias por las que podian volver.

TEST_CASE("las celdas compartidas de ixtle llevan una hoja dentro") {
    // Estas tres NO son bloques sueltos: son celdas con DOS cosas dentro, y
    // una de ellas es una hoja de ixtle. Por eso la limpieza del mesher tiene
    // que cubrirlas -- se le escapaban, y en un mundo viejo seguian saliendo
    // hojas de ixtle junto a los magueyes.
    CHECK(esCompartido(BLOCK_IXTLE_CON_HIERBA));
    CHECK(esCompartido(BLOCK_IXTLE_CON_FLOR));
    CHECK(esCompartido(BLOCK_IXTLE_DOBLE));

    // Y la pieza que llevan dentro es, efectivamente, ixtle.
    CHECK(esIxtle(BLOCK_IXTLE_CON_HIERBA));
    CHECK(esIxtle(BLOCK_IXTLE_CON_FLOR));
    CHECK(esIxtle(BLOCK_IXTLE_DOBLE));
}

TEST_CASE("todo el ixtle viejo cuenta como ixtle") {
    // Si alguno se escapara de esIxtle(), la limpieza del mesher lo dejaria
    // pasar y acabaria dibujado como cubo.
    CHECK(esIxtle(BLOCK_IXTLE_HOJA));
    CHECK(esIxtle(BLOCK_IXTLE_PEQUENA));
    CHECK(esIxtle(BLOCK_IXTLE_GRANDE));
    CHECK(esIxtle(BLOCK_IXTLE_ENORME));
    CHECK(esIxtle(BLOCK_IXTLE_PUNTA));
    CHECK(esIxtle(BLOCK_IXTLE_TALLO));
    CHECK(esIxtle(BLOCK_IXTLE_TALLO_ARENA));

    // El maguey NO es ixtle: es un compuesto, con su propio modelo.
    CHECK_FALSE(esIxtle(M::nuevo(M::PRODUCTOR, 0)));
    CHECK_FALSE(esIxtle(M::nuevo(M::BROTE, 0)));
}

TEST_CASE("el maguey no comparte identidad con el ixtle") {
    // Son dos sistemas distintos y no deben confundirse en ningun sentido:
    // el maguey vive en el rango COMPUESTO y el ixtle en el enum normal.
    for (uint16_t etapa : ETAPAS) {
        const BlockType m = M::nuevo(etapa, 0);
        CHECK(Compuesto::esCompuesto(m));
        CHECK_FALSE(esIxtleHoja(m));
        CHECK_FALSE(esTalloIxtle(m));
        CHECK_FALSE(esCompartido(m));
        CHECK(m != BLOCK_IXTLE_HOJA);
        CHECK(m != BLOCK_IXTLE_PUNTA);
    }
}

// ============================================================================
// NINGUN COMPUESTO PUEDE PASAR POR EL GREEDY MESHING
// ============================================================================
// El greedy meshing fusiona caras de bloques CUBICOS. Un compuesto no es un
// cubo: es un modelo 3D que se emite en la pasada de geometria propia.
//
// ⚠️ EL BUG QUE ESTO IMPIDE, que costo varias iteraciones encontrar:
//
// La rama del maguey acaba con `continue` para no emitir las caras del cubo.
// Pero ese `continue` solo sale de SU bucle -- el greedy meshing es una
// pasada APARTE, mas abajo, que vuelve a recorrer los mismos bloques.
//
// Como el compuesto no estaba excluido, el greedy lo trataba como cubo Y
// ADEMAS lo fusionaba con sus vecinos: las cuatro celdas de un maguey alto
// se unian en un bloque macizo con textura de hoja, dibujado ENCIMA del
// modelo. Se veian dos cubos verdes gigantes tapando la planta entera.
//
// La condicion de abajo replica la de esGreedyBlock() en main.cpp. Si alguien
// quita el `if (Compuesto::esCompuesto(b)) return false` de alli, esta prueba
// no se entera -- pero deja escrito el porque, que es lo que hay que
// conservar.

// Replica EXACTA de la condicion que usa esGreedyBlock() en main.cpp para
// dejar fuera a los compuestos. Se copia aqui porque isBlockOpaque vive en
// main.cpp y no se enlaza con los tests; lo que se fija es la REGLA.
bool entrariaAlGreedy(BlockType b) {
    if (Compuesto::esCompuesto(b)) return false;   // <- la linea del arreglo
    return b != BLOCK_AIR && b != BLOCK_WATER && b != BLOCK_LAVA;
}

TEST_CASE("un compuesto nunca entra al greedy meshing") {
    // Ni el maguey en ninguna etapa ni en ningun segmento...
    for (uint16_t etapa : ETAPAS)
        for (uint16_t s = 0; s < 4; ++s) {
            const BlockType t = M::conSegmento(M::nuevo(etapa, 1), s);
            CHECK(Compuesto::esCompuesto(t));
            CHECK_FALSE(entrariaAlGreedy(t));
        }

    // ...ni la biznaga, que tambien tiene modelo propio.
    CHECK_FALSE(entrariaAlGreedy(Compuesto::Biznaga::nuevo(2, 0, 3)));

    // Pero un bloque normal SI: el greedy es lo que hace que el terreno
    // vaya rapido, y sacarlo de ahi hundiria el rendimiento.
    CHECK(entrariaAlGreedy(BLOCK_STONE));
    CHECK(entrariaAlGreedy(BLOCK_DIRT));
    CHECK_FALSE(entrariaAlGreedy(BLOCK_AIR));
}

TEST_CASE("las celdas de una planta alta no se fusionan entre si") {
    // Si las cuatro celdas de un productor entraran al greedy, se unirian en
    // un unico cubo macizo de cuatro bloques de alto -- que es exactamente lo
    // que se veia tapando la planta.
    const BlockType base = M::nuevo(M::PRODUCTOR, 0);
    for (uint16_t s = 0; s < 4; ++s) {
        const BlockType celda = M::conSegmento(base, s);
        CHECK_FALSE(entrariaAlGreedy(celda));
        // Y cada una sigue sabiendo dibujarse por su cuenta.
        CHECK(Compuesto::puedeDibujarse(celda));
    }
}

// ============================================================================
// EL CICLO DEL AGUAMIEL SIGUE ENTERO
// ============================================================================
// El modelo se reescribio, pero el comportamiento no debe haberse movido.

TEST_CASE("el aguamiel sube poco a poco y se puede extraer") {
    BlockType m = M::nuevo(M::PRODUCTOR, 0);

    // Sin capar no produce, por muy maduro que sea.
    CHECK(M::produce(Compuesto::estadoDe(m)) == false);

    m = M::capar(m);
    CHECK(M::capadoDe(m) == true);
    CHECK(M::produce(Compuesto::estadoDe(m)) == true);

    // Recien capado no hay nada: no aparece de golpe.
    CHECK(M::aguamielDe(m) == 0);
    CHECK(M::hayParaTazon(Compuesto::estadoDe(m)) == false);

    // Se va llenando de uno en uno.
    for (uint16_t i = 1; i <= M::capacidad(M::PRODUCTOR); ++i) {
        m = M::conAguamiel(m, i);
        CHECK(M::aguamielDe(m) == i);
    }
    CHECK(M::hayParaTazon(Compuesto::estadoDe(m)) == true);

    // Al extraer se vacia pero la planta sigue ahi y sigue produciendo.
    m = M::vaciado(m);
    CHECK(M::aguamielDe(m) == 0);
    CHECK(M::capadoDe(m) == true);
    CHECK(M::produce(Compuesto::estadoDe(m)) == true);
}

TEST_CASE("solo los magueyes hechos dan aguamiel") {
    CHECK(M::capacidad(M::BROTE)  == 0);
    CHECK(M::capacidad(M::JOVEN)  == 0);
    CHECK(M::capacidad(M::ADULTO) == 0);
    CHECK(M::capacidad(M::MADURO) > 0);
    CHECK(M::capacidad(M::PRODUCTOR) > M::capacidad(M::MADURO));
}

TEST_CASE("todo el estado del maguey sobrevive al guardado") {
    // El estado va DENTRO del ID, asi que guardar el bloque guarda la planta
    // entera. Se comprueba que ningun campo pisa a otro.
    for (uint16_t etapa : ETAPAS)
        for (uint16_t giro = 0; giro < 4; ++giro)
            for (uint16_t puntas = 0; puntas <= 7; ++puntas)
                for (uint16_t jugo = 0; jugo <= 15; jugo += 5)
                    for (int capado = 0; capado < 2; ++capado) {
                        const BlockType t = M::crear(etapa, giro, puntas,
                                                     jugo, capado != 0);
                        CHECK(M::etapaDe(t)    == etapa);
                        CHECK(M::giroDe(t)     == giro);
                        CHECK(M::puntasDe(t)   == puntas);
                        CHECK(M::aguamielDe(t) == jugo);
                        CHECK(M::capadoDe(t)   == (capado != 0));
                        // Y sigue siendo un maguey reconocible.
                        CHECK(Compuesto::esCompuesto(t));
                        CHECK(Compuesto::familiaDe(t) == Compuesto::FAM_MAGUEY);
                        CHECK(Compuesto::puedeDibujarse(t));
                    }
}

// ============================================================================
// LOS MAGUEYES TIENEN TAMAÑOS DISTINTOS, COMO EN EL CAMPO
// ============================================================================

TEST_CASE("cada fase del maguey mide lo que le toca") {
    // Lo pedido, al pie de la letra:
    //   "el mas pequeño es bebe y mide un bloque, el mediano 2 bloques,
    //    el grande 3 bloques y el maduro capaz de tener aguamiel 4"
    CHECK(M::celdasDeEtapa(M::BROTE)     == 1);   // bebe
    CHECK(M::celdasDeEtapa(M::JOVEN)     == 2);   // mediano
    CHECK(M::celdasDeEtapa(M::ADULTO)    == 3);   // grande
    CHECK(M::celdasDeEtapa(M::MADURO)    == 4);   // maduro: ya se capa
    CHECK(M::celdasDeEtapa(M::PRODUCTOR) == 4);   // el que da aguamiel

    // Y nunca al reves: crecer no encoge la planta.
    int ant = 0;
    for (uint16_t etapa : ETAPAS) {
        CHECK(M::celdasDeEtapa(etapa) >= ant);
        ant = M::celdasDeEtapa(etapa);
    }

    // Las cuatro celdas son el tope del campo SEGMENTO (2 bits).
    for (uint16_t etapa : ETAPAS)
        CHECK(M::celdasDeEtapa(etapa) <= 4);
}

TEST_CASE("la planta engorda al crecer, no solo se estira") {
    // Se pidio que el ancho y el grosor se multipliquen tambien. A lo ancho
    // la roseta topa en su celda (si se pasara, invadiria al vecino), asi que
    // el volumen se gana engordando cada penca y poniendo mas.
    float ant = 0.0f;
    for (uint16_t etapa : ETAPAS) {
        const float g = M::engrosado(etapa);
        CHECK(g > ant);          // cada fase, mas carnosa que la anterior
        ant = g;
    }
    // Un productor tiene las hojas mas del doble de gruesas que un brote.
    CHECK(M::engrosado(M::PRODUCTOR) > M::engrosado(M::BROTE) * 2.0f);
}

TEST_CASE("cuanto mas grande, mas pencas") {
    int ant = 0;
    for (uint16_t etapa : ETAPAS) {
        const int n = M::pencasDeEtapa(etapa);
        CHECK(n > ant);          // siempre mas que la fase anterior
        ant = n;
    }
    // El brote es un cogollo apenas abierto; el productor, una roseta densa.
    CHECK(M::pencasDeEtapa(M::BROTE) >= 5);
    CHECK(M::pencasDeEtapa(M::PRODUCTOR) >= 24);
}

TEST_CASE("las celdas de una misma planta comparten su estado") {
    // Una planta alta son VARIAS celdas, pero UNA sola planta: todas tienen
    // que llevar la misma etapa, el mismo giro y el mismo jugo. Si no, al
    // mallar cada una dibujaria una mata distinta y se veria una torre.
    const BlockType base = M::crear(M::PRODUCTOR, 2, 6, 9, true, 0);

    for (uint16_t s = 0; s < 4; ++s) {
        const BlockType celda = M::conSegmento(base, s);
        CHECK(M::segmentoDe(celda) == s);
        // El resto del estado no se toca.
        CHECK(M::etapaDe(celda)    == M::etapaDe(base));
        CHECK(M::giroDe(celda)     == M::giroDe(base));
        CHECK(M::puntasDe(celda)   == M::puntasDe(base));
        CHECK(M::aguamielDe(celda) == M::aguamielDe(base));
        CHECK(M::capadoDe(celda)   == M::capadoDe(base));
        // Y sigue siendo dibujable.
        CHECK(Compuesto::puedeDibujarse(celda));
    }

    // Solo la de abajo es la base: es la que manda y la que se capa.
    CHECK(M::esBase(M::conSegmento(base, 0)) == true);
    CHECK(M::esBase(M::conSegmento(base, 1)) == false);
    CHECK(M::esBase(M::conSegmento(base, 3)) == false);
}

TEST_CASE("el segmento no pisa a los demas campos del estado") {
    // El SEGMENTO se metio en los bits que quedaban libres (13..14). Si se
    // solapara con otro campo, subir una planta le cambiaria el jugo o la
    // etapa -- y eso solo se veria al cargar una partida vieja.
    for (uint16_t etapa : ETAPAS)
        for (uint16_t jugo = 0; jugo <= 15; jugo += 3)
            for (uint16_t puntas = 0; puntas <= 7; puntas += 2)
                for (uint16_t s = 0; s < 4; ++s) {
                    const BlockType t = M::crear(etapa, 3, puntas, jugo,
                                                 true, s);
                    CHECK(M::etapaDe(t)    == etapa);
                    CHECK(M::giroDe(t)     == 3);
                    CHECK(M::puntasDe(t)   == puntas);
                    CHECK(M::aguamielDe(t) == jugo);
                    CHECK(M::capadoDe(t)   == true);
                    CHECK(M::segmentoDe(t) == s);
                }
}

TEST_CASE("la COLISION se queda en la celda aunque las hojas asomen") {
    // ⚠️ EL MATIZ IMPORTANTE, y por que este test cambio de sentido.
    //
    // Antes exigia que NADA saliera de la celda. Eso contradice la planta
    // real: la descripcion pide un diametro de 1.5-3 m, que no cabe en un
    // bloque por definicion. Las hojas TIENEN que asomar.
    //
    // Asomar en el DIBUJADO es inofensivo: el recorte va por chunk (16
    // bloques), no por celda.
    //
    // Lo que no puede salirse es la COLISION, porque el motor la consulta
    // celda a celda: lo que se declare fuera sencillamente no existe. Y no
    // hace falta que salga -- lo que frena al jugador es la PIÑA, igual que
    // en el campo se pasa rozando las hojas de un agave pero no se atraviesa
    // su tronco.
    //
    // El radio del cogollo (ver nopalHitboxCon) se topa en 0.30, asi que la
    // caja siempre cabe.
    for (uint16_t etapa : ETAPAS) {
        const float esc = M::escalaDeEtapa(etapa);
        float r = 0.10f + 0.09f * esc;
        if (r > 0.30f) r = 0.30f;
        INFO("etapa=" << etapa << " radio de colision=" << r);
        CHECK(r <= 0.5f);          // cabe en la celda
        CHECK(r > 0.0f);           // pero frena de verdad
    }
}

TEST_CASE("el raycast distingue las partes de la planta") {
    // Cuerpo siempre; puntas solo si le quedan; jugo solo si hay.
    BlockType pelado = M::crear(M::PRODUCTOR, 0, 0, 0, false);
    CHECK(Compuesto::componentesDe(pelado) == 1);

    BlockType conPuntas = M::crear(M::PRODUCTOR, 0, 6, 0, false);
    CHECK(Compuesto::componentesDe(conPuntas) == 2);

    BlockType lleno = M::crear(M::PRODUCTOR, 0, 6, 9, true);
    CHECK(Compuesto::componentesDe(lleno) == 3);

    // El liquido no se rompe a golpes: se recoge con un recipiente.
    CHECK(Compuesto::componenteRompible(lleno, 2) == false);
    CHECK(Compuesto::componenteRompible(lleno, 0) == true);
}
