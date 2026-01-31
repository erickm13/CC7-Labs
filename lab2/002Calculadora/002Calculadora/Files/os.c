// os.c
#include "os.h"

#define UART0_BASE 0x101f1000

#define UART_DR      0x00  // Data Register
#define UART_FR      0x18  // Flag Register
#define UART_FR_TXFF 0x20  // Transmit FIFO Full
#define UART_FR_RXFE 0x10  // Receive FIFO Empty

static volatile unsigned int * const UART0 = (unsigned int *)UART0_BASE;

void uart_putc(char c) {
    while (UART0[UART_FR / 4] & UART_FR_TXFF) { }
    UART0[UART_DR / 4] = (unsigned int)c;
}

char uart_getc(void) {
    while (UART0[UART_FR / 4] & UART_FR_RXFE) { }
    return (char)(UART0[UART_DR / 4] & 0xFF);
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Lee una línea con eco, soporta backspace.
// Retorna longitud (sin contar '\0').
int uart_getline(char *buffer, int max_length) {
    int i = 0;

    while (i < max_length - 1) {
        char c = uart_getc();

        // Enter: CR o LF
        if (c == '\r' || c == '\n') {
            uart_putc('\r');
            uart_putc('\n');
            break;
        }

        // backspace (8) o DEL (127)
        if ((c == 8 || c == 127) && i > 0) {
            i--;
            uart_putc('\b'); uart_putc(' '); uart_putc('\b');
            continue;
        }

        buffer[i++] = c;
        uart_putc(c); // eco
    }

    buffer[i] = '\0';
    return i;
}

