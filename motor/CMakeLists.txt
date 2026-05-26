cmake_minimum_required(VERSION 3.16)
project(MancalaMotor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED)

# Fuentes comunes del motor
set(MOTOR_SOURCES
    src/board.cpp
    src/evaluator.cpp
    src/minimax.cpp
    src/engine.cpp
    src/parallel.cpp
)

# Ejecutable principal (servidor HTTP)
add_executable(mancala_server
    src/main.cpp
    src/server.cpp
    ${MOTOR_SOURCES}
)
target_include_directories(mancala_server PRIVATE src)
target_link_libraries(mancala_server OpenMP::OpenMP_CXX)

# Benchmark standalone
add_executable(mancala_bench
    src/bench.cpp
    ${MOTOR_SOURCES}
)
target_include_directories(mancala_bench PRIVATE src)
target_link_libraries(mancala_bench OpenMP::OpenMP_CXX)

# Benchmark alternativo
add_executable(benchmark
    bench/benchmark.cpp
    ${MOTOR_SOURCES}
)
target_include_directories(benchmark PRIVATE src)
target_link_libraries(benchmark OpenMP::OpenMP_CXX)

# Tests
add_executable(test_board
    tests/test_board.cpp
    ${MOTOR_SOURCES}
)
target_include_directories(test_board PRIVATE src)
target_link_libraries(test_board OpenMP::OpenMP_CXX)

add_executable(test_minimax
    tests/test_minimax.cpp
    ${MOTOR_SOURCES}
)
target_include_directories(test_minimax PRIVATE src)
target_link_libraries(test_minimax OpenMP::OpenMP_CXX)
