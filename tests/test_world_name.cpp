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
