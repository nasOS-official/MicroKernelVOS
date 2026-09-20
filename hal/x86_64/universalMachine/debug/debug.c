#include "../cpu/x86_64_cpu.h"
#include "debug.h"
#include <stdint.h>

void debug_init(){
    outb(0x3F9, 0);
    outb(0x3FB, 0x80);
    //115200
    outb(0x3F8, 0);
    outb(0x3F9, 0);

    //8n1
    outb(0x3FB, 0x03);
}


static void put_char(char ch){
    while (!(inb(0x3FD) & 0x20)){
    }

    outb(0x3F8, ch);
}

void log_string(char* string){
    for (uint64_t i = 0; string[i] != '\0'; i++){
        put_char(string[i]);
    }
}


void logk(char* string){
    log_string("[KERNEL] ");
    log_string(string);
    log_string("\n\r");
}

void logh(char* string){
    log_string("[HAL] ");
    log_string(string);
    log_string("\n\r");
}

