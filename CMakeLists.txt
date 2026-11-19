# Owned by Person A. Need a new file added? Tell A the file name.
cmake_minimum_required(VERSION 3.16)
project(SeamCarving LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build: seam carving in Debug is painfully slow.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)

# ---- seamcore: everything in core/ and algos/ ------------------------------
add_library(seamcore STATIC
    core/energy.cpp
    core/carver.cpp
    algos/greedy_seam.cpp
    core/energy_mt.cpp
    # algos/ files go here (B: dp_seam.cpp, C: greedy_seam.cpp energy_mt.cpp,
    # D: flow_graph.cpp graphcut_seam.cpp)
    algos/flow_graph.cpp
    algos/graphcut_seam.cpp
)
target_include_directories(seamcore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(seamcore PUBLIC ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(seamcore PRIVATE /W4)
else()
    target_compile_options(seamcore PRIVATE -Wall -Wextra)
endif()

# ---- seamcarve: the app (Person E) -----------------------------------------
# E added app/args.cpp, app/viz.cpp, app/ui.cpp; highgui is needed for the
# interactive window and the animated visualisation.
add_executable(seamcarve
    app/main.cpp
    app/args.cpp
    app/viz.cpp
    app/ui.cpp
)
target_link_libraries(seamcarve PRIVATE seamcore)

# ---- tests (Person F) ------------------------------------------------------
enable_testing()
add_subdirectory(tests)

# ---- benchmark: for threading (wai men) ------------------------------------
add_executable(bench_energy bench/bench_energy.cpp)
target_link_libraries(bench_energy PRIVATE seamcore)
