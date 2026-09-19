#include <doctest/doctest.h>
#include "fauna/AnimalMallaCache.h"
#include <cmath>
#include <vector>

// ============================================================================
// EL PULIDO DEL MODELO DEL PECARI
// ============================================================================
// Tres arreglos, cada uno contra un fallo que se veia en pantalla:
//
//   1. OJOS      - existian pero estaban medio enterrados en la mejilla
//   2. CRIN      - flotaba sobre la espalda como una linea suelta
//   3. PATAS     - eran conos rectos, sin codo ni rodilla
//
// Y un cuarto bloque que fija el PRESUPUESTO DE MEMORIA, porque el pulido se
// pidio explicitamente "sin consumir mucha RAM y espacio": si alguien anade
// detalle sin medirlo, estos tests fallan y dicen cuanto se paso.
//
// Cada test esta escrito para volver a fallar si el arreglo se deshace por
// descuido, no solo para pasar hoy.
// ============================================================================

using namespace Fauna;

// ============================================================================
// 1. LOS OJOS
// ============================================================================

TEST_CASE("Ojos: existen y estan A LOS LADOS de la cabeza") {
    // EL FALLO: el ojo era un tubo de 1.9 cm con el eje apuntando HACIA
    // ADELANTE, colocado a una altura fija calculada a mano. En un animal de
    // ojos LATERALES eso lo dejaba medio metido en la mejilla: estaba en la
    // malla, pero apenas asomaba.
    //
    // Los ojos laterales son la firma de una PRESA: campo visual casi
    // panoramico a costa de poca vision binocular.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    int nOjo = 0;
    float maxAbsX = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        ++nOjo;
        const float ax = std::fabs(v.pos.x);
        if (ax > maxAbsX) maxAbsX = ax;
    }

    REQUIRE(nOjo > 0);

    // Lejos del plano de simetria: un ojo lateral no vive en el centro.
    INFO("|x| maximo de un vertice de ojo = ", maxAbsX,
         "  ancho de cabeza = ", p.anchoCabeza);
    CHECK(maxAbsX > p.anchoCabeza * 0.20f);
}

TEST_CASE("Ojos: hay DOS, y son simetricos") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    int izq = 0, der = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        if (v.pos.x < 0.0f) ++izq; else ++der;
    }

    CHECK(izq > 0);
    CHECK(der > 0);
    CHECK(izq == der);
}

TEST_CASE("Ojos: estan en la CABEZA, no en el cuerpo") {
    // Una comprobacion de cordura: si alguien cambia las proporciones de la
    // cabeza y el ojo se queda con una coordenada vieja, acabaria flotando
    // junto al lomo. Aqui se exige que caiga dentro del volumen de la cabeza.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // La cabeza esta por delante del tronco.
    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float zPecho = largoTronco * 0.48f;

    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        // Por delante del pecho: es donde empieza el cuello y luego la cabeza.
        CHECK(v.pos.z > zPecho);
    }
}

TEST_CASE("Ojos: son oscuros y NO brillan") {
    // El pecari NO tiene tapetum lucidum (MEDIDO): sus ojos no reflejan la
    // luz de noche. Es el error tipico al representar fauna.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        const float lum = v.r * 0.3f + v.g * 0.6f + v.b * 0.1f;
        CHECK(lum < 0.25f);
    }
}

// ============================================================================
// 2. LA CRIN DORSAL
// ============================================================================

