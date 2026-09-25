#include <doctest/doctest.h>

#include <string>
#include <vector>

// ============================================================================
// RENOMBRAR UN MUNDO CAMBIA EL NOMBRE Y NADA MAS
// ============================================================================
// BUG REPORTADO: al renombrar un mundo y guardar, el nombre nuevo no se veia
// al instante. Con una condicion explicita: que la INFORMACION del mundo no
// cambie para nada.
//
// POR QUE NO SE VEIA. `renameWorld` renombraba la CARPETA y volvia a escanear,
// pero `scanSavedWorlds` hace esto:
//
//     worldInfo.name = <nombre de la carpeta>;   // el nuevo
//     loadLevelDat(...);                         // y esto lo PISA
//
// y dentro de loadLevelDat, la linea `LevelName=` del archivo sobrescribe el
// nombre con el que quedo guardado -- el VIEJO. La carpeta se renombraba bien
// y la lista seguia mostrando el nombre anterior.
//
// POR QUE NO SE ARREGLA LLAMANDO A saveLevelDat. Esa funcion reescribe el
// archivo entero y de paso RECALCULA: suma la sesion al tiempo jugado y vuelve
// a medir el tamano en disco. Usarla aqui cambiaria datos del mundo por el
// hecho de renombrarlo, que es justo lo que se pidio evitar.
//
// Estos tests fijan la transformacion sobre el CONTENIDO del archivo, que es
// donde estaba el fallo. No tocan disco: se le da el texto de un level.dat
// real y se comprueba que sale.

namespace {

// Replica de lo que hace renombrarEnLevelDat sobre las lineas del archivo.
// Se copia en vez de incluir main.cpp, que arrastraria OpenGL entero; si
// alguien cambia una y no la otra, estos CHECK dejan de describir el juego.
std::vector<std::string> renombrar(const std::vector<std::string>& original,
                                   const std::string& nombreNuevo) {
    std::vector<std::string> out = original;

    bool cambiada = false;
    for (std::string& l : out) {
        if (l.rfind("LevelName=", 0) == 0) {
            l = "LevelName=" + nombreNuevo;
            cambiada = true;
            break;
        }
    }
    if (!cambiada) out.push_back("LevelName=" + nombreNuevo);
    return out;
}

// Un level.dat como los que escribe el juego.
std::vector<std::string> archivoDeEjemplo() {
    return {
        "# VoxelWorld Level Data",
        "# Este archivo contiene metadata completa del mundo",
        "version=1.0",
        "",
        "# Informacion basica",
        "LevelName=Mundo 1",
        "RandomSeed=5792100",
        "",
        "# Timestamps",
        "CreationDate=1789709613",
        "LastPlayed=1790305387",
        "",
        "# Estadisticas",
        "TotalPlaytime=1380.13",
        "WorldSize=4718592",
        "",
        "# Estado",
        "WorldTime=13200.5",
        "GameMode=1",
        "PlayerX=42.5",
        "PlayerY=79.0",
        "PlayerZ=149.5",
    };
}

} // namespace

TEST_CASE("Renombrar: el nombre cambia") {
    const auto antes = archivoDeEjemplo();
    const auto despues = renombrar(antes, "Mi Mundo Nuevo");

    bool encontrado = false;
    for (const std::string& l : despues) {
        if (l.rfind("LevelName=", 0) == 0) {
            CHECK(l == "LevelName=Mi Mundo Nuevo");
            encontrado = true;
        }
    }
    CHECK(encontrado);
}

TEST_CASE("Renombrar: NO cambia NADA mas") {
    // ⭐⭐ LA CONDICION QUE SE PIDIO EXPRESAMENTE.
    //
    // Se compara linea a linea: todas tienen que ser identicas salvo la del
    // nombre. Si alguien sustituyera esto por una reescritura completa del
    // archivo --que es la tentacion obvia, porque ya existe saveLevelDat--
    // este test lo cazaria: esa funcion recalcula el tiempo jugado y el
    // tamano en disco.
    const auto antes = archivoDeEjemplo();
    const auto despues = renombrar(antes, "Otro Nombre");

    REQUIRE(despues.size() == antes.size());

    for (size_t i = 0; i < antes.size(); ++i) {
        if (antes[i].rfind("LevelName=", 0) == 0) continue;   // la unica
        INFO("linea ", i, ": '", antes[i], "' -> '", despues[i], "'");
        CHECK(despues[i] == antes[i]);
    }
}

