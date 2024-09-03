# Project Info

cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(
    ModelsLoader
    VERSION 1.0.0.0
    LANGUAGES C CXX
)

# =============================================================

# CMake Settings

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# set(CMAKE_SUPPRESS_REGENERATION true)
set(DCMAKE_GENERATOR_PLATFORM "x64")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS ON)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

add_definitions(-DNOMINMAX) #present window's shit from fucking up us.


# =============================================================
# Dependencies

# Fast gltf
message(STATUS "Installing fastgltf via submodule")
set(BUILD_STATIC_LIBS ON)
add_subdirectory(external/fastgltf)
set_property(TARGET fastgltf PROPERTY FOLDER "Dependencies")
set_property(TARGET fastgltf_simdjson PROPERTY FOLDER "Dependencies")


# 
message(STATUS "Installing spdlog via submodule")
set(BUILD_STATIC_LIBS ON)
add_subdirectory(external/spdlog)
set_property(TARGET spdlog PROPERTY FOLDER "Dependencies")


# GLM
message(STATUS "Installing glm via submodule")
set(BUILD_STATIC_LIBS ON)
add_subdirectory(external/glm/glm)
set_property(TARGET glm_static PROPERTY FOLDER "Dependencies")


# Stb image
message(STATUS "Installing stb image as header only lib")
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE external/stb_image)

# =============================================================
# Sources

file(GLOB_RECURSE FILE_SOURCES RELATIVE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include/*.h
)

# Solution Filters
foreach(source IN LISTS FILE_SOURCES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    string(REPLACE "src" "" source_path_final "${source_path_msvc}")
    source_group("${source_path_final}" FILES "${source}")
endforeach()


# =============================================================

add_executable(ModelsLoader ${FILE_SOURCES})
# Finish Dependencies

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    fastgltf
    spdlog
    glm_static
    stb_image
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC src/include
  PUBLIC external/spdlog/include/spdlog
  PUBLIC external/fastgltf
  PUBLIC external/glm
  PUBLIC external/stb_image
)

add_dependencies(
    ${PROJECT_NAME}
    fastgltf
    spdlog
    glm_static
    stb_image
)

# =============================================================

# Finish Settings

# Change output dir to bin
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
# Change working directory 
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/..)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})




