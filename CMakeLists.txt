cmake_minimum_required(VERSION 2.8.9)
project(genetic_seq_gen)

set(Sources Source.cpp)

add_executable(Generator ${Sources})

set_property(TARGET Generator PROPERTY CXX_STANDARD 14)
