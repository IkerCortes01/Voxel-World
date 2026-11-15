#pragma once

#include <string>
#include "ui/TextoUtf8.h"
#include <algorithm>
#include <cctype>

// ============================================================================
// VALIDACIÓN DE NOMBRES DE MUNDO
// ============================================================================
// Los nombres de mundo se usan directamente como nombres de carpeta bajo
// saves/, así que un nombre malicioso o simplemente inválido se convierte en
// una ruta inválida (o peor, en un escape del directorio).
//
// Extraído de main.cpp para poder testearlo. Corrige dos defectos del código
// original: std::isalnum(char) es UB para bytes >127 (chars con signo), y no
// se rechazaban los nombres de dispositivo reservados de Windows.

namespace WorldName {

// Caracteres permitidos: ASCII alfanumérico, espacio, guion y guion bajo.
// Deliberadamente restrictivo: bloquea '/', '\\', ':', '..' y no-ASCII.
inline bool isAllowedChar(char c) {
    // Convertir a unsigned antes de pasar a isalnum: con char con signo, un
    // byte >127 se convierte en negativo y el comportamiento es indefinido.
    unsigned char uc = static_cast<unsigned char>(c);
    if (uc > 127) return false;  // solo ASCII: evita problemas de codificación
    return std::isalnum(uc) || c == ' ' || c == '-' || c == '_';
}

// Nombres de dispositivo reservados en Windows: crear "saves/CON" falla de
// formas confusas independientemente de la extensión.
inline bool isReservedName(const std::string& name) {
    static const char* reserved[] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };

    // Comparar contra la parte anterior al primer punto, sin distinguir mayúsculas
    std::string base = name.substr(0, name.find('.'));
    std::transform(base.begin(), base.end(), base.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    for (const char* r : reserved) {
        if (base == r) return true;
    }
    return false;
}

constexpr size_t MAX_LENGTH = 50;

// Motivo de rechazo, para poder dar un mensaje útil al jugador.
enum class Validity {
    Ok,
    Empty,
    TooLong,
    InvalidChars,
    ReservedName,
    TrailingSpace   // Windows lo recorta silenciosamente al crear la carpeta
};

inline Validity validate(const std::string& name) {
    if (name.empty()) return Validity::Empty;
    if (name.size() > MAX_LENGTH) return Validity::TooLong;

    for (char c : name) {
        if (!isAllowedChar(c)) return Validity::InvalidChars;
    }

    // Un nombre que es solo espacios produce una carpeta sin nombre utilizable
    if (name.find_first_not_of(' ') == std::string::npos) return Validity::Empty;

    // (el punto final no se comprueba: '.' ya lo rechaza isAllowedChar)
    if (name.back() == ' ') return Validity::TrailingSpace;

    if (isReservedName(name)) return Validity::ReservedName;

    return Validity::Ok;
}

inline bool isValid(const std::string& name) {
    return validate(name) == Validity::Ok;
}

// El nombre que escribio el jugador, listo para ser carpeta: sin caracteres
// prohibidos, sin espacios al principio ni al final y con el largo maximo. Si
// no queda nada usable (vacio o reservado por Windows), `siVacio`.
inline std::string limpiar(const std::string& escrito, const std::string& siVacio = "Nuevo Mundo") {
    std::string s;
    for (char c : escrito) if (isAllowedChar(c)) s += c;
    const size_t a = s.find_first_not_of(' ');
    if (a == std::string::npos) return siVacio;
    s = s.substr(a, s.find_last_not_of(' ') - a + 1);
    if (s.size() > MAX_LENGTH) s = s.substr(0, MAX_LENGTH);
    while (!s.empty() && s.back() == ' ') s.pop_back();
    return isValid(s) ? s : siVacio;
}

// ============================================================================
// ⭐ EL TITULO DEL MUNDO: LO QUE SE VE, QUE YA NO ES LA CARPETA
// ============================================================================
// PEDIDO: "que en cualquier momento, sin bugs, el usuario pueda modificar el
// nombre de su mundo como quiera y cuando quiera, y que se vea realmente la
// modificacion".
//
// Antes el nombre ERA la carpeta: renombrar movia saves/<viejo> a
// saves/<nuevo>. Eso fallaba (y sin avisar en pantalla) si ya existia otra
// carpeta con ese nombre, si el nombre nuevo era el mismo con otras
// mayusculas, si era un nombre reservado de Windows... y con el mundo abierto
// no se podia ni intentar: sus archivos estan en uso.
//
// Ahora la carpeta se queda como esta para siempre y el nombre que se ve es
// solo el `LevelName` de level.dat. Cambiarlo es escribir una linea: no hay
// nada que mover, asi que se puede hacer cuando sea -- tambien jugando --, y
// vale casi cualquier cosa: repetido, "CON", con puntos y parentesis...
//
// Lo unico que se pide es que se pueda DIBUJAR (la fuente del juego tiene
// letras, numeros y estos signos) y que no quede vacio.
//
// ⭐ DESPUES: TODO EL TECLADO. PEDIDO: "que puedas escribir con minusculas, poner
// acentos, simbolos, signos y poner manual mayusculas y el @, y otros mas". La
// fuente ya dibuja minusculas, acentos, enie y todos los signos (FuenteBits.h),
// asi que el titulo admite cualquier caracter que se pueda escribir
// (Utf8::seEscribe), guardado en UTF-8, y conserva las mayusculas y minusculas
// tal como se teclearon.
inline bool esDeTitulo(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return uc >= 32 && uc <= 126;         // (un byte suelto: solo ASCII; lo demas va en UTF-8)
}
// Lo que escribio el jugador, listo para ser titulo: sin lo que no se puede
// dibujar, sin espacios al principio ni al final y con el largo maximo. Vacio
// si no queda nada.
inline std::string limpiarTitulo(const std::string& escrito) {
    return Utf8::limpiar(escrito, MAX_LENGTH);     // el largo, en caracteres
}

inline const char* describe(Validity v) {
    switch (v) {
        case Validity::Ok:                 return "Nombre valido";
        case Validity::Empty:              return "El nombre no puede estar vacio";
        case Validity::TooLong:            return "El nombre es demasiado largo (max 50 caracteres)";
        case Validity::InvalidChars:       return "Solo se permiten letras, numeros, espacios, guiones y guiones bajos";
        case Validity::ReservedName:       return "Ese nombre esta reservado por Windows";
        case Validity::TrailingSpace:      return "El nombre no puede terminar en espacio";
    }
    return "Nombre invalido";
}

} // namespace WorldName
