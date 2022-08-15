# List any additional sources here
target_sources(${CMAKE_PROJECT_NAME} PUBLIC application.c sensors.c values.c)

# If you added some folder with header files you need to list them here
target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
