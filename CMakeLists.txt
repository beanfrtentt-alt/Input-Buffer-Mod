cmake_minimum_required(VERSION 3.25)

# Geode v5 requires C++23.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# The current workflow targets Windows x64. The Geode build action sets
# platform-specific toolchain options; do not set Apple architecture flags here.

project(SpoonClient
    VERSION 2.0.0
    DESCRIPTION "Geometry Dash 2.2081 gameplay toolkit: F6 menu, per-gamemode buffers, Assisted Gameplay"
    LANGUAGES CXX
)

# Recursively collect every .cpp under src/ — matches the official Geode
# example-mod template. Adding a new file is therefore just `git add src/...`,
# no parallel SOURCES list to keep in sync.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS src/*.hpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Find the Geode SDK. The Geode CLI sets GEODE_SDK to the SDK root when
# `geode build` is invoked. The geode-sdk/build-geode-mod GitHub Action
# also sets this environment variable for us.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "Unable to find Geode SDK! Please define the GEODE_SDK environment "
        "variable to point to the Geode SDK root (run `geode sdk install` or "
        "use the geode-sdk/build-geode-mod GitHub Action, which sets it "
        "automatically).")
else()
    message(STATUS "Found Geode SDK at: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Geode's helper that wires up include paths, links Geode + cocos2d, sets up
# mod resources, and packages the .geode file at the end of the build.
setup_geode_mod(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Project-wide warning flags. Use -Wall but disable the noisiest ones that
# Geode's own macros trigger on every hook declaration.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wno-multichar -Wno-unknown-pragmas
        -Wno-unused-variable -Wno-unused-parameter
        -fno-rtti
    )
endif()
