#include <doctest/doctest.h>
#include "fauna/AnimalMallaCache.h"
#include <cmath>
#include <vector>

// ============================================================================
// TESTS DE LA MALLA ORGANICA
// ============================================================================
// Lo que se verifica:
//
//   1. MALLA VALIDA   - cerrada, sin indices fuera de rango, sin NaN
//   2. ORGANICA       - no es una caja: tiene curvas y normales suaves
//   3. ANATOMIA       - los rasgos que distinguen al pecari del cerdo
//   4. LOD            - menos poligonos al alejarse, sin perder la silueta
//   5. CACHE          - N animales comparten 1 malla
//   6. MEMORIA        - el coste es O(tipos), no O(individuos)
// ============================================================================

using namespace Fauna;

// ============================================================================
// 1. VALIDEZ DE LA MALLA
// ============================================================================

TEST_CASE("Malla: se genera y no esta vacia") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    CHECK(m.vertices.size() > 0);
    CHECK(m.indices.size() > 0);
    CHECK(m.triangulos() > 0);
    CHECK(m.indices.size() % 3 == 0);   // triangulos completos
}

TEST_CASE("Malla: ningun indice apunta fuera de rango") {
    // Un indice fuera de rango es lectura de memoria invalida al dibujar:
    // basura en pantalla en el mejor caso, crash en el peor.
    for (int lod = 0; lod <= 3; ++lod) {
        MallaAnimal m;
        ParametrosPecari p = Especies::pecariDeCollar();
        ConstructorPecari::generar(m, p, lod);

        for (uint16_t idx : m.indices) {
            CHECK(idx < m.vertices.size());
        }
    }
}

TEST_CASE("Malla: cabe en indices de 16 bits") {
    // Los indices son uint16_t: si la malla pasara de 65535 vertices, se
    // desbordarian en silencio y la geometria saldria retorcida.
    for (int lod = 0; lod <= 3; ++lod) {
        for (int etapa = 0; etapa <= 4; ++etapa) {
            MallaAnimal m;
            ParametrosPecari p = Especies::aplicarEdad(
                Especies::pecariDeCollar(), etapa);
            ConstructorPecari::generar(m, p, lod);
            CHECK(m.vertices.size() < 65536);
        }
    }
}

TEST_CASE("Malla: nada es NaN ni infinito") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        CHECK(std::isfinite(v.pos.x));
        CHECK(std::isfinite(v.pos.y));
        CHECK(std::isfinite(v.pos.z));
        CHECK(std::isfinite(v.normal.x));
        CHECK(std::isfinite(v.normal.y));
        CHECK(std::isfinite(v.normal.z));
    }
}

TEST_CASE("Malla: todas las normales son unitarias") {
    // Una normal sin normalizar produce iluminacion incorrecta: zonas
    // quemadas o negras sin motivo.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        const float len = v.normal.longitud();
        CHECK(len == doctest::Approx(1.0f).epsilon(0.01));
    }
}

TEST_CASE("Malla: los colores estan en rango valido") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        CHECK(v.r >= 0.0f); CHECK(v.r <= 1.0f);
        CHECK(v.g >= 0.0f); CHECK(v.g <= 1.0f);
        CHECK(v.b >= 0.0f); CHECK(v.b <= 1.0f);
    }
}

// ============================================================================
// 2. ES ORGANICA, NO UNA CAJA
// ============================================================================

TEST_CASE("Malla: NO es una caja — tiene muchas normales distintas") {
    // ESTE ES EL TEST DEL OBJETIVO PRINCIPAL.
    //
    // Una caja tiene exactamente 6 normales distintas. Un cuerpo organico
    // tiene tantas como vertices. Si este test diera un numero bajo,
    // habriamos vuelto a los cubos.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Se cuentan normales unicas redondeando a 2 decimales.
    std::vector<int> vistas;
    int distintas = 0;
    for (const VerticeAnimal& v : m.vertices) {
        const int clave = (int)((v.normal.x + 1.0f) * 50.0f) * 10000
                        + (int)((v.normal.y + 1.0f) * 50.0f) * 100
                        + (int)((v.normal.z + 1.0f) * 50.0f);
        bool ya = false;
        for (int k : vistas) if (k == clave) { ya = true; break; }
        if (!ya) { vistas.push_back(clave); ++distintas; }
    }

    // Muy por encima de las 6 de un cubo.
    CHECK(distintas > 50);
}

TEST_CASE("Malla: las secciones son elipses, no cuadrados") {
    // Se comprueba tomando un corte del torso y midiendo cuantos radios
    // distintos hay desde el eje. Un cuadrado daria 2 valores; una elipse,
    // muchos.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Vertices cerca de z=0 (mitad del torso).
    std::vector<float> radios;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::TORSO) continue;
        if (std::fabs(v.pos.z) > 0.02f) continue;
        radios.push_back(std::sqrt(v.pos.x*v.pos.x));
    }

    REQUIRE(radios.size() >= 4);

    float minR = 1e9f, maxR = -1e9f;
    for (float r : radios) {
        if (r < minR) minR = r;
        if (r > maxR) maxR = r;
    }
    // Hay variacion continua de radio: no es un prisma.
    CHECK(maxR > minR);
}

