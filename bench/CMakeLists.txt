# bench/CMakeLists.txt — two programs, separate from the tests:
#   seam_bench    timing + seam cost  -> results.csv, timing_breakdown.csv
#   seam_figures  seam overlays and side-by-side quality images
#
# Top-level CMakeLists.txt needs:   add_subdirectory(bench)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)
include("${PROJECT_SOURCE_DIR}/tests/SeamDetect.cmake")

foreach(prog seam_bench seam_figures)
    if(prog STREQUAL "seam_bench")
        add_executable(${prog} bench.cpp)
    else()
        add_executable(${prog} figures.cpp)
    endif()
    target_include_directories(${prog} PRIVATE "${PROJECT_SOURCE_DIR}/tests")
    target_compile_definitions(${prog} PRIVATE ${SEAM_DETECT_DEFINITIONS})
    target_compile_features(${prog} PRIVATE cxx_std_17)
    target_link_libraries(${prog} PRIVATE seamcore ${OpenCV_LIBS})
endforeach()
