cmake_minimum_required(VERSION 3.15)
project(vinit C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O0 -Wall -Wextra -Werror -march=x86-64 -fno-pie --target=x86_64-pc-none-elf -mno-sse")

add_link_options("LINKER:-no-pie")

add_executable(vinit
    main.c
)
