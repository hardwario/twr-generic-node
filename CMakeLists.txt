cmake_minimum_required(VERSION 3.20.0)

# Setup project name and languages
project(firmware LANGUAGES C ASM)

add_subdirectory(sdk)

# If you need to add some source files to the project add them to the "src" folder and update CMakeLists there
add_subdirectory(src)
