cmake_minimum_required(VERSION 3.20)
project(YCodeEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
)

if(WIN32)
    target_sources(ycode_engine PRIVATE
        src/canvas2d_win32.cpp
        src/window_win32.cpp
    )
else()
    target_sources(ycode_engine PRIVATE
        src/canvas2d_null.cpp
        src/window_null.cpp
    )
endif()

target_include_directories(ycode_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_compile_features(ycode_engine PUBLIC cxx_std_17)

if(WIN32)
    target_compile_definitions(ycode_engine PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(ycode_engine PUBLIC box2d_vendor user32 gdi32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ycode_engine PUBLIC box2d_vendor dl)
else()
    target_link_libraries(ycode_engine PUBLIC box2d_vendor)
endif()

add_executable(ycode_engine_launcher
    src/main.cpp
)

target_link_libraries(ycode_engine_launcher PRIVATE ycode_engine)
