# Universim2
This is an unfinished version of a galaxy simulator, written in C++ on Linux. 
The renderer is written from scratch, only using the X11 library to display everything on screen

The exe is under construction but not ready yet

Controls:
"escape": Close program. 
"space": Pause/Unpause simulation. 
"page up": Increase simulation speed. 
"page down": Decrese simulation speed. 

"1": Centre parent of current centre. 
"2": Centre first trabant of current centre. 
"3": Centre previous trabant of current centres parent (f.e. jump from Venus to Mercury). 
"4": Centre next trabant of current centres parent. 
"5": Centre previous starsystem in total ordering. 
"6": Centre next starsystem in total ordering. 
"f": Set centre to Null (Enter "free roam" mode) / Centre closest stellar object. 

"q"/"w"/"e"/"a"/"s"/"d": Alter direction of camera. While centring an object, camera moves around said object. 
"r": Reset camera. 
"i"/"j"/"k"/"l": Move camera left/right/up/down. 
"o"/"p": Move camera forwards/backwards. While centring an object, move amount in-/decreases by a constant factor. While in free roam mode, move amount is constant. 
"ö"/"ä": Increase/Decrease move amount (only used in free roam mode)

Used libraries/Packages: X11, Threads, (OpenCl)

Here is the code for a CMakeLists file to compile to whole thing:

    cmake_minimum_required(VERSION 3.10)
    project(universim2)

    find_package(X11)
    find_package (Threads)
    find_package(OpenCL)

    add_executable(universim2 main.cpp renderer.cpp window.cpp positionVector.cpp point2d.cpp plane.cpp matrix3d.cpp 
    drawObject.cpp stellarObjectRenderFace.cpp renderTriangle.cpp 
    stellarObject.cpp galacticCore.cpp starSystem.cpp star.cpp planet.cpp moon.cpp comet.cpp 
    date.cpp tree.cpp treeCodeNode.cpp simplexNoise.cpp)
    add_compile_options (-o, -O3, -pthread, -g)

    target_link_libraries(universim2 ${X11_LIBRARIES})
    target_link_libraries(universim2 ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(universim2 ${OpenCL_INCLUDE_DIRS})
