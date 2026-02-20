**Laboratorio 4: Creación y Gestión de Procesos - Reporte**

---

**1. Process Creation (fork)**

`fork()` crea un nuevo proceso (hijo) duplicando el proceso actual. 
- Retorna **0** en el hijo
- Retorna **PID del hijo** en el padre
- Retorna **-1** si falla

El hijo hereda una copia del espacio de direcciones del padre (variables, memoria).

---

**2. Synchronization (waitpid)**

El padre usa `waitpid(pid, NULL, 0)` para esperar a que el hijo termine.
- **Bloquea** al padre hasta que el hijo finalice
- Recolecta el estado de terminación del hijo
- **Evita procesos zombie** (procesos que ya terminaron pero no fueron recolectados)

---

**3. IPC Mechanisms**

**Pipes:**
- `pipe(pipefd)` crea un canal unidireccional
- `pipefd[0]` = read end, `pipefd[1]` = write end
- Comunicación: padre escribe en [1], hijo lee de [0]
- El kernel administra el buffer

**Shared Memory:**
- `shmget()` crea un segmento de memoria compartida
- `shmat()` adjunta el segmento al espacio de direcciones del proceso
- Múltiples procesos pueden acceder al mismo segmento simultáneamente
- **Más eficiente que pipes** - no hay copia de datos

---

**Conclusión:**
- fork() crea procesos ligeros
- waitpid() sincroniza padre-hijo
- Pipes = comunicación por flujo (kernel buffer)
- Shared Memory = comunicación directa por memoria

---

## Execution Output

```
Laboratorio 4: Creacion y Manipulacion de Procesos

>>>> 1. Creating a New Process <<<<
Child Process: PID = 9829 Parent PID = 9823
Parent Process: PID = 9823
>>>> 2. Synchronized Parent (waitpid) <<<<
Child Process: PID = 9844, Parent PID = 9823
Parent Process: Child has finished execution
>>>> 3. Inter-Process Communication Using Pipes <<<<
Child Process: Received "Hello from Parent"
Parent Process: Writing "Hello from Parent"
>>>> 4. Creating Multiple Child Processes <<<<
Parent Process: PID = 9823
Child 1: PID = 9846, Parent PID = 9823
Child 2: PID = 9847, Parent PID = 9823
Child 3: PID = 9848, Parent PID = 9823
>>>> 5. Shared Memory <<<<
Child Process: Read "Shared Memory Example"
Parent Process: Writing "Shared Memory Example"
```
