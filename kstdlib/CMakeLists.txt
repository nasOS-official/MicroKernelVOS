cmake_minimum_required(VERSION 3.15)
project(kstdlib C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -mgeneral-regs-only -mno-red-zone -m64 -march=x86-64 --target=x86_64-pc-none-elf -fno-pie -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2 -nostdinc")
set(CMAKE_ASM_FLAGS "-march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie")


add_library(kstdlib
    ctype.c
    math.c
    snprintf.c
    stdlib.c
    string.c
)

target_include_directories(kstdlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)