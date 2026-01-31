#ifndef STDIO_H
#define STDIO_H

// PRINT: soporta %s %d %f y %%
void PRINT(const char *fmt, ...);

// READ: soporta %s %d %f
// Retorna cuantos campos asignó (estilo scanf)
int READ(const char *fmt, ...);

#endif

