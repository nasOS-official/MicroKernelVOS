cmake_minimum_required(VERSION 3.15)
project(vinit C ASM)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)


if(TARGET_ARCH STREQUAL "x86_64")
    set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O0 -Wall -Wextra -Werror -march=x86-64 -fno-pie --target=x86_64-pc-none-elf -mno-sse")
elseif(TARGET_ARCH STREQUAL "riscv64")
    set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O0 -Wall -Wextra -Werror -mno-relax --target=riscv64-unknown-none-elf -march=rv64imac_zicsr_zifencei -mabi=lp64 -fno-pie  -nostdinc")
endif()


add_link_options("LINKER:-no-pie")

add_executable(vinit
    main.c
)


if(TARGET_ARCH STREQUAL "riscv64")
    target_link_options(vinit PRIVATE
        -fuse-ld=lld
        -nostdlib
        -mno-relax
        -no-pie
        -march=rv64imac_zicsr_zifencei
        -mabi=lp64
        --target=riscv64-unknown-none-elf
        -Wno-unused-command-line-argument
        -Wl,-m,elf64lriscv
        -Wl,--no-relax
        -Wl,--no-relax-gp
    )

endif()

target_link_libraries(vinit PRIVATE kstdlib)