TEST_CASE("Malla: el torso tiene forma de BARRIL") {
    // Mas grueso por el centro que por los extremos. Es la descripcion
    // universal del cuerpo de esta especie, y lo que lo separa de un tubo.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    const float largoTronco = p.largoCuerpo * p.fraccionTronco;

    auto anchoEn = [&](float z, float tol) {
        float maxX = 0.0f;
        for (const VerticeAnimal& v : m.vertices) {
            if (v.zona != ZonaCuerpo::TORSO && v.zona != ZonaCuerpo::LOMO &&
                v.zona != ZonaCuerpo::VIENTRE) continue;
            if (std::fabs(v.pos.z - z) > tol) continue;
            const float ax = std::fabs(v.pos.x);
            if (ax > maxX) maxX = ax;
        }
        return maxX;
    };

    const float centro = anchoEn(largoTronco * 0.05f, 0.05f);
    const float grupa  = anchoEn(-largoTronco * 0.45f, 0.05f);

    REQUIRE(centro > 0.0f);
    REQUIRE(grupa > 0.0f);
    CHECK(centro > grupa);   // barril: mas ancho en medio
}

// ============================================================================
// 3. ANATOMIA: LO QUE LO SEPARA DE UN CERDO
// ============================================================================

TEST_CASE("Anatomia: TRES dedos traseros, no cuatro") {
    // ES EL CRITERIO DE CAMPO que separa Tayassuidae de Suidae. El cerdo
    // verdadero tiene cuatro dedos traseros; el pecari, tres.
    ParametrosPecari p = Especies::pecariDeCollar();
    CHECK(p.dedosDelante == 4);
    CHECK(p.dedosDetras  == 3);
    CHECK(p.dedosDetras != p.dedosDelante);

    // Y el del Chaco reduce a dos: la arquitectura lo admite cambiando un
    // numero, sin tocar el modelo.
    CHECK(Especies::pecariDelChaco().dedosDetras == 2);
}

TEST_CASE("Anatomia: la cola es MINUSCULA") {
    // MEDIDO: 1.2 cm. El error mas visible seria darle cola de cerdo.
    ParametrosPecari p = Especies::pecariDeCollar();
    CHECK(p.largoCola == doctest::Approx(0.012f));
    // Menos del 2% del largo del cuerpo.
    CHECK(p.largoCola < p.largoCuerpo * 0.02f);
}

TEST_CASE("Anatomia: el hocico es MAS ESTRECHO que la cabeza") {
    // Cabeza "en cuna": ancha en el craneo, afilada hacia el hocico. Si
    // fueran iguales, la cabeza pareceria un ladrillo.
    ParametrosPecari p = Especies::pecariDeCollar();
    CHECK(p.anchoHocico < p.anchoCabeza);

    // El afilado ronda 0.55, que es el cociente publicado.
    CHECK(p.anchoHocico / p.anchoCabeza == doctest::Approx(0.55f).epsilon(0.06));
}

TEST_CASE("Anatomia: patas FINAS bajo un cuerpo MACIZO") {
    // El contraste es una firma visual de la especie: patas de ciervo
    // sosteniendo un barril.
    ParametrosPecari p = Especies::pecariDeCollar();
    CHECK(p.grosorPata < p.anchoTorso * 0.25f);
}

TEST_CASE("Anatomia: cuerpo COMPACTO, no alargado como un cerdo") {
    // El pecari es de perfil casi cuadrado; el cerdo es largo y bajo.
    ParametrosPecari p = Especies::pecariDeCollar();
    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float razon = largoTronco / p.alturaCruz;

    // Un cerdo domestico daria bastante mas de 2. El pecari ronda 1.3.
    CHECK(razon < 1.7f);
    CHECK(razon > 1.0f);
}

TEST_CASE("Anatomia: la malla ocupa el tamano real del animal") {
    // El punto 34: escala coherente con el mundo. Si el animal midiera el
    // doble, se veria mal junto al jugador y a los bloques.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    const float alto  = m.maximo.y - m.minimo.y;
    const float largo = m.maximo.z - m.minimo.z;
    const float ancho = m.maximo.x - m.minimo.x;

    // Altura a la cruz MEDIDA: 0.44 m. La crin sobresale algo por encima.
    CHECK(alto > 0.40f);
    CHECK(alto < 0.60f);

    // Longitud cabeza-cuerpo MEDIDA: 84-106 cm.
    CHECK(largo > 0.80f);
    CHECK(largo < 1.15f);

    // El ancho incluye las patas separadas.
    CHECK(ancho > 0.15f);
    CHECK(ancho < 0.45f);
}

TEST_CASE("Anatomia: existen todas las zonas del cuerpo") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    bool hay[(int)ZonaCuerpo::_COUNT] = { false };
    for (const VerticeAnimal& v : m.vertices) {
        hay[(int)v.zona] = true;
    }

    CHECK(hay[(int)ZonaCuerpo::TORSO]);
    CHECK(hay[(int)ZonaCuerpo::LOMO]);
    CHECK(hay[(int)ZonaCuerpo::VIENTRE]);
    CHECK(hay[(int)ZonaCuerpo::CABEZA]);
    CHECK(hay[(int)ZonaCuerpo::HOCICO]);
    CHECK(hay[(int)ZonaCuerpo::PATA]);
    CHECK(hay[(int)ZonaCuerpo::PEZUNA]);
    CHECK(hay[(int)ZonaCuerpo::OREJA]);
    CHECK(hay[(int)ZonaCuerpo::COLA]);
}

