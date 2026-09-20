#include <doctest/doctest.h>
#include "render/TexturaTuna.h"

#include <cstring>
#include <string>
#include <vector>

using namespace Render;

// ============================================================================
// LAS TUNAS NO SE DIBUJABAN
// ============================================================================
// BUG REPORTADO: las texturas de las tunas no se renderizaban. No era que
// tardaran -- el fruto simplemente no aparecia nunca.
//
// LA CAUSA ERAN TRES DECISIONES CORRECTAS QUE JUNTAS DEJABAN UN PUNTO CIEGO:
//
//   1. La textura de una tuna depende de su POSICION (variedad + madurez), asi
//      que no se puede pedir con `getBlockTexture(tipo, cara)`. El barrido
//      exhaustivo de la precarga, que recorre el enum pidiendo las seis caras
//      de cada bloque, NUNCA llegaba a la tuna.
//
//   2. El unico que la pedia era el MESHER, que corre en workers, donde
//      `puedeCargar()` es false a proposito (crear texturas de GL desde un
//      hilo de trabajo cuelga el driver). Devolvia 0.
//
//   3. Con 0, el mesher no emitia ninguna cara. Y esa rama no pasaba por
//      `texSegura`, que es quien marca `texturasFaltantes`, asi que el chunk
//      llegaba a LISTO sin fruto y sin nada pendiente: nadie volvia a pedir el
//      remallado.
//
// Estos tests fijan la parte que se puede probar sin OpenGL: que la eleccion
// del archivo y la lista de precarga NO PUEDEN SEPARARSE. Que se separen es,
// literalmente, el bug.

TEST_CASE("Tuna: cada variedad madura usa su propio color") {
    // La variedad la dice el BLOQUE, no la posicion: al romper una tuna roja
    // el jugador recoge una tuna roja, asi que lo que se ve y lo que se coge
    // tienen que salir del mismo dato.
    CHECK(std::string(archivoTuna(VariedadTuna::VERDE, true))
          == "Tuna verde crecida.png");
    CHECK(std::string(archivoTuna(VariedadTuna::AMARILLA, true))
          == "Tuna amarilla crecida.png");
    CHECK(std::string(archivoTuna(VariedadTuna::ROJA, true))
          == "Tuna roja crecida.png");
}

TEST_CASE("Tuna: sin madurar SIEMPRE es verde, sea cual sea la planta") {
    // El fruto nace como brote tierno y solo toma el color de su variedad al
    // madurar. Por eso hay CUATRO casos pero solo TRES archivos.
    for (VariedadTuna v : { VariedadTuna::VERDE, VariedadTuna::AMARILLA,
                            VariedadTuna::ROJA }) {
        CHECK(std::string(archivoTuna(v, false)) == "Tuna verde crecida.png");
    }
}

TEST_CASE("Tuna: siempre se usa la version CRECIDA") {
    // Medido: las versiones "crecida" tienen un 28% de pixeles vacios y las
    // pequenas un 58%. Sobre la caja 3D del fruto ese hueco se ve como
    // AGUJEROS por los que se mira a traves de la tuna.
    //
    // El fruto tierno se distingue por el COLOR, no recortando la imagen:
    // recortarla deformaria el dibujo.
    for (VariedadTuna v : { VariedadTuna::VERDE, VariedadTuna::AMARILLA,
                            VariedadTuna::ROJA }) {
        for (bool madura : { false, true }) {
            const std::string a = archivoTuna(v, madura);
            CHECK(a.find("crecida") != std::string::npos);
        }
    }
}

TEST_CASE("Tuna: NINGUN archivo posible queda fuera de la precarga") {
    // ⭐⭐ ESTE ES EL TEST DEL BUG.
    //
    // La precarga sube a la GPU una lista fija desde el hilo principal. El
    // mesher, desde un worker, pide el archivo que toque por posicion. Si el
    // mesher puede pedir algo que la precarga no subio, ese fruto sale
    // invisible -- porque en un worker la carga esta prohibida y devuelve 0.
    //
    // Aqui se recorren TODAS las combinaciones que archivoTuna() puede
    // producir y se exige que cada una este en la lista de precarga.
    size_t n = 0;
    const char* const* lista = archivosTunaPrecarga(n);
    REQUIRE(n > 0);

    std::vector<std::string> precargados;
    for (size_t i = 0; i < n; ++i) precargados.push_back(lista[i]);

    for (VariedadTuna v : { VariedadTuna::VERDE, VariedadTuna::AMARILLA,
                            VariedadTuna::ROJA }) {
        for (bool madura : { false, true }) {
            const std::string a = archivoTuna(v, madura);

            bool esta = false;
            for (const std::string& p : precargados)
                if (p == a) { esta = true; break; }

            INFO("archivo que el mesher puede pedir: ", a);
            CHECK(esta);
        }
    }
}

TEST_CASE("Tuna: la precarga no carga imagenes que nadie va a pedir") {
    // La otra direccion. No es un bug visible, pero una lista que acumula
    // archivos muertos deja de ser fiable como documentacion de lo que hace
    // falta -- y es lo que hace confiar en ella para el test de arriba.
    size_t n = 0;
    const char* const* lista = archivosTunaPrecarga(n);

    std::vector<std::string> alcanzables;
    for (VariedadTuna v : { VariedadTuna::VERDE, VariedadTuna::AMARILLA,
                            VariedadTuna::ROJA })
        for (bool madura : { false, true })
            alcanzables.push_back(archivoTuna(v, madura));

    for (size_t i = 0; i < n; ++i) {
        bool usado = false;
        for (const std::string& a : alcanzables)
            if (a == lista[i]) { usado = true; break; }

        INFO("archivo precargado: ", lista[i]);
        CHECK(usado);
    }
}

TEST_CASE("Tuna: la lista no tiene duplicados") {
    // Tres archivos para cuatro casos: el duplicado natural (verde sin madurar
    // y verde madura son la misma imagen) tiene que resolverse en la lista,
    // no cargarse dos veces.
    size_t n = 0;
    const char* const* lista = archivosTunaPrecarga(n);

    for (size_t i = 0; i < n; ++i)
        for (size_t j = i + 1; j < n; ++j)
            CHECK(std::string(lista[i]) != std::string(lista[j]));
}

TEST_CASE("Tuna: una variedad desconocida no devuelve nullptr") {
    // Defensa: el mesher hace snprintf con lo que salga de aqui. Un nullptr
    // seria comportamiento indefinido, y ademas el fruto volveria a
    // desaparecer -- justo el fallo que se esta cerrando.
    const char* a = archivoTuna((VariedadTuna)99, true);
    REQUIRE(a != nullptr);
    CHECK(std::strlen(a) > 0);

    // Y cae en la verde, que es el caso por defecto de la planta.
    CHECK(std::string(a) == "Tuna verde crecida.png");
}
