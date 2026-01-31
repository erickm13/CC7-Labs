// stdio.c
#include "stdio.h"
#include "os.h"
#include <stdarg.h>

#ifndef STDIO_INPUT_MAX
#define STDIO_INPUT_MAX 128
#endif

#ifndef STDIO_FLOAT_PREC
#define STDIO_FLOAT_PREC 6
#endif

// ---------- helpers básicos ----------
static int is_space(char c) {
    return (c==' ' || c=='\t' || c=='\r' || c=='\n');
}

static int is_digit(char c) {
    return (c>='0' && c<='9');
}

static const char* skip_spaces(const char *p) {
    while (*p && is_space(*p)) p++;
    return p;
}

static const char* advance_token(const char *p) {
    p = skip_spaces(p);
    while (*p && !is_space(*p)) p++;
    return p;
}

// ---------- int <-> texto ----------
static void print_int(int x) {
    if (x == 0) { uart_putc('0'); return; }

    unsigned int n;
    if (x < 0) { uart_putc('-'); n = (unsigned int)(-x); }
    else n = (unsigned int)x;

    char buf[12];
    int i = 0;
    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10));
        n /= 10;
    }
    while (i--) uart_putc(buf[i]);
}

// ---------- float (requiere libgcc al linkear) ----------
static void print_float(double v, int prec) {
    if (v < 0) { uart_putc('-'); v = -v; }

    int ip = (int)v;
    double frac = v - (double)ip;

    print_int(ip);
    uart_putc('.');

    for (int i = 0; i < prec; i++) {
        frac *= 10.0;
        int d = (int)frac;
        uart_putc((char)('0' + (d % 10)));
        frac -= (double)d;
    }
}

// ---------- parsers ----------
static int parse_int_token(const char *tok, int *out) {
    tok = skip_spaces(tok);
    int sign = 1;

    if (*tok == '-') { sign = -1; tok++; }
    else if (*tok == '+') { tok++; }

    if (!is_digit(*tok)) return 0;

    int val = 0;
    while (is_digit(*tok)) {
        val = val * 10 + (*tok - '0');
        tok++;
    }
    *out = val * sign;
    return 1;
}

static int parse_float_token(const char *tok, float *out) {
    tok = skip_spaces(tok);
    int sign = 1;

    if (*tok == '-') { sign = -1; tok++; }
    else if (*tok == '+') { tok++; }

    if (!is_digit(*tok) && *tok != '.') return 0;

    double val = 0.0;

    while (is_digit(*tok)) {
        val = val * 10.0 + (double)(*tok - '0');
        tok++;
    }

    if (*tok == '.') {
        tok++;
        double place = 0.1;
        while (is_digit(*tok)) {
            val += (double)(*tok - '0') * place;
            place *= 0.1;
            tok++;
        }
    }

    val *= (double)sign;
    *out = (float)val;
    return 1;
}

static int copy_token(const char *src, char *dst, int max) {
    src = skip_spaces(src);
    int i = 0;
    while (*src && !is_space(*src)) {
        if (i < max - 1) dst[i++] = *src;
        src++;
    }
    dst[i] = '\0';
    return i;
}

// ================= PRINT =================
void PRINT(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            uart_putc(*p);
            continue;
        }

        p++; // saltar '%'
        if (*p == '\0') break;

        if (*p == '%') {
            uart_putc('%');
        } else if (*p == 's') {
            const char *s = va_arg(ap, const char*);
            if (!s) s = "(null)";
            uart_puts(s);
        } else if (*p == 'd') {
            int x = va_arg(ap, int);
            print_int(x);
        } else if (*p == 'f') {
            double f = va_arg(ap, double); // float se promueve a double
            print_float(f, STDIO_FLOAT_PREC);
        } else {
            // no soportado => lo dejamos literal
            uart_putc('%');
            uart_putc(*p);
        }
    }

    va_end(ap);
}

// ================= READ =================
int READ(const char *fmt, ...) {
    char line[STDIO_INPUT_MAX];
    uart_getline(line, STDIO_INPUT_MAX);

    va_list ap;
    va_start(ap, fmt);

    int assigned = 0;
    const char *in = line;

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') continue;
        p++;
        if (*p == '\0') break;

        in = skip_spaces(in);

        if (*p == 'd') {
            int *out = va_arg(ap, int*);
            if (!parse_int_token(in, out)) break;
            assigned++;
            in = advance_token(in);

        } else if (*p == 'f') {
            float *out = va_arg(ap, float*);
            if (!parse_float_token(in, out)) break;
            assigned++;
            in = advance_token(in);

        } else if (*p == 's') {
            char *out = va_arg(ap, char*);
            // OJO: asumimos que out tiene espacio suficiente
            copy_token(in, out, STDIO_INPUT_MAX);
            assigned++;
            in = advance_token(in);

        } else if (*p == '%') {
            // no consume input
        } else {
            break;
        }
    }

    va_end(ap);
    return assigned;
}

