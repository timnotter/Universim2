cmake_minimum_required(VERSION 3.10)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_C_COMPILER clang) 
#set(CMAKE_CXX_COMPILER clang++)
#set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")
project(universim2)

include_directories(${PROJECT_SOURCE_DIR}/universim2/include)

find_package(X11)
find_package (Threads)
#find_package(OpenCL)

add_executable(universim2 
  universim2/src/main.cpp 

  universim2/src/graphicInterface/renderer.cpp 
  universim2/src/graphicInterface/window.cpp 
  universim2/src/graphicInterface/point2d.cpp 
  universim2/src/graphicInterface/drawObject.cpp 
  universim2/src/graphicInterface/stellarObjectRenderFace.cpp 
  universim2/src/graphicInterface/renderTriangle.cpp 

  universim2/src/stellarObjects/stellarObject.cpp 
  universim2/src/stellarObjects/galacticCore.cpp 
  universim2/src/stellarObjects/starSystem.cpp 
  universim2/src/stellarObjects/star.cpp 
  universim2/src/stellarObjects/planet.cpp 
  universim2/src/stellarObjects/moon.cpp 
  universim2/src/stellarObjects/comet.cpp 
  universim2/src/stellarObjects/asteroid.cpp 

  universim2/src/helpers/positionVector.cpp 
  universim2/src/helpers/plane.cpp 
  universim2/src/helpers/matrix3d.cpp 
  universim2/src/helpers/date.cpp 
  universim2/src/helpers/tree.cpp 
  universim2/src/helpers/treeCodeNode.cpp 
  universim2/src/helpers/timer.cpp
  universim2/src/helpers/simplexNoise.cpp)
add_compile_options (-o, -O3, -pthread, -g)

target_link_libraries(universim2 ${X11_LIBRARIES})
target_link_libraries(universim2 ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(universim2 ${OpenCL_INCLUDE_DIRS})
