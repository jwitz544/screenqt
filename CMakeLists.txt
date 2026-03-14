cmake_minimum_required(VERSION 3.16)

project(screenqt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC requires these flags for proper C++17 support with Qt
if(MSVC)
    add_compile_options(/Zc:__cplusplus /permissive-)
endif()

# Qt path: configure via environment variable or -DCMAKE_PREFIX_PATH on the cmake command line.
# e.g.:  cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/msvc2022_64 ..
#   or:  set Qt6_DIR=C:/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6
#   or:  set QTDIR=C:/Qt/6.10.1/msvc2022_64
if(DEFINED ENV{Qt6_DIR})
    list(APPEND CMAKE_PREFIX_PATH $ENV{Qt6_DIR})
elseif(DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH $ENV{QTDIR})
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Suppress optional Vulkan header discovery to avoid noisy warnings
set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan TRUE)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

find_package(Qt6 COMPONENTS Core Gui Widgets PrintSupport REQUIRED)
find_package(Qt6 COMPONENTS Pdf QUIET)

option(SCREENQT_ENABLE_DEPLOY "Run windeployqt for app target" OFF)
option(SCREENQT_ENABLE_TEST_DEPLOY "Run windeployqt for test targets" OFF)

add_library(screenqt_core STATIC
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
    src/findbar.cpp
    include/findbar.h
    src/spellcheckservice.cpp
    include/spellcheckservice.h
    src/mainwindow.cpp
    include/mainwindow.h
    src/titlepage_dialog.cpp
    include/titlepage_dialog.h
    src/fountainio.cpp
    include/fountainio.h
)

if(WIN32)
    target_sources(screenqt_core PRIVATE
        src/windowsspellchecker.cpp
        include/windowsspellchecker.h
    )
    target_link_libraries(screenqt_core PUBLIC ole32)
endif()

target_include_directories(screenqt_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(screenqt_core PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PrintSupport
)

add_executable(screenqt
    src/main.cpp
)

target_link_libraries(screenqt PRIVATE screenqt_core)

# Windows-specific settings
if(WIN32 AND SCREENQT_ENABLE_DEPLOY)
    # Comment out WIN32_EXECUTABLE to show console for debugging
    # set_target_properties(screenqt PROPERTIES WIN32_EXECUTABLE ON)

    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
        HINTS "${Qt6_DIR}/../../../bin")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET screenqt POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:screenqt>"
            COMMENT "Running windeployqt..."
        )
    endif()
endif()

# --- Tests ---
include(CTest)
enable_testing()
find_package(Qt6 COMPONENTS Test REQUIRED)

add_executable(pageview_tests
    tests/pageview_enforcebreaks_test.cpp
)

target_link_libraries(pageview_tests
    screenqt_core
    Qt6::Test
)

add_test(NAME pageview_enforcebreaks COMMAND pageview_tests -o pageview_enforcebreaks.txt,txt)

add_executable(scripteditor_undo_tests
    tests/scripteditor_undo_test.cpp
)

target_link_libraries(scripteditor_undo_tests
    screenqt_core
    Qt6::Test
)

add_test(NAME scripteditor_undo COMMAND scripteditor_undo_tests -o scripteditor_undo.txt,txt)

add_executable(scripteditor_format_tests
    tests/scripteditor_format_test.cpp
)

target_link_libraries(scripteditor_format_tests
    screenqt_core
    Qt6::Test
)

add_test(NAME scripteditor_format COMMAND scripteditor_format_tests -o scripteditor_format.txt,txt)

add_executable(scripteditor_find_spellcheck_tests
    tests/scripteditor_find_spellcheck_test.cpp
)

target_link_libraries(scripteditor_find_spellcheck_tests
    screenqt_core
    Qt6::Test
)

add_test(NAME scripteditor_find_spellcheck COMMAND scripteditor_find_spellcheck_tests -o scripteditor_find_spellcheck.txt,txt)

add_executable(document_settings_tests
    tests/document_settings_test.cpp
)

target_link_libraries(document_settings_tests
    screenqt_core
    Qt6::Test
)

add_test(NAME document_settings COMMAND document_settings_tests -o document_settings.txt,txt)

add_executable(pdf_export_tests
    tests/pdf_export_test.cpp
)

target_link_libraries(pdf_export_tests
    screenqt_core
    Qt6::Test
)

if(TARGET Qt6::Pdf)
    target_link_libraries(pdf_export_tests Qt6::Pdf)
    target_compile_definitions(pdf_export_tests PRIVATE SCREENQT_HAS_QTPDF=1)
endif()

add_test(NAME pdf_export COMMAND pdf_export_tests -o pdf_export.txt,txt)

if(WIN32 AND SCREENQT_ENABLE_TEST_DEPLOY)
    if(NOT WINDEPLOYQT_EXECUTABLE)
        find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt windeployqt6
            HINTS "${Qt6_DIR}/../../../bin")
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        foreach(_test pageview_tests scripteditor_undo_tests scripteditor_format_tests
                      scripteditor_find_spellcheck_tests document_settings_tests pdf_export_tests)
            add_custom_command(TARGET ${_test} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" "$<TARGET_FILE:${_test}>"
                COMMENT "Running windeployqt for ${_test}..."
            )
        endforeach()
    endif()
endif()