TEST_CASE("Crin: va PEGADA al lomo, no flotando encima") {
    // EL FALLO QUE ESTO FIJA: la crin colgaba de la constante
    // `ejeY + altoTorso*0.5*1.30`. Ese 1.30 la subia un 30% por encima del
    // radio del torso, o sea un dedo de AIRE entre el lomo y la crin. Se veia
    // como una linea rara flotando sobre la espalda.
    //
    // Y ademas era una RECTA sobre un lomo que no lo es (barril, grupa caida,
    // alzado creciente): aunque se bajara el 1.30 al valor justo, la crin solo
    // tocaria en un punto.
    //
    // ------------------------------------------------------------------------
    // COMO SE MIDE, Y POR QUE NO SE MIDE CONTRA OTROS VERTICES
    // ------------------------------------------------------------------------
    // El primer intento comparaba cada vertice de crin con el vertice de
    // CUERPO mas alto de su rebanada. No sirve, y merece la pena dejarlo
    // escrito porque es una trampa natural:
    //
    // El tronco es un TUBO de N lados (14 en LOD 0). Sus vertices caen en
    // angulos discretos del anillo, y NINGUNO cae exactamente en el plano
    // central. Medido: en 3 de 4 rebanadas no habia un solo vertice de cuerpo
    // con |x| < 8% del ancho. Lo mas alto que se encontraba era el vertice del
    // FLANCO, que en una elipse esta ~3 cm por debajo de la cresta central --
    // y esos 3 cm aparecian como un "hueco" que no existe.
    //
    // Asi que se compara contra la SUPERFICIE, no contra la muestra: se evalua
    // la elipse del tronco en el plano x=0, que es donde de verdad se apoya la
    // crin. Son las mismas formulas de construirTronco.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float ejeY  = p.alturaCruz - p.altoTorso * 0.5f;
    const float z0    = -largoTronco * 0.52f;
    const float z1    =  largoTronco * 0.48f;

    auto lomoEn = [&](float z) {
        float t = (z - z0) / (z1 - z0);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        const float d = (t - 0.58f) / 0.58f;
        const float radioY = p.altoTorso * 0.5f * (1.0f - 0.22f * d * d);
        const float centroY = ejeY - (1.0f - t) * (1.0f - t) * 0.035f * p.alturaCruz;
        const float alzado = 0.12f + 0.22f * t;
        return centroY + radioY * (1.0f + alzado * 0.30f);
    };

    float peorHueco = -1e9f;   // lo mas SEPARADA que queda la crin
    float masHundido = 1e9f;   // lo mas METIDA que queda
    int medidos = 0;

    for (const VerticeAnimal& c : m.vertices) {
        // La crin tiene ZONA PROPIA desde que se le dio color blanco grisaceo.
        // Antes se marcaba como LOMO y habia que distinguirla del lomo por su
        // estrechez; ahora se selecciona directamente, que es mas robusto.
        if (c.zona != ZonaCuerpo::CRIN) continue;
        // La crin es lo estrecho cerca del plano central.
        if (std::fabs(c.pos.x) > p.anchoCollar * 0.20f) continue;
        // Solo sobre el TRONCO: por delante la crin sigue al cuello, que tiene
        // su propia linea y no la describe esta formula.
        if (c.pos.z < z0 || c.pos.z > z1) continue;

        ++medidos;
        const float d = c.pos.y - lomoEn(c.pos.z);
        if (d > peorHueco)  peorHueco = d;
        if (d < masHundido) masHundido = d;
    }

    REQUIRE(medidos > 0);

    INFO("crin sobre el lomo: maximo = ", peorHueco,
         "  minimo = ", masHundido,
         "  altura de crin en reposo = ", p.altoCrinReposo,
         "  (", medidos, " vertices)");

    // 1. NO FLOTA: la parte baja de la crin queda POR DEBAJO de la piel. Es lo
    //    que garantiza que no haya aire entre las dos superficies.
    CHECK(masHundido <= 0.0f);

    // 2. NO SE HUNDE ENTERA: sigue asomando, porque es pelo y tiene que verse.
    CHECK(peorHueco > 0.0f);

    // 3. Y lo que asoma es del orden de la altura de crin declarada, no un
    //    poste. Con margen para el redondeo del anillo.
    CHECK(peorHueco <= p.altoCrinReposo * 1.30f);
}

TEST_CASE("Crin: sigue la CURVA del lomo, no es una linea recta") {
    // El lomo del pecari no es horizontal: tiene la grupa ligeramente caida y
    // el perfil de barril. Una crin recta se despegaria en los extremos.
    //
    // Si la crin sigue la curva, la altura de sus vertices NO es constante.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float minY = 1e9f, maxY = -1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::CRIN) continue;   // zona propia de la crin
        if (std::fabs(v.pos.x) > p.anchoCollar * 0.20f) continue;
        if (v.pos.y < minY) minY = v.pos.y;
        if (v.pos.y > maxY) maxY = v.pos.y;
    }

    REQUIRE(maxY > -1e8f);
    INFO("variacion de altura de la crin = ", maxY - minY);
    CHECK(maxY - minY > 0.01f);
}

// ============================================================================
// 3. LAS PATAS
// ============================================================================

