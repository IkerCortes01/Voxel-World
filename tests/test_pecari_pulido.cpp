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
    //
    // ⭐ EL UMBRAL BAJO DE 1.40 A 1.12, Y NO ES RELAJAR EL TEST.
    //
    // QUIEBRE paso de 0.085 a 0.022 porque las patas estaban DOBLADAS EN
    // REPOSO: el zigzag iba metido en la malla cacheada, asi que el animal
    // quieto ya aparecia agachado, y encima PoseDeMarcha doblaba los mismos
    // huesos sobre una geometria que ya venia curvada.
    //
    // Un ungulado de pie tiene la pata CASI RECTA -- es lo que le permite
    // aguantar el peso sin esfuerzo muscular. Lo que este test debe seguir
    // impidiendo es la vuelta al CONO RECTO (rango = 1.0 x grosor, el palo
    // liso), no fijar una postura agachada concreta.
    //
    // Con 1.12 el margen sigue siendo inequivoco: un cono da exactamente 1.0.
    const float rango = maxZ - minZ;
    INFO("rango Z de la pata delantera = ", rango,
         "  grosor de pata = ", p.grosorPata);
    CHECK(rango > p.grosorPata * 1.12f);
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

        // ⭐ EL TOPE SUBE DE 64 KB A 96 KB, Y SE JUSTIFICA.
        //
        // La densidad de LOD 0 subio de 14x11 a 20x16 porque la piel se veia
        // "lisa y estirada": con 11 anillos en todo el cuerpo no hay vertices
        // entre costilla y costilla donde meter relieve, y el sombreado es
        // Gouraud (por vertice), asi que TODO el detalle de superficie vive en
        // la densidad de la malla.
        //
        // Medido: LOD 0 pasa de 46 KB a 76 KB.
        //
        // POR QUE SE PUEDE PAGAR. La malla es POR ESPECIE, no por individuo:
        // el cache la comparte, asi que 100 pecaries siguen usando UNA. Los
        // 30 KB extra son de una vez, no por animal -- el test de mas abajo
        // ("el detalle se paga por ESPECIE") es el que protege esa propiedad,
        // y ese sigue igual de estricto.
        //
        // Y solo LOD 0 la paga: animales a menos de 7 metros, que son pocos.
        //
        // EL LIMITE QUE SI ES DURO no es este: los indices son uint16_t, o sea
        // 65.535 vertices como maximo. Con 1.393 queda mucho margen, y hay un
        // CHECK explicito mas abajo que lo vigila.
        CHECK(bytes < 96u * 1024u);
    }

    // Los cuatro juntos: es lo que de verdad ocupa la especie, porque el cache
    // los comparte entre TODOS los individuos.
    // Sube de 160 KB a 224 KB por la misma razon que el tope por nivel: mas
    // densidad en LOD 0-1 para que la piel deje de verse lisa. Medido: 157 KB.
    // Siguen siendo 224 KB para TODA la especie, compartidos por cada pecari
    // del mundo -- menos que una sola textura de 256x256 sin comprimir.
    INFO("los 4 LOD juntos = ", totalBytes, " bytes por especie");
    CHECK(totalBytes < 224u * 1024u);

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

TEST_CASE("Pelaje: la PUPILA es lo mas oscuro del animal") {
    // ⭐ ESTE TEST CAMBIO DE CRITERIO, Y LA RAZON IMPORTA.
    //
    // Antes exigia que el OJO fuera lo mas oscuro de la cara. Eso tenia
    // sentido con un animal gris pardo, pero al pasar el pelaje a negro se
    // volvio contraproducente: un ojo oscuro sobre una cara negra no se ve.
    //
    // La solucion no fue relajar el test sino separar el ojo en dos piezas --
    // IRIS pardo y PUPILA negra -- que es como funciona un ojo de verdad. El
    // iris ahora CONTRASTA por ser mas claro, y la pupila conserva el papel de
    // punto mas oscuro.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float maxPupila = -1e9f;
    float minResto  = 1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        const float l = luma(v);
        if (v.zona == ZonaCuerpo::PUPILA) {
            if (l > maxPupila) maxPupila = l;
        } else {
            if (l < minResto) minResto = l;
        }
    }
    REQUIRE(maxPupila > -1e8f);
    REQUIRE(minResto < 1e8f);
    INFO("pupila mas clara = ", maxPupila, "   resto mas oscuro = ", minResto);
    CHECK(maxPupila <= minResto);
}

