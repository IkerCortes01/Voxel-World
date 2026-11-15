#include <doctest/doctest.h>

#include "WorldName.h"

#include <string>

// ============================================================================
// Validación de nombres de mundo
// ============================================================================
// El nombre se usa tal cual como carpeta bajo saves/, así que esta validación
// es lo único que separa un nombre escrito por el jugador de una ruta arbitraria.

TEST_CASE("WorldName: acepta nombres razonables") {
    CHECK(WorldName::isValid("Mundo 1"));
    CHECK(WorldName::isValid("mi_mundo"));
    CHECK(WorldName::isValid("survival-hardcore"));
    CHECK(WorldName::isValid("Test123"));
    CHECK(WorldName::isValid("a"));
    CHECK(WorldName::isValid(std::string(WorldName::MAX_LENGTH, 'x')));
}

TEST_CASE("WorldName: rechaza vacio y solo espacios") {
    CHECK(WorldName::validate("") == WorldName::Validity::Empty);
    CHECK(WorldName::validate("   ") == WorldName::Validity::Empty);
}

TEST_CASE("WorldName: rechaza nombres demasiado largos") {
    CHECK(WorldName::validate(std::string(WorldName::MAX_LENGTH + 1, 'x')) ==
          WorldName::Validity::TooLong);
}

// El motivo de existir de esta validación: sin ella, el nombre se concatena a
// "saves/" y puede escapar del directorio o apuntar a otra unidad.
TEST_CASE("WorldName: bloquea intentos de path traversal") {
    CHECK_FALSE(WorldName::isValid(".."));
    CHECK_FALSE(WorldName::isValid("../otro"));
    CHECK_FALSE(WorldName::isValid("..\\otro"));
    CHECK_FALSE(WorldName::isValid("../../Windows/System32"));
    CHECK_FALSE(WorldName::isValid("sub/dir"));
    CHECK_FALSE(WorldName::isValid("sub\\dir"));
    CHECK_FALSE(WorldName::isValid("C:otro"));
    CHECK_FALSE(WorldName::isValid("\\\\servidor\\share"));
}

TEST_CASE("WorldName: rechaza caracteres invalidos en rutas de Windows") {
    for (const char* bad : {"a<b", "a>b", "a:b", "a\"b", "a|b", "a?b", "a*b", "a\tb", "a\nb"}) {
        CAPTURE(bad);
        CHECK(WorldName::validate(bad) == WorldName::Validity::InvalidChars);
    }
}

// Regresión: el código original llamaba std::isalnum(char) directamente, que es
// UB para bytes >127 con char con signo (acentos, emoji…).
TEST_CASE("WorldName: rechaza bytes no ASCII sin comportamiento indefinido") {
    CHECK(WorldName::validate("Mundo Español") == WorldName::Validity::InvalidChars);
    CHECK(WorldName::validate("\xC3\xB1") == WorldName::Validity::InvalidChars);
    CHECK(WorldName::validate("\xFF\xFE") == WorldName::Validity::InvalidChars);

    // isAllowedChar debe ser seguro para los 256 valores posibles de byte
    for (int i = 0; i < 256; i++) {
        char c = static_cast<char>(i);
        bool allowed = WorldName::isAllowedChar(c);
        if (i > 127) CHECK_FALSE(allowed);
    }
}

TEST_CASE("WorldName: rechaza nombres de dispositivo reservados de Windows") {
    CHECK(WorldName::validate("CON") == WorldName::Validity::ReservedName);
    CHECK(WorldName::validate("con") == WorldName::Validity::ReservedName);
    CHECK(WorldName::validate("NUL") == WorldName::Validity::ReservedName);
    CHECK(WorldName::validate("COM1") == WorldName::Validity::ReservedName);
    CHECK(WorldName::validate("LPT9") == WorldName::Validity::ReservedName);

    // Un nombre que solo empieza igual sí es válido
    CHECK(WorldName::isValid("CONtinente"));
    CHECK(WorldName::isValid("COM10"));
}

TEST_CASE("WorldName: rechaza espacio final") {
    // Windows lo recorta al crear la carpeta: el nombre guardado no coincidiría
    CHECK(WorldName::validate("Mundo ") == WorldName::Validity::TrailingSpace);
    CHECK(WorldName::isValid(" Mundo"));   // al principio sí se conserva
}

