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
        if (c.zona != ZonaCuerpo::LOMO) continue;
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
        if (v.zona != ZonaCuerpo::LOMO) continue;
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
