#include "stdio.h"
#include "../os/os.h"
#include <stdarg.h>

void PRINT(const char *fmt, ...) {
    va_list listaValoresPrint; // ap son los valores que esten en los 3 puntitos
    va_start(listaValoresPrint, fmt); // empieza a leer los valores de ... despues de fmt 

    for (int i = 0; fmt[i] != '\0'; i++) {
        if(fmt[i] != '%') {
            uart_putc(fmt[i]);
            continue;
        }else{
            i++;
            char tipoDato = fmt[i]; //guardanmos d, f etc ...
                if(tipoDato == 'd') {
                    int num = va_arg(listaValoresPrint, int); // obtiene el siguiente valor de listaValoresPrint como int
                    char buffer[16];
                    uart_itoa(num, buffer);
                    uart_puts(buffer);
                }else if(tipoDato == 'f') {
                    double num = va_arg(listaValoresPrint, double); // obtiene el siguiente valor de listaValoresPrint como double
                    char buffer[32];
                    uart_ftoa((float)num, buffer, 3); // Convert float to string with 3 decimal places
                    uart_puts(buffer);
                }else{
                    uart_putc('%');
                    uart_putc(tipoDato);
                }
        }
    }
}


void READ(const char *fmt, ...) {
    va_list listaValoresRead;
    va_start(listaValoresRead, fmt);
    char buffer[64];
    uart_gets_input(buffer, sizeof(buffer));
    if (fmt[0] == '%' && fmt[1] == 'd') {
        int* direccionVar = va_arg(listaValoresRead, int*); // obtiene el siguiente valor de listaValoresRead como puntero a int
        int numCast = uart_atoi(buffer);
        *direccionVar = numCast; // Asigna el valor convertido al puntero
    }else if(fmt[0] == '%' && fmt[1] == 'f') {
        float* direccionVar = va_arg(listaValoresRead, float*); // obtiene el siguiente valor de listaValoresRead como puntero a float
        float numConvert = uart_atof(buffer);
        *direccionVar = numConvert; // Asigna el valor convertido al puntero
    }
}