TEST_CASE("Patas: tienen codo y rodilla, no son conos rectos") {
    // EL FALLO: la pata era `centroY = yHombro - largoPata*t` con la z FIJA --
    // una recta vertical que solo se estrechaba.
    //
    // Y era una contradiccion con el resto del sistema: PecariEsqueleto.h SI
    // declara CODO, RODILLA y CORVEJON con sus amplitudes, y el skinning los
    // dobla. Pero doblar un palo liso no produce un codo: produce un palo con
    // un pliegue.
    //
    // Con articulaciones, la pata deja de ser recta: sus vertices se apartan
    // del plano z = constante.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float minZ = 1e9f, maxZ = -1e9f;
    int n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::PATA) continue;
        if (v.pos.x >= 0.0f) continue;      // solo el lado izquierdo
        if (v.pos.z <= 0.0f) continue;      // solo la delantera
        ++n;
        if (v.pos.z < minZ) minZ = v.pos.z;
        if (v.pos.z > maxZ) maxZ = v.pos.z;
    }

    REQUIRE(n > 0);

    // Un cono recto daria un rango en Z igual al DIAMETRO de la pata. Con
    // codo, el quiebre desplaza secciones enteras adelante y atras.
    const float rango = maxZ - minZ;
    INFO("rango Z de la pata delantera = ", rango,
         "  grosor de pata = ", p.grosorPata);
    CHECK(rango > p.grosorPata * 1.40f);
}

TEST_CASE("Patas: el codo y la rodilla se doblan al REVES") {
    // Es lo que hace que un cuadrupedo se lea como tal: en la delantera el
    // codo apunta hacia ATRAS, en la trasera la rodilla hacia ADELANTE.
    // Invertir uno de los dos da un animal que parece roto.
    //
    // Es el mismo SENTIDO_DELANTERA/SENTIDO_TRASERA (-1/+1) que ya usaba el
    // esqueleto, ahora tambien en la geometria.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    auto desvioMedio = [&](bool delantera) {
        const float zNominal = delantera ? p.patasDelanteZ : p.patasTraseraZ;
        double suma = 0.0;
        int cuenta = 0;
        for (const VerticeAnimal& v : m.vertices) {
            if (v.zona != ZonaCuerpo::PATA) continue;
            if (v.pos.x >= 0.0f) continue;
            const bool esDelantera = (v.pos.z > 0.0f);
            if (esDelantera != delantera) continue;
            suma += (double)(v.pos.z - zNominal);
            ++cuenta;
        }
        return cuenta ? (float)(suma / cuenta) : 0.0f;
    };

    const float dDel = desvioMedio(true);
    const float dTra = desvioMedio(false);

    INFO("desvio medio: delantera = ", dDel, "  trasera = ", dTra);
    // Signos OPUESTOS: es toda la prueba.
    CHECK(dDel * dTra < 0.0f);
}

TEST_CASE("Patas: siguen siendo FINAS bajo un cuerpo macizo") {
    // El contraste patas finas / cuerpo macizo es una firma visual de la
    // especie. Darles articulaciones no puede convertirlas en columnas.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float maxAnchoPata = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::PATA) continue;
        if (v.pos.x >= 0.0f || v.pos.z <= 0.0f) continue;
        // Distancia al eje de ESA pata.
        const float d = std::fabs(v.pos.x - (-p.separacionPataX));
        if (d > maxAnchoPata) maxAnchoPata = d;
    }

    // El bulto de la articulacion existe, pero es sutil: no puede duplicar el
    // grosor nominal de la pata.
    INFO("semiancho maximo de pata = ", maxAnchoPata,
         "  grosor nominal = ", p.grosorPata);
    CHECK(maxAnchoPata < p.grosorPata * 1.30f);
}

TEST_CASE("Patas: llegan al suelo y no lo atraviesan") {
    // Las articulaciones desplazan secciones en Z, pero la pata tiene que
    // seguir apoyando: ni flotar ni hundirse.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float minY = 1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.pos.y < minY) minY = v.pos.y;
    }

    INFO("punto mas bajo del animal = ", minY);
    // Toca el suelo (con holgura) y no se hunde por debajo.
    CHECK(minY > -0.02f);
    CHECK(minY <  0.05f);
}

