cmake_minimum_required(VERSION 3.11)
project(scheduling_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# include(FetchContent)
# set(FETCHCONTENT_QUIET FALSE)
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

# FetchContent_Declare(
#     raylib
#     GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
#     GIT_TAG "master"
#     GIT_PROGRESS TRUE
# )

# FetchContent_MakeAvailable(raylib)

find_package(raylib REQUIRED)

add_executable(main)
target_sources(main PRIVATE
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/simview.cpp
    ${SOURCE_DIR}/scheduler.cpp
)
target_link_libraries(main PRIVATE raylib)

add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:main>/assets"
    COMMENT "Copying assets directory"
)
