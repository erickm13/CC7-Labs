// main.c
#include "stdio.h"

void main(void) {
    int a, b;

    PRINT("Program: Add Two Numbers\n");

    while (1) {
        PRINT("Enter first number: ");
        if (READ("%d", &a) != 1) {
            PRINT("Invalid input.\n");
            continue;
        }

        PRINT("Enter second number: ");
        if (READ("%d", &b) != 1) {
            PRINT("Invalid input.\n");
            continue;
        }

        PRINT("Sum: %d\n", a + b);
    }
}

