cmake_minimum_required(VERSION 3.20)
project(VoxelWorld VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# RUNTIME DE C++ ENLAZADO ESTÁTICAMENTE
# ============================================================================
# Sin esto, el .exe depende de MSVCP140.dll, VCRUNTIME140.dll y
# VCRUNTIME140_1.dll, que solo están presentes si el usuario tiene instalado
# el "Visual C++ Redistributable". En un PC limpio el juego no arranca: se
# cierra al instante y sin mensaje de error.
#
# Enlazando el runtime dentro del ejecutable, la carpeta del juego funciona
# tal cual en cualquier Windows 10/11 x64, sin instalar nada.
#
# Coste: el .exe crece unos 100-200 KB. A cambio, la distribución es
# autocontenida.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find OpenGL
find_package(OpenGL REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/stb
)

# Source files
set(SOURCES
    src/main.cpp
    src/ChunkSystem.cpp
    src/SaveSystem.cpp
    src/Profiler.cpp
    # src/RenderOptimizations.cpp  # Temporalmente deshabilitado (conflicto con GL headers)
)

# Executable (WIN32 elimina la ventana de consola en Windows)
add_executable(VoxelWorld WIN32 ${SOURCES})

# ============================================================================
# VERSIÓN DESDE GIT (regenerada en cada build)
# ============================================================================
# La versión que se ve en pantalla sale de git (v1.0.0-r<commits>.<hash>), así
# que cada commit produce un número distinto y se sabe exactamente qué binario
# está corriendo. Un custom target la regenera ANTES de compilar en cada build,
# no solo al configurar; copy_if_different evita recompilaciones inútiles.
set(VERSION_HEADER_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${VERSION_HEADER_DIR})
add_custom_target(gen_version
    COMMAND ${CMAKE_COMMAND}
        -DSRC_DIR=${PROJECT_SOURCE_DIR}
        -DOUT_DIR=${VERSION_HEADER_DIR}
        -P ${PROJECT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake
    COMMENT "Generando Version.h desde git"
    VERBATIM
)
add_dependencies(VoxelWorld gen_version)
target_include_directories(VoxelWorld PRIVATE ${VERSION_HEADER_DIR})

# Link libraries
target_link_libraries(VoxelWorld PRIVATE
    glfw
    ${OPENGL_LIBRARIES}
)

# ============================================================================
# TESTS
# ============================================================================
# La logica que protege los datos del jugador (compresion, paletas, nombres de
# mundo) se puede probar sin arrancar OpenGL. doctest va vendorizado en
# external/doctest para que el build siga funcionando sin red.
enable_testing()
add_subdirectory(tests)

# Windows specific
if(WIN32)
    target_compile_definitions(VoxelWorld PRIVATE _CRT_SECURE_NO_WARNINGS)
    # winmm: API waveOut usada por el mezclador de audio (src/audio/AudioMixer.h)
    target_link_libraries(VoxelWorld PRIVATE winmm)
    # Usar subsistema Windows pero mantener main() en lugar de WinMain()
    target_link_options(VoxelWorld PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()
