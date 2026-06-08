cmake_minimum_required(VERSION 3.20)
project(YCode)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()

set(SOURCES
    main.cpp
    MainWindow.cpp
    ChatWidget.cpp
    CodeEditor.cpp
    AgentManager.cpp
    SyntaxHighlighter.cpp
)

set(HEADERS
    MainWindow.h
    ChatWidget.h
    CodeEditor.h
    AgentManager.h
    SyntaxHighlighter.h
)

set(RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/YCode.rc
)

qt_add_executable(YCode ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(YCode PRIVATE Qt6::Core Qt6::Widgets Qt6::Network)

# Windows 应用程序图标
set_target_properties(YCode PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# 复制根目录构建出的 agent.exe 到客户端构建目录
set(AGENT_EXE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../agent.exe")
if(EXISTS "${AGENT_EXE_PATH}")
    add_custom_command(TARGET YCode POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${AGENT_EXE_PATH}"
            $<TARGET_FILE_DIR:YCode>
    )
endif()
