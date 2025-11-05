#include "myalloc.h"
#include <stdio.h>

int main() {
    myinit(1);
    void *p = myalloc(10);
    printf("myalloc() returned pointer: %p\n", p);
    mydestroy();
    return 0;
}

