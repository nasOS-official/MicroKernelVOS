#include "debug.h"
#include "../sbi.h"

void debug_init(){

}

void put_char(char c){
    sbi_ecall(
        SBI_CONSOLE_PUTCHAR, 
        0, 
        (unsigned long)c, 
        0, 0, 0, 0, 0
    );
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

