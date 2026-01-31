#define UART0_BASE 0x101f1000

#define UART_DR     0x00  // Data Register
#define UART_FR     0x18  // Flag Register
#define UART_FR_TXFF 0x20 // Transmit FIFO Full

volatile unsigned int * const UART0 = (unsigned int *)UART0_BASE;

void uart_putc(char c) {
    // Wait until there is space in the FIFO
    while (UART0[UART_FR / 4] & UART_FR_TXFF);
    UART0[UART_DR / 4] = c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void main() {
    uart_puts("Hello World\n");
    while (1); // Infinite loop to prevent exit
}