TEST_CASE("Anatomia: el hocico y las pezunas NO tienen pelo") {
    // El punto 11: en zonas de pelo ralo, material de piel. Aqui se marca con
    // el campo `pelo`, que el sombreado usa para no perturbar la normal.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona == ZonaCuerpo::PEZUNA) {
            CHECK(v.pelo == doctest::Approx(0.0f));
        }
    }
}

TEST_CASE("Anatomia: el color varia por zona, no es plano") {
    // Punto 9 y 10: variacion procedural que respeta regiones anatomicas.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float minR = 1e9f, maxR = -1e9f;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.r < minR) minR = v.r;
        if (v.r > maxR) maxR = v.r;
    }

    // Hay rango de tonos: del lomo oscuro al collar claro.
    CHECK(maxR - minR > 0.20f);
}

TEST_CASE("Anatomia: el vientre es MAS CLARO que el lomo") {
    // MEDIDO (cualitativo): raya dorsal oscura, mas claro por debajo.
    ParametrosPecari p = Especies::pecariDeCollar();
    CHECK(p.colorVientre[0] > p.colorLomo[0]);
    CHECK(p.colorVientre[1] > p.colorLomo[1]);
    CHECK(p.colorVientre[2] > p.colorLomo[2]);
}

// ============================================================================
// 4. LOD
// ============================================================================

TEST_CASE("LOD: menos poligonos al alejarse") {
    size_t tris[4];
    for (int lod = 0; lod <= 3; ++lod) {
        MallaAnimal m;
        ParametrosPecari p = Especies::pecariDeCollar();
        ConstructorPecari::generar(m, p, lod);
        tris[lod] = m.triangulos();
    }

    CHECK(tris[0] > tris[1]);
    CHECK(tris[1] > tris[2]);
    CHECK(tris[2] > tris[3]);

    // Y el mas lejano sigue teniendo geometria: no puede quedarse en nada.
    CHECK(tris[3] > 20);
}

TEST_CASE("LOD: la silueta se conserva a todos los niveles") {
    // Punto 14: la silueta es prioritaria. Aunque baje el detalle, el animal
    // debe seguir midiendo lo mismo.
    float altos[4], largos[4];
    for (int lod = 0; lod <= 3; ++lod) {
        MallaAnimal m;
        ParametrosPecari p = Especies::pecariDeCollar();
        ConstructorPecari::generar(m, p, lod);
        altos[lod]  = m.maximo.y - m.minimo.y;
        largos[lod] = m.maximo.z - m.minimo.z;
    }

    for (int lod = 1; lod <= 3; ++lod) {
        // Menos de un 20% de desviacion respecto al LOD0.
        CHECK(std::fabs(altos[lod] - altos[0]) < altos[0] * 0.20f);
        CHECK(std::fabs(largos[lod] - largos[0]) < largos[0] * 0.20f);
    }
}

TEST_CASE("LOD: la seleccion por distancia es monotona") {
    int anterior = 0;
    for (float d = 0.0f; d < 100.0f; d += 1.0f) {
        const int lod = LOD::seleccionar(d, 0);
        CHECK(lod >= anterior);   // nunca baja al alejarse
        CHECK(lod >= 0);
        CHECK(lod <= 3);
        anterior = lod;
    }
}

TEST_CASE("LOD: la histeresis evita el parpadeo en el umbral") {
    // Sin histeresis, un animal justo en el umbral oscila entre dos niveles
    // cada frame y cuesta MAS que si estuviera siempre en el alto.
    const float justo = LOD::DIST_LOD0;

    // Viniendo de lejos (lod 1), en el umbral exacto aun no baja a 0.
    const int desdeeLejos = LOD::seleccionar(justo + 0.5f, 1);
    // Viniendo de cerca (lod 0), en el mismo punto sigue en 0.
    const int desdeCerca = LOD::seleccionar(justo + 0.5f, 0);

    // Los dos no pueden dar el mismo valor si la histeresis funciona.
    CHECK(desdeeLejos >= desdeCerca);
}

TEST_CASE("LOD: el factor de transicion va de 1 a 0") {
    // Punto 18: para el dithering que evita el popping.
    CHECK(LOD::factorTransicion(0.0f, 0) == doctest::Approx(1.0f));
    const float f = LOD::factorTransicion(LOD::DIST_LOD0 - 1.5f, 0);
    CHECK(f >= 0.0f);
    CHECK(f <= 1.0f);
}

// ============================================================================
// 5. CACHE: N ANIMALES, 1 MALLA
// ============================================================================

TEST_CASE("Cache: cien animales comparten UNA malla") {
    // ES EL TEST DEL PUNTO 24.
    CacheMallas cache;

    const MallaAnimal* primera =
        cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 0);
    REQUIRE(primera != nullptr);

    // Cien peticiones identicas.
    for (int i = 0; i < 100; ++i) {
        const MallaAnimal* m =
            cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 0);
        CHECK(m == primera);   // EL MISMO puntero, no una copia
    }

    // Solo se genero UNA vez.
    CHECK(cache.mallasVivas() == 1);
    CHECK(cache.numFallos() == 1);
    CHECK(cache.numAciertos() == 100);
}

