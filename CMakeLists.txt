cmake_minimum_required(VERSION 3.20)
project(frogcontexto LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(frogcontexto_core
    src/context.cpp
    src/openai_client.cpp
    src/agent_runtime.cpp
)
target_include_directories(frogcontexto_core PUBLIC include)
if(WIN32)
    target_link_libraries(frogcontexto_core PUBLIC winhttp)
endif()

add_executable(frogcontexto src/main.cpp)
target_link_libraries(frogcontexto PRIVATE frogcontexto_core)

add_executable(frogcontexto_tests tests/test_context.cpp)
target_link_libraries(frogcontexto_tests PRIVATE frogcontexto_core)

add_executable(frogcontexto_json_tests tests/test_json.cpp)
target_link_libraries(frogcontexto_json_tests PRIVATE frogcontexto_core)

add_executable(frogcontexto_simulation_tests tests/test_simulation.cpp)
target_link_libraries(frogcontexto_simulation_tests PRIVATE frogcontexto_core)

enable_testing()
add_test(NAME frogcontexto_tests COMMAND frogcontexto_tests)
add_test(NAME frogcontexto_json_tests COMMAND frogcontexto_json_tests)
add_test(NAME frogcontexto_simulation_tests COMMAND frogcontexto_simulation_tests)
