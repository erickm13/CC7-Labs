#define UART0_BASE 0x101f1000

#define UART_DR      0x00  // Data Register
#define UART_FR      0x18  // Flag Register
#define UART_FR_TXFF 0x20  // Transmit FIFO Full
#define UART_FR_RXFE 0x10  // Receive FIFO Empty

volatile unsigned int * const UART0 = (unsigned int *)UART0_BASE;

// Function to send a single character via UART
void uart_putc(char c) {
    // Wait until there is space in the FIFO
    while (UART0[UART_FR / 4] & UART_FR_TXFF);
    UART0[UART_DR / 4] = c;
}

// Function to receive a single character via UART
char uart_getc() {
    // Wait until data is available
    while (UART0[UART_FR / 4] & UART_FR_RXFE);
    return (char)(UART0[UART_DR / 4] & 0xFF);
}

// Function to send a string via UART
void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

// Function to receive a line of input via UART
void uart_gets_input(char *buffer, int max_length) {
    int i = 0;
    char c;
    while (i < max_length - 1) { // Leave space for null terminator
        c = uart_getc();
        if (c == '\n' || c == '\r') {
            uart_putc('\n'); // Echo newline
            break;
        }
        uart_putc(c); // Echo character
        buffer[i++] = c;
    }
    buffer[i] = '\0'; // Null terminate the string
}

// Simple function to convert string to integer
int uart_atoi(const char *s) {
    int num = 0;
    int sign = 1;
    int i = 0;

    // Handle optional sign
    if (s[i] == '-') {
        sign = -1;
        i++;
    }

    for (; s[i] >= '0' && s[i] <= '9'; i++) {
        num = num * 10 + (s[i] - '0');
    }

    return sign * num;
}

// Function to convert integer to string
void uart_itoa(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0 && i < 14) { // Reserve space for sign and null terminator
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // Reverse the string
    int start = 0, end = i - 1;
    char temp;
    while (start < end) {
        temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;
        start++;
        end--;
    }
}

// Convert string to float 
float uart_atof(const char *s) {
    int i = 0;
    int sign = 1;

    // Handle optional sign
    if (s[i] == '-') { 
        sign = -1; 
        i++; 
    }


    // Build integer part substring (digits only)
    char parte_entera[32];
    int ip = 0;

    while (s[i] != '.' ) {
        if (s[i] < '0' || s[i] > '9'){
            break;  // <-- solo dígitos
        } 
        parte_entera[ip++] = s[i++];
    }
    parte_entera[ip] = '\0';

    // Convert integer part using existing uart_atoi

    int int_part;

    if (ip > 0) {
        int_part = uart_atoi(parte_entera);
    } else {
        int_part = 0;
    }


    // If no decimal point, done
    if (s[i] != '.') {
        return (float)(sign * int_part); // 1 o -1 x algo xd
    }

    // pasamos el punto
    i++;

    // Build decimal part substring (digits only)
    char parte_decimal[32];
    int positionArray = 0;

    while (s[i] && positionArray < (int)sizeof(parte_decimal) - 1) {
        if (s[i] < '0' || s[i] > '9') break;
        parte_decimal[positionArray++] = s[i++];
    }
    parte_decimal[positionArray] = '\0';

    // pasa strings a enteros usando uart_atoi
    int dec_part;

    if (positionArray > 0) {
        dec_part = uart_atoi(parte_decimal);
    } else {
        dec_part = 0;
    }


    float div = 1;

    // 10 100 1000 etc dependiendo de la cantidad de dígitos decimales
    for (int k = 0; k < positionArray; k++) {
        div *= 10;
    }

    float result;

    // Si hay parte decimal
    if (positionArray > 0) {
        result = (float)int_part + ((float)dec_part / div); //4.3 = 4 + 3/10
    } else {
        result = (float)int_part; 
    }

    // Aplica el signo
    result = (float)sign * result;
    return result;

}


// convert float to string
void uart_ftoa(float x, char *buffer, int decimals) {
    int posicion = 0;

    // si vienen negativo lo convierte
    if (x < 0) {
        buffer[posicion++] = '-';
        x = -x;
    }

    // separa entero y fraccion
    int int_part = (int)x;
    float frac = x - (float)int_part;

    // parte entera la convertimos a string con itoa
    char intbuf[16];
    uart_itoa(int_part, intbuf);

    // metemos el string al buffer
    for (int i = 0; intbuf[i] != '\0'; i++) {
        buffer[posicion++] = intbuf[i];
    }

    // agregamos el punto decimal despues del entero
    buffer[posicion++] = '.';

    // pasamos decimal por deciaml a string 0.3 x 10 = 3.0 redondeamos con solo pasarlo a int lo volvemos ascci y lo metemos al buffer
    for (int i = 0; i < decimals; i++) {
        frac *= 10;
        int digit = (int)frac;
        buffer[posicion++] = (char)('0' + digit);
        frac -= (float)digit;
    }

    buffer[posicion] = '\0';
}
