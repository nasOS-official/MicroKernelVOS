all:
	clang -ffreestanding -nostdlib -O0 -Wall -Wextra -Werror -march=x86-64 -fno-pie -no-pie --target=x86_64-pc-none-elf -mno-sse main.c -o test.elf