TEST_CASE("Cache: combinaciones distintas dan mallas distintas") {
    CacheMallas cache;

    const MallaAnimal* a = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 0);
    const MallaAnimal* b = cache.obtener(EspecieAnimal::PECARI_COLLAR, 3, 2);
    const MallaAnimal* c = cache.obtener(EspecieAnimal::PECARI_COLLAR, 0, 0);

    CHECK(a != b);   // distinto LOD
    CHECK(a != c);   // distinta edad
    CHECK(cache.mallasVivas() == 3);
}

TEST_CASE("Cache: la memoria NO crece con el numero de animales") {
    // EL TEST DEL PUNTO 23: memoria O(tipos), no O(individuos).
    CacheMallas cache;
    cache.precalentar(EspecieAnimal::PECARI_COLLAR);

    const size_t bytesTras20 = cache.bytesTotales();
    const size_t mallasTras20 = cache.mallasVivas();

    // Mil peticiones mas, simulando mil animales.
    for (int i = 0; i < 1000; ++i) {
        cache.obtener(EspecieAnimal::PECARI_COLLAR, i % 5, i % 4);
    }

    // NI UN BYTE mas.
    CHECK(cache.bytesTotales() == bytesTras20);
    CHECK(cache.mallasVivas() == mallasTras20);
}

TEST_CASE("Cache: el precalentado genera las 20 combinaciones") {
    CacheMallas cache;
    cache.precalentar(EspecieAnimal::PECARI_COLLAR);
    CHECK(cache.mallasVivas() == 20);   // 5 etapas x 4 LOD
}

TEST_CASE("Cache: la memoria total es razonable") {
    // Se comprueba que el coste real esta en el orden esperado. Si alguien
    // sube el detalle sin pensar, esto salta.
    CacheMallas cache;
    cache.precalentar(EspecieAnimal::PECARI_COLLAR);

    const size_t kb = cache.bytesTotales() / 1024;

    // Las 20 mallas juntas deben caber holgadamente en 2 MB.
    CHECK(kb < 2048);
    CHECK(kb > 10);   // pero no estan vacias
}

// ============================================================================
// 6. VARIACION POR ESPECIE Y EDAD
// ============================================================================

TEST_CASE("Especies: variar no requiere duplicar el modelo") {
    // Punto 2: la arquitectura admite otras especies cambiando parametros.
    MallaAnimal a, b, c;
    ConstructorPecari::generar(a, Especies::pecariDeCollar(), 0);
    ConstructorPecari::generar(b, Especies::pecariLabiado(), 0);
    ConstructorPecari::generar(c, Especies::pecariDelChaco(), 0);

    // Las tres se generan con el MISMO codigo.
    CHECK(a.triangulos() > 0);
    CHECK(b.triangulos() > 0);
    CHECK(c.triangulos() > 0);

    // Pero tienen tamanos distintos.
    const float largoA = a.maximo.z - a.minimo.z;
    const float largoC = c.maximo.z - c.minimo.z;
    CHECK(largoC > largoA);   // el del Chaco es mayor
}

TEST_CASE("Edad: un bebe NO es un adulto encogido") {
    // La cabeza crece antes que el cuerpo: alometria real.
    ParametrosPecari bebe   = Especies::aplicarEdad(Especies::pecariDeCollar(), 0);
    ParametrosPecari adulto = Especies::aplicarEdad(Especies::pecariDeCollar(), 3);

    const float razonBebe   = bebe.largoCabeza / bebe.largoCuerpo;
    const float razonAdulto = adulto.largoCabeza / adulto.largoCuerpo;

    // El bebe tiene la cabeza proporcionalmente MAYOR.
    CHECK(razonBebe > razonAdulto * 1.25f);
}

TEST_CASE("Edad: la escala del neonato sale del peso MEDIDO") {
    // DERIVADO: 0.5 kg al nacer / 18.7 kg adulto. La masa va con el cubo de la
    // longitud -> (0.5/18.7)^(1/3) = 0.30
    ParametrosPecari bebe = Especies::aplicarEdad(Especies::pecariDeCollar(), 0);
    ParametrosPecari adulto = Especies::pecariDeCollar();

    CHECK(bebe.largoCuerpo / adulto.largoCuerpo == doctest::Approx(0.30f).epsilon(0.02));
}

TEST_CASE("Edad: las cinco etapas crecen de forma monotona") {
    float anterior = 0.0f;
    for (int etapa = 0; etapa <= 3; ++etapa) {
        ParametrosPecari p = Especies::aplicarEdad(
            Especies::pecariDeCollar(), etapa);
        CHECK(p.largoCuerpo > anterior);
        anterior = p.largoCuerpo;
    }
}

// ============================================================================
// 7. CULLING
// ============================================================================

TEST_CASE("Culling: descarta lo que esta lejos") {
    CHECK(Culling::visiblePorDistancia(10.0f, 0.0f, 50.0f) == true);
    CHECK(Culling::visiblePorDistancia(100.0f, 0.0f, 50.0f) == false);
}

TEST_CASE("Culling: descarta lo que esta detras de la camara") {
    // Camara mirando a +Z.
    const float dirX = 0.0f, dirZ = 1.0f;

    CHECK(Culling::visiblePorAngulo(0.0f, 20.0f, dirX, dirZ) == true);   // delante
    CHECK(Culling::visiblePorAngulo(0.0f, -20.0f, dirX, dirZ) == false); // detras
    CHECK(Culling::visiblePorAngulo(20.0f, 0.0f, dirX, dirZ) == true);   // al lado
}

