cmake_minimum_required(VERSION 3.18)
project(tls-chat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ydb-cpp-sdk REQUIRED)

add_executable(${PROJECT_NAME} SHARED
    src/main.cpp
)

target_compile_options(${PROJECT_NAME}
    PUBLIC "-fPIC"
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE
)

