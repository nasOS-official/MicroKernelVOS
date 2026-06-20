#include <ctype.h>

int isdigit(int c){
    if (c >= '0' && c <= '9'){
        return 1;
    }
    return 0;
}

int toupper(int c){
    if (c >= 'a' && c <= 'z'){
        c -= 'a' - 'A'; 
    }
    return c;
}