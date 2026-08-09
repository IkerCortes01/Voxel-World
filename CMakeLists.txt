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

# Link libraries
target_link_libraries(VoxelWorld PRIVATE
    glfw
    ${OPENGL_LIBRARIES}
)

# Windows specific
if(WIN32)
    target_compile_definitions(VoxelWorld PRIVATE _CRT_SECURE_NO_WARNINGS)
    # winmm: API waveOut usada por el mezclador de audio (src/audio/AudioMixer.h)
    target_link_libraries(VoxelWorld PRIVATE winmm)
    # Usar subsistema Windows pero mantener main() en lugar de WinMain()
    target_link_options(VoxelWorld PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()
