#ifndef __THREADS_H
#define __THREADS_H

#include <stdint.h>
typedef struct thread_t {
    uint64_t pid;
    uint64_t tid;
    uint64_t usp;
    uint64_t ksp;

} thread_t;
//из thread_t берем при переключении потоков sp нужные и кладем в gs, а при syscall из gs
#endif