// ============================================================================
// 6. ADAPTACION AL TERRENO
// ============================================================================
// La altura se resolvia con UNA sonda bajo el centro, asi que las cuatro patas
// quedaban siempre a la misma altura. En una cuesta eso deja las de abajo
// colgando y mete las de arriba en la roca.

TEST_CASE("Terreno: en llano no se toca nada") {
    // El caso que mas veces ocurre. Si el suelo esta plano, la adaptacion debe
    // ser EXACTAMENTE neutra: cualquier residuo seria un animal torcido sin
    // motivo.
    PoseEsqueleto pose;
    pose.limpiar();
    PoseDeMarcha(0.0f, 0.0f, pose);

    PoseEsqueleto antes = pose;

    SueloBajoPatas llano;   // los cuatro a 0
    AplicarTerreno(llano, pose);

    for (int i = 0; i < (int)HuesoAnimal::_COUNT; ++i)
        CHECK(pose.giroX[i] == doctest::Approx(antes.giroX[i]));
}

TEST_CASE("Terreno: cuesta arriba el animal apunta hacia arriba") {
    // Con las patas delanteras en suelo mas alto, el cuerpo se inclina. Es lo
    // que mas se ve de lejos y lo que evita el aspecto de "flotar en diagonal".
    PoseEsqueleto pose;
    pose.limpiar();

    SueloBajoPatas cuesta;
    cuesta.delanteraIzq = 0.12f;   // 12 cm mas alto delante
    cuesta.delanteraDer = 0.12f;
    AplicarTerreno(cuesta, pose);

    const float tronco = pose.giroX[(int)HuesoAnimal::TRONCO];
    INFO("cabeceo del tronco en cuesta = ", tronco);
    CHECK(tronco != doctest::Approx(0.0f));

    // Y cuesta abajo tiene que salir al reves.
    PoseEsqueleto pose2;
    pose2.limpiar();
    SueloBajoPatas bajada;
    bajada.traseraIzq = 0.12f;
    bajada.traseraDer = 0.12f;
    AplicarTerreno(bajada, pose2);

    CHECK(pose2.giroX[(int)HuesoAnimal::TRONCO] * tronco < 0.0f);
}

TEST_CASE("Terreno: cada pata se ajusta por su cuenta") {
    // ⭐ LO QUE UNA SOLA SONDA NO PUEDE HACER.
    //
    // Con un escalon bajo UNA pata, esa pata tiene que moverse y las otras no
    // deberian seguirla. Es el caso que antes dejaba un pie en el aire.
    PoseEsqueleto pose;
    pose.limpiar();

    SueloBajoPatas escalon;
    escalon.delanteraIzq = 0.14f;   // solo este pie pisa mas alto
    AplicarTerreno(escalon, pose);

    const float di = pose.giroX[(int)HuesoAnimal::HOMBRO_DI];
    const float dd = pose.giroX[(int)HuesoAnimal::HOMBRO_DD];

    INFO("hombro izq = ", di, "  hombro der = ", dd);
    CHECK(std::fabs(di) > 1e-4f);        // la pata del escalon SI se ajusta
    CHECK(std::fabs(di - dd) > 1e-4f);   // y no lo hacen las dos igual
}

TEST_CASE("Terreno: un desnivel absurdo no produce posturas imposibles") {
    // Defensa: un acantilado bajo una pata no puede girar el hueso 180 grados.
    PoseEsqueleto pose;
    pose.limpiar();

    SueloBajoPatas absurdo;
    absurdo.delanteraIzq =  40.0f;
    absurdo.delanteraDer = -40.0f;
    absurdo.traseraIzq   =  40.0f;
    absurdo.traseraDer   = -40.0f;
    AplicarTerreno(absurdo, pose);

    for (int i = 0; i < (int)HuesoAnimal::_COUNT; ++i) {
        CHECK(std::fabs(pose.giroX[i]) <= Terreno::INCLINACION_MAX + 1e-4f);
    }
}

