cmake_minimum_required(VERSION 3.15)
project(kstdlib C ASM)


if(TARGET_ARCH STREQUAL "x86_64")
    # set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -mgeneral-regs-only -mno-red-zone -m64 -march=x86-64 --target=x86_64-pc-none-elf -fno-pie -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2 -nostdinc")
    # set(CMAKE_ASM_FLAGS "-march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie")
    add_compile_options(
        --target=x86_64-pc-none-elf
        -march=x86-64
        -m64
        -ffreestanding
        -nostdlib
        -nostdinc
        -O2
        -Wall
        -Wextra
        -Werror
        -mgeneral-regs-only
        -mno-red-zone
        -fno-pie
        -mcmodel=kernel
        -mno-mmx
        -mno-sse
        -mno-sse2
        -mno-avx
        -mno-avx2
    )
elseif(TARGET_ARCH STREQUAL "riscv64")
    # set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -march=rv64imac_zicsr_zifencei -mabi=lp64 -mno-relax --target=riscv64-unknown-none-elf -fno-pie -nostdinc")
    # set(CMAKE_ASM_FLAGS "-march=rv64imac_zicsr_zifencei -mabi=lp64 -mno-relax --target=riscv64-unknown-none-elf -fno-pie")
    add_compile_options(
        --target=riscv64-unknown-none-elf
        -march=rv64imac_zicsr_zifencei
        -mabi=lp64
        -ffreestanding
        -nostdlib
        -nostdinc
        -O2
        -Wall
        -Wextra
        -Werror
        -mno-relax
        -fno-pie
    )
endif()

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