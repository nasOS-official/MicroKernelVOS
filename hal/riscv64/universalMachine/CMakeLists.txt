cmake_minimum_required(VERSION 3.15)

project(hal C ASM)

# set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -march=rv64imac_zicsr_zifencei -mabi=lp64 -mno-relax --target=riscv64-unknown-none-elf -fno-pie -nostdinc")
# set(CMAKE_ASM_FLAGS "-march=rv64imac_zicsr_zifencei -mabi=lp64 -mno-relax --target=riscv64-unknown-none-elf -fno-pie")


# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../kernel/include)


add_library(hal
    cpu/cpu.c
    cpu/trap.S
    mmu/mmu.c
    debug/debug.c
)

target_link_libraries(hal PRIVATE kstdlib)




target_compile_options(hal PRIVATE
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



