cmake_minimum_required(VERSION 3.20)
project(MicroKernelVOS C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER clang)
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_C_FLAGS "-ffreestanding -nostdlib -O0 -Wall -Wextra -Werror -mgeneral-regs-only -mno-red-zone -m64 -march=x86-64 --target=x86_64-pc-none-elf -fno-pie -mcmodel=kernel -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2 -nostdinc")
set(CMAKE_ASM_FLAGS "-march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie")

include_directories(include)

add_custom_target(run
    COMMAND qemu-system-x86_64 -cdrom image.iso -enable-kvm -m 512
    DEPENDS image.iso
)

add_custom_target(image.iso
    # OUTPUT nasOS.iso
    COMMAND cp kernel.bin iso/kernel.bin && xorriso -as mkisofs -R -r -J -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label iso -o image.iso && limine bios-install image.iso

    DEPENDS kernel.bin
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/iso
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


add_executable(kernel.bin
    stdlib/stdlib.c
    stdlib/math.c
    stdlib/string.c
    stdlib/snprintf.c
    stdlib/ctype.c
    debug/fb.c
    limine_requests/limine_requests.c
    paging/paging.c
    tasks/task.c
    syscall/x86_64/syscall.c
    syscall/x86_64/syscall.s
    panic/panic.c
    main.c
)


set_target_properties(kernel.bin PROPERTIES LINK_FLAGS "-T linker.ld -nostdlib -lgcc -march=x86-64 -mno-red-zone --target=x86_64-pc-none-elf -fno-pie -no-pie ")
