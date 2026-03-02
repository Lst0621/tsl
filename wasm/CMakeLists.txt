cmake_minimum_required(VERSION 3.15)
project(wasm_sample)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXECUTABLE_SUFFIX ".js")

add_executable(wasm_sample
        src/main.cpp
        src/comb.cpp
        src/comb.h
        src/helper.h
)


target_link_options(wasm_sample PRIVATE
        -sWASM=1
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sENVIRONMENT=web
        -O1
)

target_link_options(wasm_sample PRIVATE
    -sEXPORTED_FUNCTIONS=_wasm_number_of_sequences,_wasm_number_of_sequences_all
)