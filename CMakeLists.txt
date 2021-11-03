cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(CppFigureDrawer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build.")
# Set the possible values of build type for cmake-gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")

### Config

find_package(Vulkan REQUIRED)

set(glfw3_DIR $ENV{GLFW3_DIR})
set(glm_DIR $ENV{GLM_DIR})

cmake_path(SET GLFW3_INCLUDE_DIR "${glfw3_DIR}/include")
cmake_path(SET GLFW3_LIBRARY "${glfw3_DIR}/lib")
link_directories(${GLFW3_LIBRARY})

#find_package(glfw3 3.3 REQUIRED)

cmake_path(SET GLM_INCLUDE_DIR "${glm_DIR}/glm")
cmake_path(APPEND GLM_INCLUDE_DIR ${glm_DIR})

cmake_path(SET FIGURE2D_INCLUDE_DIRS "figure2d")
cmake_path(APPEND FIGURE2D_INCLUDE_DIRS "figure2d_type_object")
#cmake_path(APPEND FIGURE2D_INCLUDE_DIRS "figure2d_factory_method")

set(MAIN "main.cpp" CACHE STRING "")

set(SHADERS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/shaders" CACHE STRING "")

function(add_spirv_shader INPUT_FILE OUTPUT_FILE)
  if(WIN32)
    cmake_path(SET GLSL_COMPILER_DIR "$ENV{VULKAN_SDK}/Bin")
  endif()

  add_custom_command(
    OUTPUT ${OUTPUT_FILE}
    COMMAND "${GLSL_COMPILER_DIR}/glslc" ${INPUT_FILE} -o ${OUTPUT_FILE}
    MAIN_DEPENDENCY ${INPUT_FILE}
  )
endfunction()

### Build

file(GLOB VERTEX_SHADER ${SHADERS_SOURCE_DIR}/*.vert)
file(GLOB FRAGMENT_SHADER ${SHADERS_SOURCE_DIR}/*.frag)

add_spirv_shader(${VERTEX_SHADER} ${CMAKE_BINARY_DIR}/shaders/vert.spv)
add_spirv_shader(${FRAGMENT_SHADER} ${CMAKE_BINARY_DIR}/shaders/frag.spv)

add_executable(${PROJECT_NAME} ${MAIN} ${VERTEX_SHADER} ${FRAGMENT_SHADER})

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

include_directories(${GLFW3_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${FIGURE2D_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} glfw3)
