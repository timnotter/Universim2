cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(window_test)

cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
find_package(glfw3 3.4 REQUIRED)

add_executable(window_test 
	src/window_test.cpp
	src/glad.c
)
target_compile_options(window_test PRIVATE -O0 -g)

include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(window_test glfw OpenGL::GL dl)
#target_link_libraries(universim2 ${OpenCL_INCLUDE_DIRS})
