// os.h
#ifndef OS_H
#define OS_H

// UART primitives
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char *s);

// line input (echo + enter)
int uart_getline(char *buffer, int max_length);

#endif

