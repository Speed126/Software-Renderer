cmake_minimum_required(VERSION 3.20)
project(SoftwareRenderer VERSION 1.0.0 LANGUAGES CXX)

include(CTest)
set(CMAKE_CXX_EXTENSIONS OFF)
option(RENDERER_STRICT "Treat project warnings as errors" OFF)

function(renderer_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- $<$<BOOL:${RENDERER_STRICT}>:/WX>)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            $<$<BOOL:${RENDERER_STRICT}>:-Werror>)
    endif()
endfunction()

add_library(renderer_core
    src/math.cpp
    src/model.cpp
    src/framebuffer.cpp
    src/rasterizer.cpp
    src/pipeline.cpp
    src/texture.cpp
    src/tga_writer.cpp)
target_include_directories(renderer_core PUBLIC include)
target_compile_features(renderer_core PUBLIC cxx_std_20)
renderer_warnings(renderer_core)

add_executable(renderer src/main.cpp)
target_link_libraries(renderer PRIVATE renderer_core)
renderer_warnings(renderer)
configure_file(assets/orbital.obj assets/orbital.obj COPYONLY)

if(BUILD_TESTING)
    foreach(suite core integration)
        if(suite STREQUAL "core")
            set(test_source tests/core.cpp)
        else()
            set(test_source tests/integration.cpp)
        endif()
        add_executable(${suite}_tests ${test_source})
        target_link_libraries(${suite}_tests PRIVATE renderer_core)
        renderer_warnings(${suite}_tests)
        add_test(NAME ${suite} COMMAND ${suite}_tests)
    endforeach()
    add_test(NAME render_smoke COMMAND renderer assets/orbital.obj --output smoke.tga --width 96 --height 96)
    add_test(NAME cli_help COMMAND renderer --help)
    set_tests_properties(cli_help PROPERTIES PASS_REGULAR_EXPRESSION "renderer")
    foreach(case missing_model invalid_size oversized_size bad_number missing_value unknown duplicate_model bad_camera bad_light missing_texture output_failure)
        add_test(NAME cli_${case} COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:renderer> -DCASE=${case}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli.cmake)
    endforeach()
endif()
