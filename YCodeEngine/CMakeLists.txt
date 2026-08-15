cmake_minimum_required(VERSION 3.20)
project(YCodeEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件统一按 UTF-8 解释，避免中文注释在 GBK 代码页下被误读。
# PUBLIC 让链接引擎的游戏项目也带上 /utf-8（引擎头文件含中文注释）。
if(MSVC)
    add_compile_options(/utf-8)
endif()

file(GLOB BOX2D_VENDOR_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/box2d/src/*.c"
)

add_library(box2d_vendor STATIC
    ${BOX2D_VENDOR_SOURCES}
)

target_include_directories(box2d_vendor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/box2d/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/box2d/src
)

set_target_properties(box2d_vendor PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS YES
)

add_library(ycode_engine STATIC
    src/engine.cpp
    src/event_bus.cpp
    src/physics2d.cpp
    src/plugin_loader.cpp
    src/resource_manager.cpp
    src/scene.cpp
    src/scene_loader.cpp
    src/scene_saver.cpp
)

if(WIN32)
    target_sources(ycode_engine PRIVATE
        src/canvas2d_win32.cpp
        src/window_win32.cpp
        src/texture_win32.cpp
        src/audio_win32.cpp
    )
else()
    target_sources(ycode_engine PRIVATE
        src/canvas2d_null.cpp
        src/window_null.cpp
        src/texture_null.cpp
        src/audio_null.cpp
    )
endif()

target_include_directories(ycode_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(ycode_engine PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(ycode_engine PUBLIC /utf-8)
endif()

if(WIN32)
    target_compile_definitions(ycode_engine PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(ycode_engine PUBLIC box2d_vendor user32 gdi32 gdiplus winmm)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ycode_engine PUBLIC box2d_vendor dl)
else()
    target_link_libraries(ycode_engine PUBLIC box2d_vendor)
endif()

add_executable(ycode_engine_launcher
    src/main.cpp
)

target_link_libraries(ycode_engine_launcher PRIVATE ycode_engine)

# ---- Tests -------------------------------------------------------------
include(CTest)
if(BUILD_TESTING)
    add_executable(ycode_engine_tests
        tests/engine_tests.cpp
    )
    target_link_libraries(ycode_engine_tests PRIVATE ycode_engine)
    target_compile_features(ycode_engine_tests PRIVATE cxx_std_17)
    add_test(NAME ycode_engine_tests COMMAND ycode_engine_tests)
endif()
