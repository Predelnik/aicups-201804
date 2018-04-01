cmake_minimum_required(VERSION 3.5)
project(MyStrategy)

file(GLOB src "*.cpp" "*.h")
add_executable(MyStrategy ${src})
target_include_directories (MyStrategy PRIVATE deps/dummy)
target_compile_definitions (MyStrategy PRIVATE _ITERATOR_DEBUG_LEVEL=0)
target_compile_options (MyStrategy PRIVATE /std:c++latest /permissive-)


