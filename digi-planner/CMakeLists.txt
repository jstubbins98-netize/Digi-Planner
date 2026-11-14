cmake_minimum_required(VERSION 3.24)
project(DigiPlanner VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Digi-Planner targets Windows 11 only. Build on Windows with the MSVC Qt kit.")
endif()
if(NOT MSVC)
    message(FATAL_ERROR "Use Visual Studio 2022 and the matching Qt MSVC 2022 64-bit kit.")
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
install(SCRIPT ${deploy_script})
install(FILES README.md DESTINATION .)
