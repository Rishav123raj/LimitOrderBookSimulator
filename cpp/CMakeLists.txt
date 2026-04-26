cmake_minimum_required(VERSION 3.10)

project(OrderBookSimulator VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(OrderBookSimulatorLib 
    src/OrderBook.cpp
    src/MatchingEngine.cpp
)

target_include_directories(OrderBookSimulatorLib PUBLIC include)

target_compile_options(OrderBookSimulatorLib PRIVATE -W)

add_executable(OrderBookSimulator src/main.cpp)
target_link_libraries(OrderBookSimulator PRIVATE OrderBookSimulatorLib)
