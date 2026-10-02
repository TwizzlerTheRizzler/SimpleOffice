cmake_minimum_required(VERSION 3.16)

project(SimpleSheet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# Adding WIN32 suppresses the command prompt / console window on Windows
add_executable(SimpleSheet WIN32
    main.cpp
)

target_link_libraries(SimpleSheet PRIVATE Qt6::Widgets)