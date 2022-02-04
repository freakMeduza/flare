cmake_minimum_required(VERSION 3.18.0)

file(GLOB HDRS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libs/spdlog/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/stb
                    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glm
                    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/json
                    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/shaderc/libshaderc/include)

add_executable(flare ${HDRS} ${SRCS})

target_link_libraries(flare PRIVATE Vulkan::Vulkan shaderc spdlog glfw glm::glm nlohmann_json)

file(GLOB_RECURSE GLSL ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
                       ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag)

add_custom_command(TARGET ${PROJECT_NAME} 
                   PRE_BUILD 
                   COMMAND ${CMAKE_COMMAND} -E copy_directory 
                   ${CMAKE_CURRENT_SOURCE_DIR}/icons/
                   ${CMAKE_CURRENT_BINARY_DIR}/icons/
                   DEPENDS ${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} 
                   PRE_BUILD 
                   COMMAND ${CMAKE_COMMAND} -E make_directory 
                   ${CMAKE_CURRENT_BINARY_DIR}/shaders/
                   DEPENDS ${PROJECT_NAME})

foreach(FILE ${GLSL})
    get_filename_component(FILE_NAME ${FILE} NAME)
    set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND Vulkan::glslc -c ${FILE} -o ${OUTFILE})
endforeach(FILE)