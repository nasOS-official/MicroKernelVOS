cmake_minimum_required(VERSION 3.15)
project(hal C ASM)

add_library(hal
    cpu/cpu.c
    cpu/trap.S
    mmu/mmu.c
    debug/debug.c
)

set(HAL_COMPILE_OPTIONS
    -ffreestanding
    -nostdlib
    -O2
    -Wall
    -Wextra
    -Werror
    -march=rv64imac_zicsr_zifencei
    -mabi=lp64
    -mno-relax
    --target=riscv64-unknown-none-elf
    -fno-pie
    -nostdinc
)

target_compile_options(hal PRIVATE ${HAL_COMPILE_OPTIONS})
target_link_libraries(hal PRIVATE kstdlib)

set(OFFSETS_C_SRC "${CMAKE_CURRENT_SOURCE_DIR}/asm_offsets.c")
set(OFFSETS_H_GEN "${CMAKE_CURRENT_BINARY_DIR}/asm_offsets.h")

add_custom_command(
    OUTPUT "${OFFSETS_H_GEN}"
    COMMAND ${CMAKE_C_COMPILER} 
            ${HAL_COMPILE_OPTIONS}
            "$<$<BOOL:$<TARGET_PROPERTY:hal,INCLUDE_DIRECTORIES>>:-I$<JOIN:$<TARGET_PROPERTY:hal,INCLUDE_DIRECTORIES>,;-I>>"
            -S "${OFFSETS_C_SRC}" -o "${OFFSETS_H_GEN}"
    DEPENDS "${OFFSETS_C_SRC}"
    COMMAND_EXPAND_LISTS
    COMMENT "Generating ASM offsets: asm_offsets.h"
)

add_custom_target(generate_asm_offsets DEPENDS "${OFFSETS_H_GEN}")
add_dependencies(hal generate_asm_offsets)

target_include_directories(hal PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