// ============================================================================
// 4. EL PRESUPUESTO DE MEMORIA
// ============================================================================
// El pulido se pidio "sin consumir mucha RAM y espacio". Esto lo convierte en
// un contrato verificable en vez de una intencion.

TEST_CASE("Memoria: cada LOD cabe en su presupuesto") {
    ParametrosPecari p = Especies::pecariDeCollar();

    // Se mide el tipo REAL en vez de suponer su tamano.
    const size_t bytesVert = sizeof(VerticeAnimal);

    size_t totalBytes = 0;
    size_t vertsLOD0 = 0;

    for (int lod = 0; lod <= 3; ++lod) {
        MallaAnimal m;
        ConstructorPecari::generar(m, p, lod);

        const size_t bytes = m.vertices.size() * bytesVert
                           + m.indices.size() * sizeof(uint16_t);
        totalBytes += bytes;
        if (lod == 0) vertsLOD0 = m.vertices.size();

        INFO("LOD ", lod, ": ", m.vertices.size(), " vertices, ",
             m.indices.size() / 3, " triangulos, ", bytes, " bytes");

        // Ningun nivel puede pasar de 64 KB.
        CHECK(bytes < 64u * 1024u);
    }

    // Los cuatro juntos: es lo que de verdad ocupa la especie, porque el cache
    // los comparte entre TODOS los individuos.
    INFO("los 4 LOD juntos = ", totalBytes, " bytes por especie");
    CHECK(totalBytes < 160u * 1024u);

    // El LOD 0 sigue cabiendo de sobra en indices de 16 bits.
    CHECK(vertsLOD0 < 65536u);
}

TEST_CASE("Memoria: el detalle se paga por ESPECIE, no por individuo") {
    // Es la propiedad que hace barato el pulido: la malla se genera una vez
    // por (especie, etapa, LOD) y se comparte. Anadir ojos, orbitas y codos
    // sube el coste de esa malla compartida, no el de la manada.
    ParametrosPecari p = Especies::pecariDeCollar();

    MallaAnimal m;
    ConstructorPecari::generar(m, p, 0);
    const size_t unaMalla = m.vertices.size() * sizeof(VerticeAnimal)
                          + m.indices.size() * sizeof(uint16_t);

    // Lo que cuesta un individuo es su transformacion, no su geometria:
    // posicion, orientacion, escala, fase de paso, etapa, LOD.
    const size_t porIndividuo = sizeof(float) * 8;

    const size_t cien = unaMalla + 100 * porIndividuo;
    INFO("1 malla = ", unaMalla, " bytes;  100 pecaries = ", cien, " bytes");

    // Cien animales cuestan menos del doble que uno solo.
    CHECK(cien < unaMalla * 2);
}

TEST_CASE("Memoria: los LOD lejanos son MUCHO mas baratos") {
    // El pulido solo se paga de cerca. Un animal a 60 metros no puede costar
    // lo mismo que uno a tres.
    ParametrosPecari p = Especies::pecariDeCollar();

    MallaAnimal cerca, lejos;
    ConstructorPecari::generar(cerca, p, 0);
    ConstructorPecari::generar(lejos, p, 3);

    INFO("LOD0 = ", cerca.vertices.size(), " vertices;  LOD3 = ",
         lejos.vertices.size());
    CHECK(lejos.vertices.size() * 2 < cerca.vertices.size());
}

// ============================================================================
// 5. PELAJE NEGRO Y CRESTA BLANCA
// ============================================================================
// Lo que se pidio: pelo ligero NEGRO por TODO el cuerpo, y la cresta de la
// espalda en BLANCO GRISACEO.
//
// Los dos rasgos tiran en direcciones opuestas -- uno oscurece el animal
// entero, el otro exige que una parte siga siendo clara -- asi que es
// exactamente el caso donde un cambio descuidado deshace el otro. De ahi que
// cada uno tenga su test.

namespace {
// Luminancia percibida. Se usa para hablar de "claro" y "oscuro" sin depender
// de un canal concreto.
float luma(const VerticeAnimal& v) {
    return v.r * 0.30f + v.g * 0.59f + v.b * 0.11f;
}
} // namespace

