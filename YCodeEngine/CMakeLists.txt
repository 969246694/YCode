cmake_minimum_required(VERSION 3.20)
project(YCodeEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ycode_engine STATIC
    src/engine.cpp
    src/event_bus.cpp
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
)

target_compile_features(ycode_engine PUBLIC cxx_std_17)

if(WIN32)
    target_compile_definitions(ycode_engine PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(ycode_engine PUBLIC user32 gdi32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(ycode_engine PUBLIC dl)
endif()

add_executable(ycode_engine_launcher
    src/main.cpp
)

target_link_libraries(ycode_engine_launcher PRIVATE ycode_engine)