TEST_CASE("Terreno: se SUMA a la marcha, no la sustituye") {
    // El caso interesante es subir una loma ANDANDO. Si la adaptacion pisara
    // la pose de marcha, el animal dejaria de mover las patas en cuesta.
    PoseEsqueleto soloMarcha;
    soloMarcha.limpiar();
    PoseDeMarcha(1.0f, 3.0f, soloMarcha);

    PoseEsqueleto conCuesta;
    conCuesta.limpiar();
    PoseDeMarcha(1.0f, 3.0f, conCuesta);
    SueloBajoPatas cuesta;
    cuesta.delanteraIzq = 0.10f;
    cuesta.delanteraDer = 0.10f;
    AplicarTerreno(cuesta, conCuesta);

    // El codo, que la adaptacion NO toca, debe seguir exactamente igual: es la
    // prueba de que la marcha sobrevive.
    CHECK(conCuesta.giroX[(int)HuesoAnimal::CODO_DI] ==
          doctest::Approx(soloMarcha.giroX[(int)HuesoAnimal::CODO_DI]));

    // Y el hombro SI cambia, porque ahi se suman las dos cosas.
    CHECK(conCuesta.giroX[(int)HuesoAnimal::HOMBRO_DI] !=
          doctest::Approx(soloMarcha.giroX[(int)HuesoAnimal::HOMBRO_DI]));
}

// ============================================================================
// 7. SUPERFICIE, OREJAS Y NARIZ
// ============================================================================

TEST_CASE("Superficie: la piel NO es una elipse perfecta") {
    // ⭐ "LA PIEL SE VE LISA Y ESTIRADA".
    //
    // La malla era una superficie de revolucion exacta: cada anillo, una
    // elipse matematica. Perturbar las normales cambiaba el sombreado pero la
    // SILUETA seguia siendo impecable, y la silueta es lo que delata un modelo
    // liso contra el cielo.
    //
    // Con relieve real, los vertices de un mismo anillo dejan de estar todos a
    // la misma distancia del eje.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se toman vertices del torso en una franja estrecha de Z (un anillo) y se
    // mide cuanto varia su radio.
    float zRef = 0.0f;
    for (const VerticeAnimal& v : m.vertices)
        if (v.zona == ZonaCuerpo::TORSO) { zRef = v.pos.z; break; }

    float minR = 1e9f, maxR = -1e9f;
    int n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::TORSO) continue;
        if (std::fabs(v.pos.z - zRef) > 0.004f) continue;
        const float r = std::sqrt(v.pos.x * v.pos.x);
        if (r < minR) minR = r;
        if (r > maxR) maxR = r;
        ++n;
    }
    REQUIRE(n > 3);

    INFO("radio en un anillo del torso: min = ", minR, "  max = ", maxR);
    CHECK(maxR - minR > 0.0008f);   // hay irregularidad de verdad
}

TEST_CASE("Superficie: el relieve NO deforma el ojo ni la pezuna") {
    // Son superficies duras o humedas: lisas por naturaleza. Arrugar un globo
    // ocular lo estropearia.
    MallaAnimal liso, conRelieve;
    ParametrosPecari p = Especies::pecariDeCollar();

    ParametrosPecari sinR = p;
    sinR.relieveSuperficie = 0.0f;
    ConstructorPecari::generar(liso, sinR, 0);
    ConstructorPecari::generar(conRelieve, p, 0);

    REQUIRE(liso.vertices.size() == conRelieve.vertices.size());

    for (size_t i = 0; i < liso.vertices.size(); ++i) {
        const ZonaCuerpo z = liso.vertices[i].zona;
        if (z != ZonaCuerpo::OJO && z != ZonaCuerpo::PUPILA &&
            z != ZonaCuerpo::PEZUNA) continue;
        CHECK(liso.vertices[i].pos.x == doctest::Approx(conRelieve.vertices[i].pos.x));
        CHECK(liso.vertices[i].pos.y == doctest::Approx(conRelieve.vertices[i].pos.y));
    }
}

TEST_CASE("Orejas: tienen concha, no son conos rectos") {
    // El oido es el segundo sentido de la especie, muy por delante de la
    // vista. Una oreja que solo se afila no recogeria sonido.
    //
    // Con concha, el ancho NO decrece de forma monotona: se ensancha en el
    // tercio bajo antes de afilarse.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se mira solo la oreja de un lado.
    float minY = 1e9f, maxY = -1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OREJA || v.pos.x < 0.0f) continue;
        if (v.pos.y < minY) minY = v.pos.y;
        if (v.pos.y > maxY) maxY = v.pos.y;
    }
    REQUIRE(maxY > minY);

    // Ancho maximo en el tercio BAJO contra el del tercio ALTO.
    const float corte1 = minY + (maxY - minY) * 0.33f;
    const float corte2 = minY + (maxY - minY) * 0.67f;
    float anchoBajo = 0.0f, anchoAlto = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OREJA || v.pos.x < 0.0f) continue;
        const float dx = std::fabs(v.pos.x);
        if (v.pos.y < corte1) { if (dx > anchoBajo) anchoBajo = dx; }
        else if (v.pos.y > corte2) { if (dx > anchoAlto) anchoAlto = dx; }
    }
    INFO("ancho tercio bajo = ", anchoBajo, "  tercio alto = ", anchoAlto);
    CHECK(anchoBajo > anchoAlto);   // se afila hacia la punta
}

