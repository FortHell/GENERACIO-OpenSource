cmake_minimum_required(VERSION 3.15)
project(KI_ENGINE CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(KI_ENGINE main.cpp)

target_link_libraries(KI_ENGINE PRIVATE
    glm::glm
    glfw
    glad::glad
    OpenXR::headers
    OpenXR::openxr_loader
    X11::X11
    OpenGL::GL
)