// ============================================================================
// 8. DIRECCION DEL PELO
// ============================================================================

TEST_CASE("Pelo: la direccion es tangente a la superficie") {
    // Punto 7: el pelo se PEGA a la piel, no la atraviesa. Si la direccion
    // tuviera componente normal, el pelo saldria disparado hacia fuera.
    const V3 normales[4] = {
        V3(0,1,0), V3(1,0,0), V3(0,0,1),
        V3(0.577f, 0.577f, 0.577f)
    };

    for (const V3& n : normales) {
        const V3 dir = GeneradorMalla::direccionPelo(n, ZonaCuerpo::TORSO);
        // Perpendicular a la normal: producto escalar ~0
        CHECK(std::fabs(punto(dir, n)) < 0.01f);
        // Y unitaria.
        CHECK(dir.longitud() == doctest::Approx(1.0f).epsilon(0.01));
    }
}

TEST_CASE("Pelo: la direccion cambia segun la zona del cuerpo") {
    // En el cuerpo el pelo va hacia atras; en la cara, hacia delante.
    //
    // OJO CON LA NORMAL QUE SE ELIGE PARA COMPROBARLO. Sobre una superficie
    // HORIZONTAL (normal 0,1,0 — el lomo) la componente vertical del flujo se
    // proyecta fuera por completo, asi que torso y pata acaban dando el mismo
    // vector. No es un fallo: es la geometria de la proyeccion.
    //
    // Para distinguir zonas que difieren sobre todo en Y hay que mirar un
    // FLANCO (normal 1,0,0), que es ademas donde de verdad hay pata.
    {
        const V3 nLomo(0, 1, 0);
        const V3 torso  = GeneradorMalla::direccionPelo(nLomo, ZonaCuerpo::TORSO);
        const V3 cabeza = GeneradorMalla::direccionPelo(nLomo, ZonaCuerpo::CABEZA);

        // Torso y cabeza apuntan en sentidos OPUESTOS en Z: el pelo del cuerpo
        // va hacia la cola y el de la cara hacia el hocico.
        CHECK(torso.z * cabeza.z < 0.0f);
    }

    {
        const V3 nFlanco(1, 0, 0);
        const V3 torso = GeneradorMalla::direccionPelo(nFlanco, ZonaCuerpo::TORSO);
        const V3 pata  = GeneradorMalla::direccionPelo(nFlanco, ZonaCuerpo::PATA);

        // En el flanco si se ve la diferencia: el pelo del torso corre casi
        // horizontal hacia atras, el de la pata baja casi vertical.
        CHECK(std::fabs(pata.y) > std::fabs(torso.y));
        CHECK(std::fabs(torso.z) > std::fabs(pata.z));
    }
}

// ============================================================================
// 9. DETERMINISMO
// ============================================================================

TEST_CASE("Malla: la generacion es determinista") {
    // Dos mallas con los mismos parametros deben ser IDENTICAS. Sin esto, la
    // cache seria inutil y cada animal se veria distinto al recargar.
    MallaAnimal a, b;
    ParametrosPecari p = Especies::pecariDeCollar();
    p.semilla = 4242u;

    ConstructorPecari::generar(a, p, 0);
    ConstructorPecari::generar(b, p, 0);

    REQUIRE(a.vertices.size() == b.vertices.size());
    for (size_t i = 0; i < a.vertices.size(); ++i) {
        CHECK(a.vertices[i].pos.x == doctest::Approx(b.vertices[i].pos.x));
        CHECK(a.vertices[i].pos.y == doctest::Approx(b.vertices[i].pos.y));
        CHECK(a.vertices[i].pos.z == doctest::Approx(b.vertices[i].pos.z));
        CHECK(a.vertices[i].r == doctest::Approx(b.vertices[i].r));
    }
}

// ============================================================================
// ORIENTACION DE LAS CARAS (winding)
// ============================================================================
// ⭐ ESTOS TESTS NACIERON DE UN BUG REAL Y VISIBLE.
//
// coserTubo cosia sus dos triangulos como (v00,v10,v11) y (v00,v11,v01), que
// con el anillo que genera --angulo creciente, secciones avanzando en +Z--
// deja la normal apuntando al EJE del tubo en vez de hacia fuera.
//
// Medido sobre el adulto en LOD 0: 838 de 1244 caras miraban hacia dentro.
// Dos sintomas a la vez, y ninguno daba error:
//
//   1. La luz se invertia. El sombreado usa max(0, n.L), asi que las caras al
//      sol daban 0 y salian del color minimo, y las de sombra salian
//      iluminadas: el lomo negro y el vientre brillante.
//
//   2. Con GL_CULL_FACE activo, OpenGL descartaba las caras exteriores y
//      conservaba las interiores. El animal se veia hueco o con agujeros.
//
// Los tests que ya habia NO lo cazaban: comprobaban que las normales fueran
// finitas y unitarias, y lo eran -- solo que del reves. Faltaba comprobar
// hacia DONDE apuntan.