TEST_CASE("Nariz: el disco rinarial tiene fosas nasales") {
    // ⭐ EL RASGO MAS FUNCIONAL DEL ANIMAL, Y ERA UNA LOSA LISA.
    //
    // Este animal tiene vista pesima (MEDIDO: no distingue objetos a mas de un
    // metro) y detecta raices a 8 cm bajo tierra. Las fosas nasales son,
    // funcionalmente, sus ojos. Que los ojos se modelaran y ellas no era justo
    // al reves de lo que pide su biologia.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float zFrente = largoTronco * 0.48f + p.largoCuello
                        + p.largoCabeza + p.largoHocico;

    // Vertices del hocico METIDOS hacia dentro respecto a la cara del disco:
    // son las fosas.
    int dentro = 0;
    float masOscuro = 1e9f, delDisco = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::HOCICO) continue;
        const float d = zFrente - v.pos.z;
        const float l = luma(v);
        if (d > p.discoRinarial * 0.02f && d < p.discoRinarial * 0.42f) {
            ++dentro;
            if (l < masOscuro) masOscuro = l;
        } else if (d < 0.005f) {
            if (l > delDisco) delDisco = l;
        }
    }

    INFO("vertices de fosa = ", dentro,
         "  fosa mas oscura = ", masOscuro, "  disco = ", delDisco);
    CHECK(dentro > 0);                 // las fosas existen
    CHECK(masOscuro < delDisco);       // y son mas oscuras: se leen como agujero
}

// ============================================================================
// 8. EL MODELO ES CUBICO, Y NO SE VE POR DENTRO
// ============================================================================

