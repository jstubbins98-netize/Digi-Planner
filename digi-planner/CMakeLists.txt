cmake_minimum_required(VERSION 4.2)
project(DigiPlanner VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Digi-Planner targets Windows 11 only. Build on Windows with the MSVC Qt kit.")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "Use Visual Studio 2026 or newer with an MSVC x64 Qt kit.")
endif()
if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio ([0-9]+) ")
    message(FATAL_ERROR "Use a Visual Studio 2026 or newer generator (for example: Visual Studio 18 2026).")
endif()
if(CMAKE_MATCH_1 LESS 18)
    message(FATAL_ERROR "Visual Studio 2026 (version 18) or newer is required.")
endif()
if(NOT CMAKE_VS_PLATFORM_TOOLSET MATCHES "^v[0-9]+$")
    message(FATAL_ERROR "Use the standard MSVC toolset supplied with Visual Studio.")
endif()
string(SUBSTRING "${CMAKE_VS_PLATFORM_TOOLSET}" 1 -1 DIGIPLANNER_TOOLSET)
if(DIGIPLANNER_TOOLSET LESS 145)
    message(FATAL_ERROR "MSVC v145 or newer is required; older optional toolsets are not supported.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Portable application directory")
include(GNUInstallDirs)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Sql)
qt_standard_project_setup()
qt_add_executable(DigiPlanner WIN32
    src/main.cpp
    src/Event.h
    src/Database.h src/Database.cpp
    src/CalendarView.h src/CalendarView.cpp
    src/MainWindow.h src/MainWindow.cpp
    src/Export.h src/Export.cpp
    src/Tutorial.h src/Tutorial.cpp
)
target_link_libraries(DigiPlanner PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql)
target_compile_options(DigiPlanner PRIVATE /W4 /utf-8 /permissive-)
target_compile_definitions(DigiPlanner PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
set_target_properties(DigiPlanner PROPERTIES
    VS_DPI_AWARE "PerMonitor"
)
install(TARGETS DigiPlanner RUNTIME DESTINATION .)
qt_generate_deploy_app_script(
    TARGET DigiPlanner
    OUTPUT_SCRIPT deploy_script
)
# Set the install-time deployment layout explicitly. Qt otherwise defaults to
# a plugins/ directory, while this portable package expects plugins beside the EXE.
install(CODE "set(QT_DEPLOY_PLUGINS_DIR \".\")")
install(SCRIPT ${deploy_script})
install(FILES README.md DESTINATION .)
