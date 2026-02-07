#ifndef OS_H
#define OS_H

// define UART metods

void uart_putc(char c);
char uart_getc();
void uart_puts(const char *s);
void uart_gets_input(char *buffer, int max_length);
int uart_atoi(const char *s);
void uart_itoa(int num, char *buffer);
float uart_atof(const char *s);
void uart_ftoa(float x, char *out, int decimals);

#endif // OS_H