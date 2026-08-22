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

add_test(NAME unit_tests COMMAND voxel_tests)
