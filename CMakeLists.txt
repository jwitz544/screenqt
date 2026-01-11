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
    include/scripteditor.h
    src/pageview.cpp
    include/pageview.h
    src/startscreen.cpp
    include/startscreen.h
    src/elementtypepanel.cpp
    include/elementtypepanel.h
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