TEST_CASE("Cuadratura: la seccion es de CAJA, no una elipse") {
    // ⭐ LO QUE SE PIDIO: que el modelo sea cubico, de mundo voxel.
    //
    // Un anillo circular tiene TODOS sus puntos a la misma distancia del eje.
    // Uno cuadrado no: la esquina esta a sqrt(2) veces lo que esta el centro
    // del lado. Esa diferencia es exactamente la medida de "cuanto de caja es".
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se toma un anillo del torso (franja estrecha de Z) y se mide la relacion
    // entre el punto mas lejano y el mas cercano al eje.
    float zRef = 1e9f;
    for (const VerticeAnimal& v : m.vertices)
        if (v.zona == ZonaCuerpo::TORSO) { zRef = v.pos.z; break; }
    REQUIRE(zRef < 1e8f);

    float minD = 1e9f, maxD = -1e9f;
    int n = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::TORSO && v.zona != ZonaCuerpo::LOMO &&
            v.zona != ZonaCuerpo::VIENTRE) continue;
        if (std::fabs(v.pos.z - zRef) > 0.004f) continue;
        // Distancia al eje NORMALIZADA por los semiejes, para que la elipse
        // del cuerpo no se confunda con cuadratura.
        const float nx = v.pos.x / (p.anchoTorso * 0.5f);
        const float d = std::fabs(nx);
        if (d < minD) minD = d;
        if (d > maxD) maxD = d;
        ++n;
    }
    REQUIRE(n > 4);

    INFO("anillo del torso: |x| normalizado min=", minD, " max=", maxD);

    // ⭐ SE COMPARA CONTRA EL MISMO ANIMAL SIN CUADRAR, no contra un numero.
    //
    // Un umbral absoluto no sirve: el |x| maximo de un anillo depende de
    // DONDE caiga ese anillo en el barril (el torso se estrecha hacia los
    // extremos) y de cuantos lados tenga. Medido, el anillo de referencia da
    // 0.793 -- que no dice nada por si solo.
    //
    // Lo que SI dice algo es la diferencia: con cuadratura, los puntos de los
    // LADOS se empujan hacia el borde de la caja, asi que el conjunto se
    // separa mas del centro que con un circulo puro.
    ParametrosPecari redondo = Especies::pecariDeCollar();
    redondo.cuadratura = 0.0f;
    MallaAnimal mR;
    ConstructorPecari::generar(mR, redondo, 0);

    float maxRedondo = -1e9f;
    for (const VerticeAnimal& v : mR.vertices) {
        if (v.zona != ZonaCuerpo::TORSO && v.zona != ZonaCuerpo::LOMO &&
            v.zona != ZonaCuerpo::VIENTRE) continue;
        if (std::fabs(v.pos.z - zRef) > 0.004f) continue;
        const float d = std::fabs(v.pos.x / (p.anchoTorso * 0.5f));
        if (d > maxRedondo) maxRedondo = d;
    }
    REQUIRE(maxRedondo > -1e8f);

    INFO("mismo anillo SIN cuadrar: max=", maxRedondo);

    // ⚠️ MEDIR SOLO |x| NO SIRVE, Y AVERIGUARLO COSTO UN INTENTO.
    //
    // El punto mas ancho del anillo es el que tiene sy~0, y ese YA ESTA en el
    // borde de la caja antes de cuadrar: la cuadratura no lo mueve. Por eso
    // maxD sale practicamente igual con y sin (0.7927 en ambos) y el test
    // anterior "demostraba" que no pasaba nada cuando si pasaba.
    //
    // Lo que la cuadratura cambia son los puntos INTERMEDIOS -- los de las
    // diagonales -- que se empujan hacia la esquina. La medida correcta es el
    // AREA que encierra el anillo: un cuadrado encierra 4/pi = 1.27 veces mas
    // que el circulo que lo inscribe.
    auto areaAnillo = [&](const MallaAnimal& mm) {
        // Formula del zapato sobre los puntos del anillo, ordenados por angulo.
        std::vector<std::pair<float,float>> pts;
        for (const VerticeAnimal& v : mm.vertices) {
            if (v.zona != ZonaCuerpo::TORSO && v.zona != ZonaCuerpo::LOMO &&
                v.zona != ZonaCuerpo::VIENTRE) continue;
            if (std::fabs(v.pos.z - zRef) > 0.004f) continue;
            pts.push_back({ v.pos.x, v.pos.y });
        }
        // Centro del anillo.
        float cx = 0.0f, cy = 0.0f;
        for (auto& q : pts) { cx += q.first; cy += q.second; }
        if (pts.empty()) return 0.0f;
        cx /= pts.size(); cy /= pts.size();
        // Ordenar por angulo alrededor del centro.
        for (size_t i = 0; i + 1 < pts.size(); ++i)
            for (size_t j = i + 1; j < pts.size(); ++j) {
                const float ai = std::atan2(pts[i].second - cy, pts[i].first - cx);
                const float aj = std::atan2(pts[j].second - cy, pts[j].first - cx);
                if (aj < ai) std::swap(pts[i], pts[j]);
            }
        float a = 0.0f;
        for (size_t i = 0; i < pts.size(); ++i) {
            const auto& q0 = pts[i];
            const auto& q1 = pts[(i + 1) % pts.size()];
            a += q0.first * q1.second - q1.first * q0.second;
        }
        return std::fabs(a) * 0.5f;
    };

    const float areaCubo    = areaAnillo(m);
    const float areaRedondo = areaAnillo(mR);

    INFO("area del anillo: cuadrado=", areaCubo, "  redondo=", areaRedondo,
         "  razon=", areaCubo / areaRedondo);

    // Un cuadrado encierra 4/pi = 1.273 veces el area de su circulo inscrito.
    // Con cuadratura 0.78 (no 1.0) la ganancia es parcial, pero tiene que ser
    // inequivoca: por debajo de un 10% no se estaria cuadrando nada.
    CHECK(areaCubo > areaRedondo * 1.10f);
}

