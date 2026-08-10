#pragma once

#include <string>
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