TEST_CASE("WorldName: el punto se rechaza en cualquier posicion") {
    // No está entre los caracteres permitidos, así que ni siquiera llega a la
    // comprobación de sufijo
    CHECK(WorldName::validate("Mundo.") == WorldName::Validity::InvalidChars);
    CHECK(WorldName::validate("mi.mundo") == WorldName::Validity::InvalidChars);
}

TEST_CASE("WorldName: describe da un mensaje para cada motivo") {
    for (auto v : {WorldName::Validity::Ok,
                   WorldName::Validity::Empty,
                   WorldName::Validity::TooLong,
                   WorldName::Validity::InvalidChars,
                   WorldName::Validity::ReservedName,
                   WorldName::Validity::TrailingSpace}) {
        CHECK(std::string(WorldName::describe(v)).length() > 0);
    }
}

TEST_CASE("WorldName: limpiar deja el nombre escrito listo para ser carpeta") {
    // BUG: el mundo salia siempre como "Mundo N" en vez del nombre escrito.
    CHECK(WorldName::limpiar("Mi Casa") == "Mi Casa");
    CHECK(WorldName::limpiar("  Mi Casa  ") == "Mi Casa");
    CHECK(WorldName::limpiar("a/b:c") == "abc");
    CHECK(WorldName::limpiar("") == "Nuevo Mundo");
    CHECK(WorldName::limpiar("   ") == "Nuevo Mundo");
    CHECK(WorldName::limpiar("CON") == "Nuevo Mundo");
    CHECK(WorldName::limpiar(std::string(80, 'x')).size() == WorldName::MAX_LENGTH);
    CHECK(WorldName::isValid(WorldName::limpiar("hola   ")));
}

// ⭐ EL TITULO: lo que se ve, que ya no es la carpeta. PEDIDO: "que en
// cualquier momento, sin bugs, el usuario pueda modificar el nombre de su
// mundo como quiera y cuando quiera".
TEST_CASE("WorldName: como titulo vale lo que antes no valia como carpeta") {
    using namespace WorldName;
    // Reservados de Windows, puntos, parentesis, barras: ya no son una ruta.
    CHECK(limpiarTitulo("CON") == "CON");
    CHECK(limpiarTitulo("Mi mundo (v2.0)") == "Mi mundo (v2.0)");
    CHECK(limpiarTitulo("Casa/Granja + 50%") == "Casa/Granja + 50%");
    CHECK(limpiarTitulo("El rancho de 'Iker', 2026") == "El rancho de 'Iker', 2026");
    CHECK(limpiarTitulo("a") == "a");
    CHECK_FALSE(isValid("CON"));            // como carpeta sigue sin valer
    CHECK_FALSE(isValid("Mi mundo (v2.0)"));
}

TEST_CASE("WorldName: el titulo se limpia y nunca queda vacio por descuido") {
    using namespace WorldName;
    CHECK(limpiarTitulo("   Mundo 1   ") == "Mundo 1");
    CHECK(limpiarTitulo("") == "");
    CHECK(limpiarTitulo("     ") == "");
    // (Despues entro todo el teclado: la enie y los signos ya se dibujan y se
    // quedan. Lo que se quita son los caracteres de control.)
    CHECK(limpiarTitulo("Ni\xC3\xB1o*<>|?\"\\:") == "Ni\xC3\xB1o*<>|?\"\\:");
    CHECK(limpiarTitulo("a\x01\x7f" "b\n") == "ab");
    // Un texto viejo en Latin-1 se lee y queda en UTF-8.
    CHECK(limpiarTitulo("\xF1\xE9") == "\xC3\xB1\xC3\xA9");
    // El largo maximo, y sin espacio colgando al cortar.
    const std::string largo = std::string(49, 'a') + " bcd";
    CHECK(limpiarTitulo(largo) == std::string(49, 'a'));
    CHECK(limpiarTitulo(std::string(200, 'z')).size() == MAX_LENGTH);
    // Limpiar dos veces da lo mismo.
    for (const char* t : { "  hola  mundo ", "CON", "x.y.z", "(a)+(b)" })
        CHECK(limpiarTitulo(limpiarTitulo(t)) == limpiarTitulo(t));
    for (int c = 0; c < 256; ++c) (void)esDeTitulo((char)c);     // no revienta con ningun byte
    CHECK(esDeTitulo(' '));
    CHECK_FALSE(esDeTitulo('\n'));
    CHECK(esDeTitulo('\\'));              // el titulo ya no es una ruta: vale
    CHECK_FALSE(esDeTitulo('\x7f'));
}