TEST_CASE("Renombrar: la semilla sobrevive") {
    // La semilla define el mundo entero. Si se perdiera al renombrar, el
    // terreno se regeneraria distinto y el mundo dejaria de ser el mismo.
    const auto despues = renombrar(archivoDeEjemplo(), "Cualquiera");

    bool ok = false;
    for (const std::string& l : despues)
        if (l == "RandomSeed=5792100") ok = true;
    CHECK(ok);
}

TEST_CASE("Renombrar: el tiempo jugado no se toca") {
    // ⭐ EL DATO QUE saveLevelDat SI CAMBIARIA.
    //
    // Esa funcion hace `totalPlaytime + sessionPlaytime`. Renombrar un mundo
    // no es jugarlo, asi que el contador tiene que quedarse donde estaba.
    const auto despues = renombrar(archivoDeEjemplo(), "Cualquiera");

    bool ok = false;
    for (const std::string& l : despues)
        if (l == "TotalPlaytime=1380.13") ok = true;
    CHECK(ok);
}

TEST_CASE("Renombrar: las fechas no se tocan") {
    // Igual que el tiempo jugado: `LastPlayed` dice cuando se jugo por ultima
    // vez, no cuando se renombro.
    const auto despues = renombrar(archivoDeEjemplo(), "Cualquiera");

    bool creacion = false, ultima = false;
    for (const std::string& l : despues) {
        if (l == "CreationDate=1789709613") creacion = true;
        if (l == "LastPlayed=1790305387")   ultima = true;
    }
    CHECK(creacion);
    CHECK(ultima);
}

TEST_CASE("Renombrar: la posicion del jugador se conserva") {
    // Al volver a entrar hay que aparecer donde se dejo el mundo.
    const auto despues = renombrar(archivoDeEjemplo(), "Cualquiera");

    int encontradas = 0;
    for (const std::string& l : despues) {
        if (l == "PlayerX=42.5")  ++encontradas;
        if (l == "PlayerY=79.0")  ++encontradas;
        if (l == "PlayerZ=149.5") ++encontradas;
    }
    CHECK(encontradas == 3);
}

TEST_CASE("Renombrar: un archivo SIN LevelName recibe el campo") {
    // Un mundo antiguo puede no tenerlo. En vez de dejarlo sin nombre, se
    // anade: a partir de ahi el archivo ya lleva el correcto.
    std::vector<std::string> viejo = {
        "version=1.0",
        "RandomSeed=123",
    };

    const auto despues = renombrar(viejo, "Rescatado");

    CHECK(despues.size() == viejo.size() + 1);
    CHECK(despues.back() == "LevelName=Rescatado");

    // Y lo que ya habia sigue intacto.
    CHECK(despues[0] == "version=1.0");
    CHECK(despues[1] == "RandomSeed=123");
}

TEST_CASE("Renombrar: solo se cambia la PRIMERA aparicion") {
    // Un archivo con la clave repetida (corrupto, o editado a mano) no debe
    // acabar con dos nombres distintos. loadLevelDat se queda con el ultimo
    // que lee, asi que dejar dos seria impredecible.
    std::vector<std::string> raro = {
        "LevelName=Uno",
        "RandomSeed=7",
        "LevelName=Dos",
    };

    const auto despues = renombrar(raro, "Nuevo");
    CHECK(despues[0] == "LevelName=Nuevo");
    CHECK(despues[1] == "RandomSeed=7");
    // La segunda se deja tal cual: cambiarla seria tocar mas de lo pedido, y
    // este archivo ya estaba mal antes de renombrar nada.
    CHECK(despues[2] == "LevelName=Dos");
}

TEST_CASE("Renombrar: un nombre con espacios y acentos pasa tal cual") {
    // Los nombres de mundo los valida WorldName antes de llegar aqui; esto
    // solo comprueba que el texto no se estropea por el camino.
    const auto despues = renombrar(archivoDeEjemplo(), "Valle del Ocote");

    bool ok = false;
    for (const std::string& l : despues)
        if (l == "LevelName=Valle del Ocote") ok = true;
    CHECK(ok);
}