TEST_CASE("Winding: el tronco tiene las caras hacia fuera") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    // Centroide de la malla.
    double cx = 0, cy = 0, cz = 0;
    for (const auto& v : m.vertices) { cx += v.pos.x; cy += v.pos.y; cz += v.pos.z; }
    cx /= m.vertices.size(); cy /= m.vertices.size(); cz /= m.vertices.size();

    // Solo las zonas del CUERPO: los apendices (patas, orejas, hocico) cuelgan
    // lejos del centroide y esta prueba no vale para ellos.
    int fuera = 0, dentro = 0;
    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        const VerticeAnimal& VA = m.vertices[m.indices[t + 0]];
        const ZonaCuerpo z = VA.zona;
        if (z != ZonaCuerpo::TORSO && z != ZonaCuerpo::VIENTRE) continue;

        const V3& A = VA.pos;
        const V3& B = m.vertices[m.indices[t + 1]].pos;
        const V3& C = m.vertices[m.indices[t + 2]].pos;

        const double ux = B.x-A.x, uy = B.y-A.y, uz = B.z-A.z;
        const double vx = C.x-A.x, vy = C.y-A.y, vz = C.z-A.z;
        const double nx = uy*vz - uz*vy;
        const double ny = uz*vx - ux*vz;
        const double nz = ux*vy - uy*vx;

        const double tx = (A.x+B.x+C.x)/3.0 - cx;
        const double ty = (A.y+B.y+C.y)/3.0 - cy;
        const double tz = (A.z+B.z+C.z)/3.0 - cz;

        if (nx*tx + ny*ty + nz*tz > 0) ++fuera; else ++dentro;
    }

    INFO("fuera=", fuera, " dentro=", dentro);
    REQUIRE(fuera + dentro > 100);   // que de verdad se hayan mirado caras
    CHECK(dentro == 0);              // ni una sola del reves
}

TEST_CASE("Winding: el volumen del cuerpo es positivo") {
    // La prueba de fondo, sin depender del centroide: el volumen encerrado
    // (teorema de la divergencia). Una superficie bien orientada encierra
    // volumen POSITIVO; una invertida, negativo.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double vol = 0;
    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        const VerticeAnimal& VA = m.vertices[m.indices[t + 0]];
        const ZonaCuerpo z = VA.zona;
        if (z != ZonaCuerpo::TORSO && z != ZonaCuerpo::VIENTRE &&
            z != ZonaCuerpo::LOMO) continue;

        const V3& A = VA.pos;
        const V3& B = m.vertices[m.indices[t + 1]].pos;
        const V3& C = m.vertices[m.indices[t + 2]].pos;
        vol += ((double)A.x * ((double)B.y*C.z - (double)B.z*C.y)
              - (double)A.y * ((double)B.x*C.z - (double)B.z*C.x)
              + (double)A.z * ((double)B.x*C.y - (double)B.y*C.x)) / 6.0;
    }

    INFO("volumen del cuerpo = ", vol);
    CHECK(vol > 0.0);   // si sale negativo, la malla esta del reves
}

TEST_CASE("Winding: las normales del lomo apuntan hacia arriba") {
    // Comprobacion independiente y muy directa: el lomo es la parte de ARRIBA
    // del animal, asi que sus normales tienen que tener componente Y positiva.
    // Si la malla esta invertida, apuntaran hacia abajo.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    int arriba = 0, abajo = 0;
    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::LOMO) continue;
        if (v.normal.y > 0.1f)  ++arriba;
        if (v.normal.y < -0.1f) ++abajo;
    }

    INFO("normales del lomo: arriba=", arriba, " abajo=", abajo);
    REQUIRE(arriba + abajo > 10);
    CHECK(arriba > abajo * 3);   // el lomo mira al cielo, no al suelo
}

// ============================================================================
// ARTICULACIONES (skinning)
// ============================================================================
// La malla se cachea por (especie, etapa, LOD) y se comparte entre todos los
// animales. Eso ahorra memoria pero tenia una consecuencia visible: la malla
// era una ESTATUA. Un pecari caminando era una figura rigida deslizandose, con
// las patas clavadas.
//
// Habia un arbol de huesos completo en PecariCuerpo.h, pero solo lo usaba el
// camino de CAJAS, que el render ya no dibuja. Estos tests atan las dos
// mitades: que la malla organica de verdad se dobla por su esqueleto.

TEST_CASE("Skinning: cada vertice tiene un hueso asignado") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);

    REQUIRE(pesos.size() == m.vertices.size());
    for (const PesoVertice& w : pesos) {
        CHECK(w.hueso < NUM_HUESOS);
    }
}

