cmake_minimum_required(VERSION 3.16)

project(screenqt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC requires these flags for proper C++17 support with Qt
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# Set Qt installation path
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.1/msvc2022_64")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Suppress optional Vulkan header discovery to avoid noisy warnings
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

find_package(Qt6 COMPONENTS Core Gui Widgets PrintSupport REQUIRED)

add_executable(screenqt
    src/main.cpp
    src/scripteditor.cpp
    src/scripteditor_undo.cpp
    include/scripteditor.h
    include/scripteditor_undo.h
    src/pageview.cpp
    include/pageview.h
    src/screenplayio.cpp
    include/screenplayio.h
    src/pdfexporter.cpp
    include/pdfexporter.h
    src/startscreen.cpp
    include/startscreen.h
    src/elementtypepanel.cpp
    include/elementtypepanel.h
    src/outlinepanel.cpp
    include/outlinepanel.h
    src/characterspanel.cpp
    include/characterspanel.h
)

target_include_directories(screenqt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(screenqt
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

# Windows-specific settings
if(WIN32)
    # Comment out WIN32_EXECUTABLE to show console for debugging
    # set_target_properties(screenqt PROPERTIES WIN32_EXECUTABLE ON)

    # Automatically run windeployqt after building
    add_custom_command(TARGET screenqt POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:screenqt>"
        COMMENT "Running windeployqt..."
    )
endif()

# --- Tests ---
include(CTest)
enable_testing()
find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(pageview_tests
    tests/pageview_enforcebreaks_test.cpp
    src/pageview.cpp
    include/pageview.h
    src/screenplayio.cpp
    include/screenplayio.h
    src/pdfexporter.cpp
    include/pdfexporter.h
    src/scripteditor.cpp
    src/scripteditor_undo.cpp
    include/scripteditor.h
    include/scripteditor_undo.h
)

target_include_directories(pageview_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pageview_tests
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Test
)

add_test(NAME pageview_enforcebreaks COMMAND pageview_tests -o pageview_enforcebreaks.txt,txt)

add_executable(scripteditor_undo_tests
    tests/scripteditor_undo_test.cpp
    src/scripteditor.cpp
    src/scripteditor_undo.cpp
    include/scripteditor.h
    include/scripteditor_undo.h
)

target_include_directories(scripteditor_undo_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(scripteditor_undo_tests
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Test
)

add_test(NAME scripteditor_undo COMMAND scripteditor_undo_tests -o scripteditor_undo.txt,txt)

add_executable(scripteditor_format_tests
    tests/scripteditor_format_test.cpp
    src/scripteditor.cpp
    src/scripteditor_undo.cpp
    include/scripteditor.h
    include/scripteditor_undo.h
)

target_include_directories(scripteditor_format_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(scripteditor_format_tests
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Test
)

add_test(NAME scripteditor_format COMMAND scripteditor_format_tests -o scripteditor_format.txt,txt)

if(WIN32)
    add_custom_command(TARGET pageview_tests POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:pageview_tests>"
        COMMENT "Running windeployqt for tests..."
    )

    add_custom_command(TARGET scripteditor_undo_tests POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:scripteditor_undo_tests>"
        COMMENT "Running windeployqt for scripteditor_undo_tests..."
    )

    add_custom_command(TARGET scripteditor_format_tests POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" "$<TARGET_FILE:scripteditor_format_tests>"
        COMMENT "Running windeployqt for scripteditor_format_tests..."
    )
endif()
