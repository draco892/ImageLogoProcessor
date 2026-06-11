cmake_minimum_required(VERSION 3.20)
project(logo_processor_cpp23 VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ImageLogoProcessor
    src/main.cpp
)

target_include_directories(ImageLogoProcessor PRIVATE include)

if(MSVC)
    target_compile_options(ImageLogoProcessor PRIVATE /W4 /permissive-)
else()
    target_compile_options(ImageLogoProcessor PRIVATE -Wall -Wextra -Wpedantic)
endif()