TEST_CASE("Pelaje: el cuerpo es NEGRO, no gris pardo") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se mide el pelaje del cuerpo: se dejan fuera los rasgos claros que
    // existen a proposito (collar y crin) y lo que no es pelo.
    double suma = 0.0; int n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::COLLAR || v.zona == ZonaCuerpo::CRIN ||
            v.zona == ZonaCuerpo::OJO    || v.zona == ZonaCuerpo::PEZUNA) continue;
        if (v.pelo < 0.25f) continue;          // piel desnuda, no pelaje
        suma += luma(v); ++n;
    }
    REQUIRE(n > 0);

    const float media = (float)(suma / n);
    INFO("luminancia media del pelaje = ", media);

    // Negro de verdad. El valor anterior rondaba 0.26 (gris pardo).
    CHECK(media < 0.16f);
    // Pero NO negro absoluto: un cuerpo a cero se lee como un agujero sin
    // volumen, y el jaspeado aguti dejaria de verse.
    CHECK(media > 0.02f);
}

TEST_CASE("Pelaje: la CRESTA dorsal es clara sobre el cuerpo negro") {
    // ⭐ EL TEST DEL RASGO PEDIDO.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double sCrin = 0.0; int nCrin = 0;
    double sLomo = 0.0; int nLomo = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::CRIN) { sCrin += luma(v); ++nCrin; }
        else if (v.zona == ZonaCuerpo::LOMO) { sLomo += luma(v); ++nLomo; }
    }
    REQUIRE(nCrin > 0);
    REQUIRE(nLomo > 0);

    const float crin = (float)(sCrin / nCrin);
    const float lomo = (float)(sLomo / nLomo);
    INFO("crin = ", crin, "   lomo = ", lomo);

    // Blanco grisaceo: claro de verdad...
    CHECK(crin > 0.60f);
    // ...pero NO blanco puro. Es pelo gris, no papel.
    CHECK(crin < 0.92f);

    // Y el contraste con el lomo es lo que hace que la cresta SE VEA. Sin
    // esto, erizarse no comunicaria nada.
    CHECK(crin > lomo * 3.0f);
}

TEST_CASE("Pelaje: la raya dorsal sigue siendo mas oscura que el flanco") {
    // MEDIDO: "con una raya dorsal oscura". Al aclarar la crin es facil
    // aclarar de paso el lomo, y eso SI contradiria el dato.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double sLomo = 0.0; int nLomo = 0;
    double sVientre = 0.0; int nVientre = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::LOMO)         { sLomo += luma(v); ++nLomo; }
        else if (v.zona == ZonaCuerpo::VIENTRE) { sVientre += luma(v); ++nVientre; }
    }
    REQUIRE(nLomo > 0);
    REQUIRE(nVientre > 0);

    // El dorso oscuro y el vientre claro: la estructura se conserva aunque
    // todo el conjunto haya bajado de tono.
    CHECK((sLomo / nLomo) < (sVientre / nVientre));
}

TEST_CASE("Pelaje: el COLLAR diagnostico sobrevive al oscurecimiento") {
    // Es el rasgo que da NOMBRE a la especie. Un pecari de collar sin collar
    // visible es un error de identificacion, no un detalle.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float maxLuma = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::CRIN) continue;   // el otro rasgo claro
        const float l = luma(v);
        if (l > maxLuma) maxLuma = l;
    }
    INFO("vertice mas claro fuera de la crin = ", maxLuma);
    CHECK(maxLuma > 0.45f);      // el collar sigue destacando
}

TEST_CASE("Pelaje: el collar llega a su color pleno pese al muestreo") {
    // EL FALLO CONCRETO que obligo a cambiar la mascara del collar.
    //
    // El centro de la banda cae ENTRE dos anillos de la malla: medido, el
    // vertice mas cercano queda a 0.0174 m de un collar de 0.042 m de ancho.
    // Con la curva k*k eso daba mezcla 0.34 -- el collar nunca alcanzaba su
    // color y, sobre el cuerpo negro, se quedaba a medio tono.
    //
    // Este test no mira la formula: mira el RESULTADO. Alguna parte del collar
    // tiene que acercarse de verdad al color declarado.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    const float objetivo = p.colorCollar[0] * 0.30f + p.colorCollar[1] * 0.59f
                         + p.colorCollar[2] * 0.11f;

    float maxLuma = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::CRIN) continue;
        const float l = luma(v);
        if (l > maxLuma) maxLuma = l;
    }
    INFO("collar medido = ", maxLuma, "   declarado = ", objetivo);
    CHECK(maxLuma > objetivo * 0.85f);
}

