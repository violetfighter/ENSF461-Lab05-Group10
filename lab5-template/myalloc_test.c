#include "myalloc.h"
#include <stdio.h>

int main() {
    myinit(1);  // initialize arena with 1 byte
    mydestroy(); // destroy the arena
    return 0;
}

