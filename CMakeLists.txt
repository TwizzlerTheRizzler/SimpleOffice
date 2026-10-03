cmake_minimum_required(VERSION 3.16)

project(SimpleOfficeLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# WIN32 suppresses the command prompt window on Windows launch
add_executable(SimpleOffice WIN32
    main.cpp
)

target_link_libraries(SimpleOffice PRIVATE Qt6::Widgets)