cmake_minimum_required(VERSION 3.15)
project(hal C ASM)


if(TARGET_ARCH STREQUAL "x86_64")
    add_subdirectory(x86_64/universalMachine)
elseif(TARGET_ARCH STREQUAL "riscv64")
    add_subdirectory(riscv64/universalMachine)
endif()


target_include_directories(hal PUBLIC
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