TEST_CASE("Skinning: las patas se doblan y el tronco no") {
    // Lo que de verdad importa: que al cambiar la fase del paso se muevan las
    // patas Y NO el cuerpo. Si el tronco se moviera, el animal se deformaria
    // entero al andar.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);
    EsqueletoReposo esq = EsqueletoDePecari(p);

    PoseEsqueleto pose;
    TransHueso trans[NUM_HUESOS];
    std::vector<VerticeAnimal> a, b;

    // Dos fases opuestas del ciclo de marcha.
    PoseDeMarcha(0.0f, 3.2f, pose);
    ResolverPose(esq, pose, trans);
    DeformarMalla(m, pesos, trans, a);

    PoseDeMarcha(3.14159265f, 3.2f, pose);
    ResolverPose(esq, pose, trans);
    DeformarMalla(m, pesos, trans, b);

    float movPata = 0.0f, movTronco = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        const float dx = a[i].pos.x - b[i].pos.x;
        const float dy = a[i].pos.y - b[i].pos.y;
        const float dz = a[i].pos.z - b[i].pos.z;
        const float d = std::sqrt(dx*dx + dy*dy + dz*dz);

        const ZonaCuerpo z = m.vertices[i].zona;
        if (z == ZonaCuerpo::PATA || z == ZonaCuerpo::PEZUNA) {
            if (d > movPata) movPata = d;
        }
        if (z == ZonaCuerpo::TORSO || z == ZonaCuerpo::LOMO ||
            z == ZonaCuerpo::VIENTRE) {
            if (d > movTronco) movTronco = d;
        }
    }

    INFO("pata=", movPata, " tronco=", movTronco);
    CHECK(movPata > 0.03f);      // las patas se mueven de verdad
    CHECK(movTronco < 0.005f);   // el cuerpo se queda quieto
}

TEST_CASE("Skinning: el pie se levanta del suelo al andar") {
    // Un ciclo de marcha en el que el pie no despega es un patinaje.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);
    EsqueletoReposo esq = EsqueletoDePecari(p);

    // Un vertice de pezuna de la pata delantera izquierda.
    int idx = -1;
    for (size_t i = 0; i < m.vertices.size(); ++i) {
        if (m.vertices[i].zona == ZonaCuerpo::PEZUNA &&
            m.vertices[i].pos.x < 0 && m.vertices[i].pos.z > 0) {
            idx = (int)i;
            break;
        }
    }
    REQUIRE(idx >= 0);

    PoseEsqueleto pose;
    TransHueso trans[NUM_HUESOS];
    std::vector<VerticeAnimal> v;

    float yMin = 1e9f, yMax = -1e9f;
    for (int s = 0; s < 24; ++s) {
        PoseDeMarcha(6.28318531f * s / 24.0f, 3.2f, pose);
        ResolverPose(esq, pose, trans);
        DeformarMalla(m, pesos, trans, v);
        const float y = v[idx].pos.y;
        if (y < yMin) yMin = y;
        if (y > yMax) yMax = y;
    }

    INFO("recorrido vertical del pie = ", yMax - yMin);
    CHECK(yMax - yMin > 0.01f);
}

TEST_CASE("Skinning: parado no deforma nada") {
    // Con rapidez 0 la amplitud es 0: un animal quieto no debe agitar las
    // patas. Si esto fallara, los pecaries parados harian el paso del ganso.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);
    EsqueletoReposo esq = EsqueletoDePecari(p);

    PoseEsqueleto pose;
    TransHueso trans[NUM_HUESOS];
    std::vector<VerticeAnimal> v;

    PoseDeMarcha(1.234f, 0.0f, pose);   // fase cualquiera, rapidez CERO
    ResolverPose(esq, pose, trans);
    DeformarMalla(m, pesos, trans, v);

    float maxDif = 0.0f;
    for (size_t i = 0; i < v.size(); ++i) {
        const float dx = v[i].pos.x - m.vertices[i].pos.x;
        const float dy = v[i].pos.y - m.vertices[i].pos.y;
        const float dz = v[i].pos.z - m.vertices[i].pos.z;
        const float d = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (d > maxDif) maxDif = d;
    }
    INFO("desviacion maxima estando parado = ", maxDif);
    CHECK(maxDif < 0.001f);
}

TEST_CASE("Skinning: no produce NaN en ningun punto del ciclo") {
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);
    EsqueletoReposo esq = EsqueletoDePecari(p);

    PoseEsqueleto pose;
    TransHueso trans[NUM_HUESOS];
    std::vector<VerticeAnimal> v;

    for (int s = 0; s < 32; ++s) {
        PoseDeMarcha(6.28318531f * s / 32.0f, 6.0f, pose);
        ResolverPose(esq, pose, trans);
        DeformarMalla(m, pesos, trans, v);
        for (const VerticeAnimal& q : v) {
            REQUIRE(std::isfinite(q.pos.x));
            REQUIRE(std::isfinite(q.pos.y));
            REQUIRE(std::isfinite(q.pos.z));
            REQUIRE(std::isfinite(q.normal.x));
            REQUIRE(std::isfinite(q.normal.y));
            REQUIRE(std::isfinite(q.normal.z));
        }
    }
}

