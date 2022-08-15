cmake_minimum_required(VERSION 3.20.0)

# Setup project name and languages
project(firmware LANGUAGES C ASM)

add_subdirectory(sdk)

if(DEFINED LED_STRIP_COUNT)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC LED_STRIP_COUNT=${LED_STRIP_COUNT})
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC LED_STRIP_COUNT=144)
endif()

if(DEFINED MODULE_POWER)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC MODULE_POWER=${MODULE_POWER})
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC MODULE_POWER=0)
endif()

if(DEFINED LED_STRIP_TYPE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC LED_STRIP_TYPE=${LED_STRIP_TYPE})
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC LED_STRIP_TYPE=4)
endif()

if(DEFINED FIRMWARE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC FIRMWARE=${FIRMWARE})
endif()

target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC TWR_SCHEDULER_MAX_TASKS=64)

# If you need to add some source files to the project add them to the "src" folder and update CMakeLists there
add_subdirectory(src)
