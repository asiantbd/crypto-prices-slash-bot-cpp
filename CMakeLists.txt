# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
include(FetchContent)

# Project name, version and description
project(asiantbd_bot VERSION 0.0.1 DESCRIPTION "AsianTBD Discord Bot")

# Add DPP and nlohmann_json as dependency
# option using EXTERNAL dependency (default)
# or set to OFF to manually build (using auto FetchContent from CMake > v3.11).
option(USE_EXTERNAL_DPP "Use an external DPP library" ON)
option(USE_EXTERNAL_JSON "Use an external JSON library" ON)

if(USE_EXTERNAL_DPP)
  find_package(dpp 10.1.0 REQUIRED)
else()
  FetchContent_Declare(
    dpp
    GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
    GIT_TAG        v10.1.0
  )
  FetchContent_MakeAvailable(dpp)
endif()

if(USE_EXTERNAL_JSON)
  find_package(nlohmann_json 3.2.0 REQUIRED)
else()
  FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
  )
  FetchContent_MakeAvailable(json)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

file(GLOB all_SRCS
    "${PROJECT_SOURCE_DIR}/include/*.h"
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.c"
)

# Create an executable
add_executable(${PROJECT_NAME}
    ${all_SRCS}
)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
    dpp
    curl
    nlohmann_json::nlohmann_json
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

#Set Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Generate compile_commands.json automatically for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
