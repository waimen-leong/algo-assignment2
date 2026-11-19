# Owned by Person F. test_core is Person A's sanity check for the core.
add_executable(test_core test_core.cpp)
target_link_libraries(test_core PRIVATE seamcore)
add_test(NAME test_core COMMAND test_core)

# test_greedy belongs to wai men
add_executable(test_greedy test_greedy.cpp)
target_link_libraries(test_greedy PRIVATE seamcore)
add_test(NAME test_greedy COMMAND test_greedy)