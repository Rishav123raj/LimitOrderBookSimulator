# Specifies the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Defines the project name and language
project(OrderBookSimulator VERSION 1.0 LANGUAGES CXX)

# Sets the C++ standard to C++17 (optional, adjust as needed)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adds an executable target named "OrderBookSimulator" from the source file "hello.cpp"
add_executable(OrderBookSimulator hello.cpp)