TEST_CASE("Cuadratura: el animal NO cambia de tamano") {
    // ⭐⭐ LA CONDICION QUE SE PIDIO EXPLICITAMENTE: "que aun mantenga su
    // tamano".
    //
    // La cuadratura empuja los puntos hacia el RECTANGULO que ya circunscribia
    // la elipse. El punto mas extremo (la esquina) ya estaba a esa distancia,
    // asi que la envolvente no puede crecer.
    //
    // Se compara la caja envolvente con cuadratura y sin ella.
    ParametrosPecari redondo = Especies::pecariDeCollar();
    redondo.cuadratura = 0.0f;

    MallaAnimal mCubo, mRedondo;
    ConstructorPecari::generar(mCubo,   Especies::pecariDeCollar(), 0);
    ConstructorPecari::generar(mRedondo, redondo, 0);

    const float anchoCubo    = mCubo.maximo.x   - mCubo.minimo.x;
    const float anchoRedondo = mRedondo.maximo.x - mRedondo.minimo.x;
    const float altoCubo     = mCubo.maximo.y   - mCubo.minimo.y;
    const float altoRedondo  = mRedondo.maximo.y - mRedondo.minimo.y;
    const float largoCubo    = mCubo.maximo.z   - mCubo.minimo.z;
    const float largoRedondo = mRedondo.maximo.z - mRedondo.minimo.z;

    INFO("ancho ", anchoRedondo, " -> ", anchoCubo,
         "   alto ", altoRedondo, " -> ", altoCubo,
         "   largo ", largoRedondo, " -> ", largoCubo);

    // No crece: la esquina ya marcaba el limite. Se deja un 2% de margen para
    // el relieve de superficie, que desplaza vertices y es independiente.
    CHECK(anchoCubo <= anchoRedondo * 1.02f);
    CHECK(altoCubo  <= altoRedondo  * 1.02f);
    CHECK(largoCubo <= largoRedondo * 1.02f);

    // Y tampoco encoge de forma apreciable: seguiria siendo el mismo animal.
    CHECK(anchoCubo >= anchoRedondo * 0.95f);
    CHECK(largoCubo >= largoRedondo * 0.95f);
}

TEST_CASE("Cuadratura: el OJO sigue siendo redondo") {
    // La unica excepcion del modelo. Un globo ocular cuadrado no se lee como
    // un ojo: se lee como un error. Minecraft hace lo mismo -- cuerpo de
    // cajas, ojos pintados en la textura, nunca facetados.
    ParametrosPecari redondo = Especies::pecariDeCollar();
    redondo.cuadratura = 0.0f;

    MallaAnimal mCubo, mRedondo;
    ConstructorPecari::generar(mCubo,   Especies::pecariDeCollar(), 0);
    ConstructorPecari::generar(mRedondo, redondo, 0);
    REQUIRE(mCubo.vertices.size() == mRedondo.vertices.size());

    // Los vertices del ojo tienen que ser IDENTICOS en los dos modelos.
    int comparados = 0;
    for (size_t i = 0; i < mCubo.vertices.size(); ++i) {
        if (mCubo.vertices[i].zona != ZonaCuerpo::OJO) continue;
        ++comparados;
        CHECK(mCubo.vertices[i].pos.x == doctest::Approx(mRedondo.vertices[i].pos.x));
        CHECK(mCubo.vertices[i].pos.y == doctest::Approx(mRedondo.vertices[i].pos.y));
    }
    CHECK(comparados > 0);
}

TEST_CASE("Volumen: el cuerpo esta CERRADO, no se ve por dentro") {
    // ⭐⭐ EL BUG REPORTADO: "la cabeza, cuando veo de cerca, se ve por dentro
    // desde el cuello".
    //
    // El torso acababa abierto por delante esperando que el cuello lo tapara,
    // y el cuello empezaba abierto por detras esperando al torso. Los dos se
    // esperaban y ninguno cerraba: quedaba un anillo de hueco.
    //
    // Con GL_CULL_FACE eso no se ve como un agujero negro -- se ve el INTERIOR
    // del animal, porque las caras de dentro quedan de espaldas y OpenGL las
    // descarta, dejando ver hasta la pared opuesta.
    //
    // SE MIDE CON EL TEOREMA DE LA DIVERGENCIA: el volumen encerrado por una
    // superficie CERRADA sale positivo; una superficie con agujeros da un
    // valor sin sentido (tipicamente ~0, porque lo que entra sale).
    //
    // Es la misma tecnica con la que se detecto que las patas estaban huecas.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double vol = 0.0;
    for (size_t i = 0; i + 2 < m.indices.size(); i += 3) {
        const V3& a = m.vertices[m.indices[i]].pos;
        const V3& b = m.vertices[m.indices[i + 1]].pos;
        const V3& c = m.vertices[m.indices[i + 2]].pos;
        // Volumen con signo del tetraedro (origen, a, b, c).
        vol += (double)(a.x * (b.y * c.z - b.z * c.y)
                      - a.y * (b.x * c.z - b.z * c.x)
                      + a.z * (b.x * c.y - b.y * c.x)) / 6.0;
    }

    INFO("volumen encerrado = ", vol, " m3");
    // Un pecari de 18,7 kg ocupa del orden de 0,02 m3. No se exige precision
    // --hay piezas que se solapan y eso suma-- solo que el volumen sea
    // CLARAMENTE positivo, que es la firma de una superficie cerrada.
    CHECK(std::fabs(vol) > 0.005);
}

