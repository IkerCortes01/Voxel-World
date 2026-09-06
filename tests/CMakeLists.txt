# ============================================================================
# TESTS UNITARIOS (doctest)
# ============================================================================
# doctest está vendorizado en external/doctest: el build funciona sin red.
# Ejecutar con: ctest --test-dir build -C Release --output-on-failure

add_executable(voxel_tests
    test_main.cpp
    test_save_system.cpp
    test_paletted_storage.cpp
    test_world_name.cpp
    test_rng_compat.cpp
    test_inventory.cpp
    test_hacha.cpp
    test_creativo.cpp
    test_cuevas.cpp
    test_maguey.cpp
    test_minerales.cpp
    test_fisica.cpp
    test_maguey_puntas.cpp
    test_celdas_mixtas.cpp
    test_barro.cpp
    test_celdas_compartidas.cpp
    test_compuesto.cpp
    test_maguey_organico.cpp
    test_derrumbe.cpp
    test_apilado.cpp
    test_texturas_existen.cpp
    test_soporte_pared.cpp
    test_aterrizaje.cpp
    test_biznaga.cpp
    test_agua_niveles.cpp
    test_agua_flujo.cpp
    test_agua_escenario.cpp
    test_agave_azul.cpp
    test_caras_ocultas.cpp
    test_maguey_conexion.cpp
    test_maguey_cosecha.cpp
    test_ocote.cpp
    test_pecari_spawn.cpp
    test_pecari_repoblacion.cpp
    test_pecari_entidad.cpp
    test_pecari_mundo.cpp
    test_pecari_cuerpo.cpp
    test_pecari_malla.cpp
    test_pecari_pulido.cpp
    test_agua_simplificada.cpp
    test_perfil.cpp
    test_borde_vecinos.cpp
    test_items_sueltos.cpp
    test_malla_chunk.cpp
    test_creativo_agua.cpp
    test_creativo_completo.cpp
    test_apilado_niveles.cpp
    test_cuerpo_jugador.cpp
    test_hojas_sueltas.cpp
    test_cristales.cpp
    test_dormir.cpp
    test_luz_costura.cpp
    test_rio_cueva.cpp
)

target_include_directories(voxel_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/external
)

# test_save_system.cpp incluye SaveSystem.cpp entero para alcanzar SimpleLZ4
# (enlace interno), así que NO se enlaza aquí: daría símbolos duplicados.

if(MSVC)
    target_compile_options(voxel_tests PRIVATE /W4)
    target_compile_definitions(voxel_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ⭐ LA RAIZ DEL PROYECTO, PARA LOS TESTS QUE MIRAN ARCHIVOS
#
# test_texturas_existen.cpp comprueba que toda textura que el codigo pide
# existe de verdad en disco. Para eso necesita saber donde esta el arbol, y
# adivinarlo con rutas relativas no funciona: los tests se ejecutan desde
# sitios distintos segun se lancen con ctest, a mano o desde el IDE.
#
# Cuando no lo encontraba, esos tests se "omitian" y pasaban sin comprobar
# nada -- que es peor que fallar, porque da falsa seguridad.
target_compile_definitions(voxel_tests PRIVATE
    RAIZ_PROYECTO="${PROJECT_SOURCE_DIR}")

add_test(NAME unit_tests COMMAND voxel_tests)
