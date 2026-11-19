# tests/CMakeLists.txt — builds one test program, seam_tests, and registers it with ctest.
#
# The top-level CMakeLists.txt needs these two lines:
#     enable_testing()
#     add_subdirectory(tests)

find_package(OpenCV REQUIRED COMPONENTS core imgproc)

# doctest is a single header. If tests/doctest.h is committed we use it;
# otherwise CMake downloads it (CI has internet access, so this just works).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/doctest.h")
    set(DOCTEST_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(doctest
        URL https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
        DOWNLOAD_NO_EXTRACT TRUE)
    FetchContent_MakeAvailable(doctest)
    set(DOCTEST_INCLUDE_DIR "${doctest_SOURCE_DIR}")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/SeamDetect.cmake")

add_executable(seam_tests
    test_main.cpp
    test_core_units.cpp
    test_algorithms.cpp)

target_include_directories(seam_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${DOCTEST_INCLUDE_DIR}")
target_compile_definitions(seam_tests PRIVATE ${SEAM_DETECT_DEFINITIONS})
target_compile_features(seam_tests PRIVATE cxx_std_17)
target_link_libraries(seam_tests PRIVATE seamcore ${OpenCV_LIBS})

add_test(NAME seam_tests COMMAND seam_tests)

# Javier's own sanity checks (tests/test_core.cpp): a separate program with its own
# main(), so it is built on its own and also run by ctest.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_core.cpp")
    add_executable(test_core test_core.cpp)
    target_compile_features(test_core PRIVATE cxx_std_17)
    target_link_libraries(test_core PRIVATE seamcore ${OpenCV_LIBS})
    add_test(NAME test_core COMMAND test_core)
endif()

# test_greedy belongs to Wai Men
add_executable(test_greedy test_greedy.cpp)
target_link_libraries(test_greedy PRIVATE seamcore)
add_test(NAME test_greedy COMMAND test_greedy)

# Jun Siang's own graph cut checks (tests/test_graphcut.cpp): standalone program with its own main().
add_executable(test_graphcut test_graphcut.cpp)
target_link_libraries(test_graphcut PRIVATE seamcore ${OpenCV_LIBS})
add_test(NAME test_graphcut COMMAND test_graphcut)

# Shahir's DP checks (tests/test_dp.cpp): standalone program with its own main().
add_executable(test_dp test_dp.cpp)
target_link_libraries(test_dp PRIVATE seamcore ${OpenCV_LIBS})
add_test(NAME test_dp COMMAND test_dp)
