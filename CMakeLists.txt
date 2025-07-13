cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(cpp_obj-preview VERSION 1.0 LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG release
)

FetchContent_MakeAvailable(tinyobjloader)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG bf71a834948186f4097caa076cd2663c69a10e1e
)

FetchContent_MakeAvailable(glm)

add_executable(cpp_obj-preview src/processing.cpp src/main.cpp)

find_package(OpenGL REQUIRED)

add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

target_include_directories(cpp_obj-preview PRIVATE
    include
    external/glad/include
    external/glfw/include
)

target_link_libraries(cpp_obj-preview PRIVATE tinyobjloader glm::glm OpenGL::GL dl glad)

target_link_libraries(cpp_obj-preview PRIVATE
    "${CMAKE_SOURCE_DIR}/external/glfw/lib/libglfw3.a"
)


