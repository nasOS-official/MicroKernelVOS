cmake_minimum_required(VERSION 3.15)
project(MicroKernelVOS CXX)



add_subdirectory(hal)
add_subdirectory(kernel)

add_custom_target(run
    COMMAND qemu-system-x86_64 -cdrom image.iso -enable-kvm -m 512 -d int,guest_errors,cpu_reset
    DEPENDS image.iso
)

add_custom_target(image.iso
    # OUTPUT nasOS.iso
    COMMAND cp kernel/kernel.bin iso/kernel.bin && xorriso -as mkisofs -R -r -J -b limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus -apm-block-size 2048 --efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label iso -o image.iso && limine bios-install image.iso

    DEPENDS kernel.bin
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/iso
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})