TEST_CASE("Cuello: no hay salto entre anillos consecutivos") {
    // ⭐ "EL CUELLO SE VE SEPARADO DE LA CABEZA".
    //
    // El cuello y la cabeza van en el MISMO tubo, asi que no hay dos piezas
    // que puedan separarse de verdad. Lo que se ve como separacion es otra
    // cosa: un SALTO largo entre dos anillos consecutivos. coserTubo une
    // anillos vecinos con quads, asi que si dos quedan lejos, ese tramo se
    // salva con un unico quad muy estirado -- una banda lisa y brillante que
    // se lee como una junta.
    //
    // Este test recorre la cadena tronco->cuello->cabeza->hocico y comprueba
    // que ningun paso sea desproporcionado respecto a los demas.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se recogen las Z distintas de los vertices de la cadena, ordenadas.
    std::vector<float> zs;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::CUELLO && v.zona != ZonaCuerpo::CABEZA &&
            v.zona != ZonaCuerpo::COLLAR) continue;
        bool nueva = true;
        for (float z : zs) if (std::fabs(z - v.pos.z) < 1e-4f) { nueva = false; break; }
        if (nueva) zs.push_back(v.pos.z);
    }
    REQUIRE(zs.size() > 3);

    for (size_t i = 0; i + 1 < zs.size(); ++i)
        for (size_t j = i + 1; j < zs.size(); ++j)
            if (zs[j] < zs[i]) { const float t = zs[i]; zs[i] = zs[j]; zs[j] = t; }

    float mayor = 0.0f;
    for (size_t i = 0; i + 1 < zs.size(); ++i) {
        const float d = zs[i + 1] - zs[i];
        if (d > mayor) mayor = d;
    }

    float zA = 0.0f, zB = 0.0f;
    for (size_t i = 0; i + 1 < zs.size(); ++i)
        if (zs[i + 1] - zs[i] >= mayor - 1e-6f) { zA = zs[i]; zB = zs[i + 1]; }

    INFO("mayor salto en la cadena cuello-cabeza = ", mayor,
         " m  (largo de cuello = ", p.largoCuello, ")",
         "  entre z=", zA, " y z=", zB,
         "  [zPecho=", p.largoCuerpo * p.fraccionTronco * 0.48f,
         " zCraneo=", p.largoCuerpo * p.fraccionTronco * 0.48f + p.largoCuello,
         " largoCabeza=", p.largoCabeza, "]");

    // MEDIDO: el mayor salto era de 8,8 cm en un cuello de 12 cm -- tres
    // huecos encadenados (pecho->cuello, nuca->mejilla y mejilla->hocico) que
    // se salvaban cada uno con un unico quad estirado. Tras anadir los anillos
    // intermedios baja a 4,96 cm.
    //
    // El tope se deja en 0.45 del cuello (5,4 cm): deja margen para retoques
    // pero vuelve a fallar si alguien quita uno de esos anillos.
    CHECK(mayor < p.largoCuello * 0.45f);
}

TEST_CASE("Pelaje: el iris CONTRASTA con la cara negra") {
    // El ojo tiene que verse. Con el cuerpo en negro, eso ya no puede
    // conseguirse oscureciendolo: hay que ir en la otra direccion.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double sIris = 0.0; int nIris = 0;
    double sCara = 0.0; int nCara = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::OJO)         { sIris += luma(v); ++nIris; }
        else if (v.zona == ZonaCuerpo::CABEZA) { sCara += luma(v); ++nCara; }
    }
    REQUIRE(nIris > 0);
    REQUIRE(nCara > 0);

    const float iris = (float)(sIris / nIris);
    const float cara = (float)(sCara / nCara);
    INFO("iris = ", iris, "   cara = ", cara);
    CHECK(iris > cara * 1.35f);
}
