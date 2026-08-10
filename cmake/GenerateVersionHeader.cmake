# ============================================================================
# GENERADOR DE Version.h (se ejecuta en CADA build, no solo al configurar)
# ============================================================================
# El "v1.0.0" de la esquina de la pantalla estaba hardcodeado y no cambiaba
# nunca: era imposible saber que binario estabas ejecutando. Este script
# construye la version desde git, de modo que cada commit produce un numero
# distinto sin que nadie tenga que acordarse de subirlo a mano:
#
#     v1.0.0-r<numero de commit>.<hash corto>[+] ("+" = cambios sin commitear)
#
# Se escribe primero a un temporal y se copia solo si cambio, para no tocar el
# timestamp de Version.h en cada build (eso recompilaria main.cpp en vano).
#
# Variables esperadas: SRC_DIR (raiz del repo), OUT_DIR (donde dejar Version.h)

set(GAME_VERSION_BASE "1.0.0")

set(GIT_REV "")
set(GIT_COUNT "")
set(GIT_DIRTY "")

find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${SRC_DIR}" rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_REV
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${SRC_DIR}" rev-list --count HEAD
        OUTPUT_VARIABLE GIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    # ¿Hay cambios sin commitear? Marca el binario como no reproducible.
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C "${SRC_DIR}" status --porcelain
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT "${GIT_STATUS}" STREQUAL "")
        set(GIT_DIRTY "+")
    endif()
endif()

if(GIT_REV STREQUAL "")
    # Sin git (p. ej. compilando desde un zip): version base sin metadatos.
    set(GAME_VERSION_FULL "v${GAME_VERSION_BASE}-dev")
else()
    set(GAME_VERSION_FULL "v${GAME_VERSION_BASE}-r${GIT_COUNT}.${GIT_REV}${GIT_DIRTY}")
endif()

string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M" UTC)

file(WRITE "${OUT_DIR}/Version.h.tmp"
"#pragma once
// Generado por cmake/GenerateVersionHeader.cmake en cada build. NO editar ni
// versionar: la fuente de verdad es git.
#define GAME_VERSION_STRING \"${GAME_VERSION_FULL}\"
#define GAME_BUILD_DATE     \"${BUILD_DATE} UTC\"
")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OUT_DIR}/Version.h.tmp" "${OUT_DIR}/Version.h"
)
file(REMOVE "${OUT_DIR}/Version.h.tmp")
