cmake_minimum_required(VERSION 3.15)
project(kernel C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -mgeneral-regs-only -mno-red-zone -m64 -march=x86-64 --target=x86_64-pc-none-elf -fno-pie -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2 -nostdinc")
set(CMAKE_ASM_FLAGS "-march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie")

include_directories(include)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


add_executable(kernel.bin
    debug/fb.c
    limine_requests/limine_requests.c
    paging/paging.c
    syscall/syscall.c
    panic/panic.c
    bootstrap/elfloader/elfloader.c
    process/process.c
    process/x86_64/scheduler.S
    cpu/x86_64/cpu.c
    main.c
)


set_target_properties(kernel.bin PROPERTIES LINK_FLAGS "-T linker.ld -nostdlib -lgcc -march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie -no-pie ")

target_link_libraries(kernel.bin PRIVATE hal kstdlib)