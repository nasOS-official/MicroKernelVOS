cmake_minimum_required(VERSION 3.15)
project(hal C ASM)


add_library(hal
    x86_64/universalMachine/mmu/mmu.c
)

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)