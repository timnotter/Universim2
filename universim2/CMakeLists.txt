cmake_minimum_required(VERSION 3.10)
project(universim2)

find_package(X11)
find_package (Threads)
find_package(OpenCL)

add_executable(universim2 main.cpp renderer.cpp window.cpp positionVector.cpp matrix3d.cpp drawObject.cpp stellarObject.cpp galacticCore.cpp starSystem.cpp star.cpp planet.cpp moon.cpp comet.cpp date.cpp tree.cpp treeCodeNode.cpp)
add_compile_options (-o, -O3, -pthread, -g)

target_link_libraries(universim2 ${X11_LIBRARIES})
target_link_libraries(universim2 ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(universim2 ${OpenCL_INCLUDE_DIRS})