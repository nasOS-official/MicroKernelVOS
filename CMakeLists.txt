cmake_minimum_required(VERSION 3.15)


set(TARGET_ARCH "riscv64" CACHE STRING "Architecture")
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "x86_64" "riscv64")

set(CMAKE_SYSTEM_NAME Generic CACHE STRING "")
set(CMAKE_C_COMPILER clang CACHE STRING "")
set(CMAKE_ASM_COMPILER clang CACHE STRING "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(TARGET_ARCH STREQUAL "riscv64")
    set(CMAKE_SYSTEM_PROCESSOR riscv64 CACHE STRING "")
endif()

project(MicroKernelVOS CXX)

add_subdirectory(kstdlib)
add_subdirectory(hal)
add_subdirectory(kernel)
add_subdirectory(vinit)

set(LIMINE_VERSION "12.6.0")
set(LIMINE_DIR "${CMAKE_BINARY_DIR}/limine-binary")

set(LIMINE_ARCHIVE "${CMAKE_BINARY_DIR}/limine-binary.tar.xz")
if(NOT EXISTS "${LIMINE_ARCHIVE}")
    message(STATUS "Downloading limine...")
    file(DOWNLOAD
        "https://github.com/Limine-Bootloader/Limine/releases/download/v${LIMINE_VERSION}/limine-binary.tar.xz"
        "${LIMINE_ARCHIVE}"
        TIMEOUT 60
    )
    file(ARCHIVE_EXTRACT
        INPUT "${LIMINE_ARCHIVE}"
        DESTINATION "${LIMINE_DIR}"
    )
endif()

set(LIMINE_DIR "${LIMINE_DIR}/limine-binary/")
set(LIMINE_TOOL "${LIMINE_DIR}/limine")

add_custom_target(compile_limine_tool
    COMMAND make
    WORKING_DIRECTORY "${LIMINE_DIR}"
    BYPRODUCTS "${LIMINE_TOOL}"
    COMMENT "Compiling limine tool..."
)






if(TARGET_ARCH STREQUAL "x86_64")
    add_custom_target(run
        COMMAND qemu-system-x86_64 -cdrom image.iso -enable-kvm -m 512 -d int,guest_errors,cpu_reset
        DEPENDS image.iso
    )
elseif(TARGET_ARCH STREQUAL "riscv64")
    add_custom_target(run
        COMMAND qemu-system-riscv64 -machine virt -drive if=pflash,format=raw,unit=0,file=/usr/share/ovmf/riscv64/RISCV_VIRT_CODE.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=/usr/share/ovmf/riscv64/RISCV_VIRT_VARS.fd,readonly=on -cdrom image.iso -serial chardev:charmux -chardev vc,id=charmux
        DEPENDS image.iso
    )
endif()


add_custom_target(image.iso


    COMMAND ${CMAKE_COMMAND} -E copy kernel/kernel.bin ${CMAKE_CURRENT_BINARY_DIR}/iso/
    COMMAND ${CMAKE_COMMAND} -E copy vinit/vinit ${CMAKE_CURRENT_BINARY_DIR}/iso/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/BOOTX64.EFI ${CMAKE_CURRENT_BINARY_DIR}/iso/EFI/BOOT/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/BOOTRISCV64.EFI ${CMAKE_CURRENT_BINARY_DIR}/iso/EFI/BOOT/

    COMMAND xorriso -as mkisofs -R -r -J 
        -b limine-bios-cd.bin 
        -no-emul-boot -boot-load-size 4 -boot-info-table 
        -hfsplus -apm-block-size 2048 --efi-boot limine-uefi-cd.bin 
        -efi-boot-part --efi-boot-image --protective-msdos-label 
        iso -o image.iso 
        
    COMMAND ${LIMINE_TOOL} bios-install image.iso

    DEPENDS compile_limine_tool kernel.bin vinit
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/iso
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})