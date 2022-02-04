cmake_minimum_required(VERSION 3.18.0)

project(flare VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Git)

set(flare_GIT_REVISION "")

if(GIT_FOUND)
    execute_process(
      COMMAND git rev-parse --abbrev-ref HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_BRANCH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Get the latest abbreviated commit hash of the working branch
    execute_process(
      COMMAND git log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COMMIT_HASH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(flare_GIT_REVISION "${GIT_COMMIT_HASH}-${GIT_BRANCH}")
endif(GIT_FOUND)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FlareConfig.in flare_config.h)

find_package(Vulkan 1.2.0 REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(flare)
add_subdirectory(libs/glfw)
add_subdirectory(libs/spdlog)
add_subdirectory(libs/glm)
add_subdirectory(libs/json)
add_subdirectory(libs/shaderc)

