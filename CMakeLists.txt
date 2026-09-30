cmake_minimum_required(VERSION 3.16)

project(WordProcessor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# Adding WIN32 suppresses the command prompt / console window on Windows
add_executable(WordProcessor WIN32
    main.cpp
    icon.ico
)

# Embed icon as Qt resource
qt_add_resources(WordProcessor
    PREFIX "/"
    FILES icon.ico
)

target_link_libraries(WordProcessor PRIVATE Qt6::Widgets)