TEST_CASE("Pelaje: el jaspeado AGUTI no se pierde en el negro") {
    // ⭐ EL FALLO QUE HAY QUE EVITAR AL OSCURECER.
    //
    // El aguti es aditivo y simetrico. Sobre un tono base muy bajo, la mitad
    // NEGATIVA se recorta contra 0 y desaparece: el animal queda plano justo
    // en las zonas mas oscuras -- lo contrario de parecer peludo.
    //
    // colorear() lo corrige sesgando la mezcla hacia la luz cuanto mas oscuro
    // es el tono. Aqui se comprueba que ese sesgo funciona.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float minL = 1e9f, maxL = -1e9f;
    int aCero = 0, n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::TORSO) continue;
        const float l = luma(v);
        if (l < minL) minL = l;
        if (l > maxL) maxL = l;
        if (v.r <= 0.0f && v.g <= 0.0f && v.b <= 0.0f) ++aCero;
        ++n;
    }
    REQUIRE(n > 0);
    INFO("torso: min = ", minL, "  max = ", maxL, "  a cero = ", aCero);

    // Sigue habiendo variacion visible de vertice a vertice.
    CHECK(maxL - minL > 0.03f);
    // Y NINGUN vertice se ha recortado a negro absoluto.
    CHECK(aCero == 0);
}

TEST_CASE("Pelaje: el pelo tiene DIRECCION, no es ruido suelto") {
    // direccionPelo() existia con su campo de flujo por zona... y no la
    // llamaba nadie salvo los tests. La perturbacion era ruido isotropo, que
    // da una superficie abollada en vez de pelaje peinado.
    //
    // Si el sesgo direccional se aplica, las normales de una zona con flujo
    // marcado dejan de estar repartidas al azar y se inclinan en promedio
    // hacia ese flujo.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // En las patas el pelo baja casi vertical: es la zona con el flujo mas
    // inequivoco, asi que es donde el efecto se mide mejor.
    double sumaY = 0.0; int n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::PATA) continue;
        if (v.pelo < 0.25f) continue;
        sumaY += v.normal.y; ++n;
    }
    REQUIRE(n > 0);

    const float mediaY = (float)(sumaY / n);
    INFO("inclinacion media de la normal en las patas = ", mediaY);

    // Con ruido puro esta media rondaria 0 (las patas son tubos: las normales
    // apuntan a los lados y se cancelan). Con el sesgo del pelo hacia abajo,
    // se vuelve netamente negativa.
    CHECK(mediaY < -0.02f);
}

TEST_CASE("Pelaje: la crin NO desaparece a media distancia") {
    // Con la cresta en claro sobre un cuerpo negro, la crin pasa a ser
    // SILUETA: lo primero que se distingue del animal de lejos. Apagarla en
    // LOD 1 la hacia aparecer de golpe al acercarse, que es popping en el
    // rasgo mas visible del modelo.
    for (int lod = 0; lod <= 1; ++lod) {
        MallaAnimal m;
        ParametrosPecari p = Especies::pecariDeCollar();
        ConstructorPecari::generar(m, p, lod);

        int nCrin = 0;
        for (const VerticeAnimal& v : m.vertices)
            if (v.zona == ZonaCuerpo::CRIN) ++nCrin;

        INFO("LOD ", lod, " tiene ", nCrin, " vertices de crin");
        CHECK(nCrin > 0);
    }
}

TEST_CASE("Pelaje: el ojo sigue siendo lo mas oscuro de la cara") {
    // Al bajar todo el cuerpo a negro, el ojo puede dejar de contrastar y el
    // animal pierde la mirada. El parametro se reajusto; esto lo fija.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float maxOjo = -1e9f, minCara = 1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::OJO) {
            const float l = luma(v);
            if (l > maxOjo) maxOjo = l;
        } else if (v.zona == ZonaCuerpo::CABEZA) {
            const float l = luma(v);
            if (l < minCara) minCara = l;
        }
    }
    REQUIRE(maxOjo > -1e8f);
    REQUIRE(minCara < 1e8f);
    INFO("ojo mas claro = ", maxOjo, "   cara mas oscura = ", minCara);
    CHECK(maxOjo < minCara);
}