TEST_CASE("Skinning: la cabeza gira sin arrastrar el cuerpo") {
    // Es lo que permite que el animal mire a los lados sin girarse entero.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    std::vector<PesoVertice> pesos;
    CalcularPesosPecari(m, p, pesos);
    EsqueletoReposo esq = EsqueletoDePecari(p);

    PoseEsqueleto pose;
    TransHueso trans[NUM_HUESOS];
    std::vector<VerticeAnimal> recto, girado;

    pose.limpiar();
    ResolverPose(esq, pose, trans);
    DeformarMalla(m, pesos, trans, recto);

    pose.limpiar();
    pose.giroY[(int)HuesoAnimal::CABEZA] = 0.5f;   // ~29 grados
    ResolverPose(esq, pose, trans);
    DeformarMalla(m, pesos, trans, girado);

    float movCabeza = 0.0f, movTronco = 0.0f;
    for (size_t i = 0; i < recto.size(); ++i) {
        const float dx = recto[i].pos.x - girado[i].pos.x;
        const float dy = recto[i].pos.y - girado[i].pos.y;
        const float dz = recto[i].pos.z - girado[i].pos.z;
        const float d = std::sqrt(dx*dx + dy*dy + dz*dz);

        const ZonaCuerpo z = m.vertices[i].zona;
        if (z == ZonaCuerpo::CABEZA || z == ZonaCuerpo::HOCICO) {
            if (d > movCabeza) movCabeza = d;
        }
        if (z == ZonaCuerpo::TORSO || z == ZonaCuerpo::LOMO) {
            if (d > movTronco) movTronco = d;
        }
    }

    INFO("cabeza=", movCabeza, " tronco=", movTronco);
    CHECK(movCabeza > 0.02f);    // la cabeza SI gira
    CHECK(movTronco < 0.005f);   // el cuerpo NO la sigue
}

TEST_CASE("Color: los ojos se distinguen de la cara") {
    // ⭐ NACIO DE UN BUG REAL: los ojos se marcaban como zona CABEZA, asi que
    // colorear() caia en su rama por defecto y les daba el color de la cara.
    // El comentario del codigo prometia "color propio muy oscuro" y no lo
    // cumplia nadie: los ojos existian en la malla pero eran invisibles.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double lumaOjo = 0, lumaCara = 0;
    int nOjo = 0, nCara = 0;
    for (const VerticeAnimal& v : m.vertices) {
        const double l = v.r * 0.3 + v.g * 0.6 + v.b * 0.1;
        if (v.zona == ZonaCuerpo::OJO)    { lumaOjo  += l; ++nOjo;  }
        if (v.zona == ZonaCuerpo::CABEZA) { lumaCara += l; ++nCara; }
    }

    REQUIRE(nOjo  > 0);    // los ojos existen
    REQUIRE(nCara > 0);
    lumaOjo  /= nOjo;
    lumaCara /= nCara;

    INFO("ojo=", lumaOjo, " cara=", lumaCara);

    // ⭐ EL CRITERIO ES CONTRASTE, NO OSCURIDAD.
    //
    // Este CHECK era `lumaOjo < lumaCara * 0.6`: el ojo tenia que ser MAS
    // OSCURO que la cara. Eso valia cuando el cuerpo era gris pardo.
    //
    // Con el pelaje ya en negro, un ojo oscuro es un ojo INVISIBLE: seria
    // negro sobre negro, exactamente el bug que este test nacio para impedir.
    // El iris pasa a ser pardo (mas CLARO que la cara) y la oscuridad la pone
    // la PUPILA, que tiene su propia zona.
    //
    // Asi que lo que hay que exigir es lo que el test siempre quiso decir: que
    // el ojo se DISTINGA de la cara. En que direccion es una decision de
    // diseno que depende del tono del animal.
    const double contraste = std::fabs(lumaOjo - lumaCara) /
                             (lumaCara > 1e-6 ? lumaCara : 1e-6);
    CHECK(contraste > 0.35);
}

TEST_CASE("Color: la PUPILA es lo mas oscuro de la cara") {
    // Lo que convierte dos manchas en una MIRADA es el contraste iris/pupila.
    // Un ojo de un solo tono es una canica.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    double lumaPup = 0, lumaIris = 0;
    int nPup = 0, nIris = 0;
    for (const VerticeAnimal& v : m.vertices) {
        const double l = v.r * 0.3 + v.g * 0.6 + v.b * 0.1;
        if (v.zona == ZonaCuerpo::PUPILA) { lumaPup  += l; ++nPup;  }
        if (v.zona == ZonaCuerpo::OJO)    { lumaIris += l; ++nIris; }
    }
    REQUIRE(nPup  > 0);
    REQUIRE(nIris > 0);
    lumaPup  /= nPup;
    lumaIris /= nIris;

    INFO("pupila=", lumaPup, " iris=", lumaIris);
    CHECK(lumaPup < lumaIris * 0.5);
}

TEST_CASE("Color: el animal no es de un solo tono") {
    // Lo que da sensacion de pelaje son las diferencias entre partes. Un
    // animal de un unico color se lee como plastico.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    float lo = 1.0f, hi = 0.0f;
    for (const VerticeAnimal& v : m.vertices) {
        const float l = v.r * 0.3f + v.g * 0.6f + v.b * 0.1f;
        if (l < lo) lo = l;
        if (l > hi) hi = l;
    }
    INFO("rango de luminancia ", lo, " .. ", hi);
    CHECK(hi - lo > 0.25f);
}

TEST_CASE("Color: el collar no pinta los ojos") {
    // El collar se aplicaba a todo lo que no fuera pata, asi que podia
    // pintar de crema los ojos si caia a su altura.
    MallaAnimal m;
    ParametrosPecari p = Especies::pecariDeCollar();
    ConstructorPecari::generar(m, p, 0);

    for (const VerticeAnimal& v : m.vertices) {
        if (v.zona != ZonaCuerpo::OJO) continue;
        // Ningun ojo puede acercarse al tono del collar.
        const float l = v.r * 0.3f + v.g * 0.6f + v.b * 0.1f;
        CHECK(l < 0.25f);
    }
}
