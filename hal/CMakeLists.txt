cmake_minimum_required(VERSION 3.15)
project(hal C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -mgeneral-regs-only -mno-red-zone -m64 -march=x86-64 --target=x86_64-pc-none-elf -fno-pie -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2 -nostdinc")
set(CMAKE_ASM_FLAGS "-march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie")


add_library(hal
    x86_64/universalMachine/mmu/mmu.c
    x86_64/universalMachine/cpu/cpu.c
    x86_64/universalMachine/cpu/syscall.S
)

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(hal PRIVATE kstdlib)