# TP2 — 72.11 Sistemas Operativos — 2C 2025

Implementación de un sistema operativo monolítico de 64 bits sobre x64BareBones
(Pure64). El kernel administra interrupciones, system calls, drivers de teclado y
video gráfico, dos administradores de memoria intercambiables (free list y buddy
con árbol), un scheduler Round Robin con prioridades, semáforos contadores y
nombrados, pipes con FIFO bloqueante, y file descriptors por proceso. En userland
corre un shell interactivo con soporte para pipelines y procesos en background, y
un proceso terminal aparte que renderiza la salida a framebuffer.

## Integrantes

- Octavio Roberts Sanchez — Grupo 18

## Instrucciones de Compilación y Ejecución

### Requisitos Previos

Para compilar nativamente se necesitan los siguientes paquetes:

```bash
sudo apt-get install nasm qemu-system-x86 gcc make
```

Alternativamente, el repo trae un `Dockerfile` con la imagen
`agodio/itba-so-multiarch:3.1` que provee la toolchain x86_64-linux-gnu lista.

### Compilación

Hay dos variantes del administrador de memoria que se compilan con reglas
separadas:

**Compilación estándar (free list con coalescing):**
```bash
make clean all
```

**Compilación con Buddy System:**
```bash
make clean buddy
```

El build genera secuencialmente:

1. El bootloader Pure64 + BMFS (`Bootloader/`)
2. El kernel (`Kernel/kernel.bin`)
3. El módulo de userland (`Userland/0000-sampleCodeModule.bin`)
4. La imagen del disco (`Image/x64BareBonesImage.qcow2`)

### Ejecución

Para correr el sistema operativo con el memory manager por defecto:

```bash
./run.sh
```

Para correrlo con el Buddy System:

```bash
./run.sh buddy
```

`run.sh` limpia builds previos, recompila, lanza QEMU con la imagen generada y
deja la pantalla del kernel en una ventana de QEMU; al cerrarse, hace un `make
clean` final.

## Instrucciones de Replicación

### Comandos Disponibles

#### Comandos del shell

| Comando | Descripción | Parámetros | Uso |
|---------|-------------|------------|-----|
| `help`      | Lista los comandos disponibles y los tests. | Ninguno | `help` |
| `clear`     | Limpia la pantalla (manda `\f` al terminal). | Ninguno | `clear` |
| `echo`      | Escribe sus argumentos en STDOUT. | `<texto…>` | `echo hola mundo` |
| `datetime`  | Imprime la fecha y hora actuales del RTC. | Ninguno | `datetime` |
| `registers` | Imprime el snapshot de registros capturado con F6. | Ninguno | `registers` |
| `ls`        | "Lista" archivos (easter egg). | Ninguno | `ls` |
| `music`     | Reproduce una canción por el speaker PC. | `<r\|c>` (r=Rickroll, c=Coffin) | `music r` |
| `zero`      | Provoca una división por cero (excepción \#0). | Ninguno | `zero` |
| `invalid`   | Provoca un opcode inválido (excepción \#6). | Ninguno | `invalid` |
| `ps`        | Lista todos los procesos con PID, PPID, prioridad, foreground, estado, RSP, stack base y nombre. | Ninguno | `ps` |
| `mem`       | Muestra el estado del heap (total, ocupado, libre, fragmentos). | Ninguno | `mem` |
| `loop`      | Imprime el PID con un saludo cada N segundos (espera activa). | `[segundos]` (default 1) | `loop 3` |
| `kill`      | Mata un proceso por PID. | `<pid>` | `kill 5` |
| `nice`      | Cambia la prioridad de un proceso. | `<pid> <prioridad>` | `nice 5 3` |
| `block`     | Alterna el estado bloqueado/listo de un proceso (toggle). | `<pid>` | `block 5` |
| `cat`       | Copia STDIN a STDOUT hasta EOF. | Ninguno | `cat` o `echo hola \| cat` |
| `wc`        | Cuenta líneas, palabras y caracteres de STDIN. | Ninguno | `wc` o `echo "hola mundo" \| wc` |
| `filter`    | Filtra las vocales de STDIN. | Ninguno | `filter` o `echo hola \| filter` |
| `mvar`      | Spawnea writers/readers compartiendo un slot estilo Haskell MVar; cada reader pinta su letra con un color distinto. | `<writers> <readers>` | `mvar 2 3` |

#### Tests de cátedra

| Comando | Descripción | Parámetros | Uso |
|---------|-------------|------------|-----|
| `test_mm`   | Stress del memory manager: aloca/libera bloques y verifica que no se solapen. | `<max_bytes>` | `test_mm 1000000` |
| `test_proc` | Stress del scheduler: crea/bloquea/mata procesos. | `<max_procs>` | `test_proc 10` |
| `test_prio` | Tres procesos con prioridades distintas contando hasta un máximo. | `<max_value>` | `test_prio 1000000` |
| `test_sync` | Crea pares de procesos que incrementan/decrementan una variable global con y sin semáforos, con n iteraciones | `<pair_processes> <n_iter> <0|1>` | `test_sync 4 1000 1` |

### Caracteres Especiales

#### Pipes (`|`)

El pipe conecta STDOUT de la etapa N con STDIN de la etapa N+1:

```bash
echo hola | wc
cat | filter
```

El shell soporta **una sola** `|` por línea (máximo 2 etapas de pipeline).

#### Ejecución en Background (`&`)

`&` al final de la línea ejecuta el proceso sin esperar:

```bash
loop 3 &
ps
kill 4
```

`&` debe estar al final. El shell saca al proceso del foreground set, así Ctrl+C
no lo afecta.

### Atajos de Teclado

| Atajo | Función | Descripción |
|-------|---------|-------------|
| `Ctrl+C` | Interrumpir foreground | Mata todos los procesos en foreground del shell. |
| `Ctrl+D` | EOF en foreground | Marca EOF en STDIN de cada proceso foreground cuyo STDIN sea el pipe del teclado, y despierta los reads bloqueados con un `\0`. |
| `F6` (Print Screen) | Snapshot de registros | Captura el contexto de CPU del momento; se imprime después con `registers`. |
| `Backspace` | Borrar carácter | Borra el último carácter tipeado en la línea de comando. |

### Escape Sequences del Terminal

El terminal de userland interpreta secuencias `\033<cmd><payload>;` para
control visual:

| Secuencia | Efecto |
|-----------|--------|
| `\033c;`           | Limpia la pantalla. |
| `\033f<6hex>;`     | Setea el color de foreground (6 dígitos hex RGB). |
| `\033b<6hex>;`     | Setea el color de background (también limpia la pantalla). |
| `\033r;`           | Reset de colores a defaults (blanco sobre negro). |

Ejemplos:

```c
sys_write(STDOUT, "\033fFF0000;ERROR\033r;\n", 19);  // ERROR en rojo
sys_write(STDOUT, "\033c;", 3);                     // clear screen
```

`shell.c` y `mvar.c` los usan para colorear el prompt, el welcome, los
mensajes de error y las letras que los readers de la MVar escriben.

### Ejemplos de Uso

#### Ejemplo 1: Pipeline simple
```bash
echo hola mundo | wc
```
Salida: `1 lineas, 2 palabras, 11 caracteres`

#### Ejemplo 2: Filtrar vocales
```bash
echo abracadabra | filter
```
Salida: `brcdbr`

#### Ejemplo 3: Ejecución en background
```bash
loop 2 &
ps
```
Muestra el proceso `loop` corriendo en background; el shell sigue aceptando
comandos.

#### Ejemplo 4: Gestión de procesos
```bash
loop 5 &
ps
kill 3
ps
```
Crea, lista, mata y verifica.

#### Ejemplo 5: Snapshot de registros
```
[apretar F6 en cualquier momento]
registers
```
Imprime los registros de CPU del instante en que se apretó F6.

#### Ejemplo 6: MVar
```bash
mvar 2 3
```
Spawnea 2 escritores (letras A, B) y 3 lectores (cada uno con su color). El
proceso `mvar` termina inmediatamente; los hijos quedan corriendo hasta que se
los mata explícitamente con `kill`.

#### Ejemplo 7: Test de memoria
```bash
test_mm 500000
```
Aloca/libera bloques hasta 500 KB y verifica que no se solapan ni se corrompen.

#### Ejemplo 8: Test de sincronización
```bash
test_sync N 1000 1
```
N pares de procesos suman/restan sobre una variable global 1000 veces usando un semáforo
nombrado; el resultado esperado es 0.

#### Ejemplo 9: Cambio de prioridad
```bash
test_prio 1000000
```
Tres procesos con prioridades distintas cuentan hasta el mismo número; los de
prioridad alta terminan antes.

### Requerimientos Cumplidos

Se cumplen todos los requerimientos de la consigna del TP:

- **Memory Manager** con dos implementaciones intercambiables (free list y buddy
  con árbol) seleccionables por make target.
- **Procesos** con context switch por software, jerarquía padre/hijo, scheduler
  Round Robin con prioridades por nivel de quantums, `block`/`unblock`, `kill`,
  `nice`, `yield`, `waitpid`.
- **Sincronización** por semáforos contadores con spinlock atómico y semáforos
  nombrados compartidos por proceso.
- **IPC** por pipes bloqueantes con tabla de file descriptors por proceso,
  herencia de fds a hijos, override de fds al spawnear, y `dup`/`close`/`reopen`.
- **Shell** en userland con parser de tokens, soporte de un `|` por línea, `&`
  para background, eco con manejo de backspace, y reaping de zombies entre
  prompts.
- **Comandos** pedidos por la consigna (`help`, `clear`, `echo`, `ps`, `mem`,
  `loop`, `kill`, `nice`, `block`, `cat`, `wc`, `filter`, `mvar`) y de cátedra
  (`test_mm`, `test_proc`, `test_prio`, `test_sync`).

## Limitaciones

1. **Memoria:**
   - Hay un máximo de **64 procesos simultáneos** (`MAX_PROCESSES = 64`).
   - El tamaño máximo de un comando es **128 caracteres** (`MAX_COMMAND_LENGTH = 128`).
   - El shell tokeniza a lo sumo **16 tokens por etapa** (`MAX_TOKENS = 16`).
   - El Buddy System tiene un pool fijo de **8192 nodos** (`MAX_NODES = 8192`);
     cargas muy fragmentadas pueden agotarlo.
   - No hay detección contra stack overflow; un proceso que use más de los 32
     KiB de stack producirá comportamiento indefinido.

2. **File Descriptors:**
   - Cada proceso tiene un máximo de **16 fds** abiertos simultáneamente
     (`MAX_FDS = 16`).

3. **Pipes:**
   - Buffer circular interno de **1024 bytes** (`PIPE_BUF_SIZE = 1024`). Un
     writer que escriba más rápido de lo que el reader consume se va a bloquear
     cuando el buffer esté lleno.
   - El shell soporta **una sola `|` por línea** (`MAX_PIPELINE = 2`).

4. **Teclado:**
   - Solo el layout US (QWERTY). No hay soporte de teclas dead, layouts
     internacionales ni autorepeat configurable.
   - No hay historial de comandos (las flechas no navegan).

5. **Video:**
   - Resolución y modo fijos por el bootloader Pure64.
   - Asume framebuffer lineal a 24/32 BPP; modos más exóticos pueden romper el
     renderizado de glifos.
   - No hay terminales múltiples ni ventanas: el terminal cubre la pantalla.

6. **Hardware:**
   - El sistema se desarrolló y probó en QEMU; en hardware real puede fallar.

7. **Scheduler:**
   - Round Robin con prioridades estáticas. La prioridad determina la cantidad
     de quantums que recibe el proceso por turno, no hay aging ni boost de I/O.

8. **Procesos:**
   - Si un proceso muere por una causa externa (kill, Ctrl+C) sin liberar
     memoria que haya alocado vía `sys_malloc`, esa memoria se filtra (el kernel
     no escanea el heap del proceso).
   - Si un proceso muere mientras tiene un semáforo tomado o esperándolo, se
     pueden generar interbloqueos en el resto de los esperadores.
   - No hay protección contra que el entry point de un proceso "retorne" sin
     llamar a `sys_exit` (se cae al stack de basura).

9. **Shell:**
   - Un proceso background que escriba a STDOUT comparte el área visual del
     prompt: la salida se intercala con lo que el usuario tipea.

## Citas de Fragmentos de Código / Uso de IA

**Nota importante:** Se utilizó IA durante el desarrollo para:
- Discutir alternativas de diseño antes de codificar.
- Documentar el código (docstrings y comentarios de bloque).
- Ajustes de prolijidad, modularización y formateo.

Toda la lógica funcional fue diseñada por el programador y codificada con ayuda de la IA como mano de obra unicamente, siempre con revision humana.

### Fragmentos de Código Relevantes

#### Spinlock atómico para semáforos

El TAD `Semaphore` serializa su estado con un test-and-set atómico (`__atomic_exchange_n`)
en lugar de cli/sti. En una arquitectura single-CPU con interrupciones por
hardware, la ventana del lock dura pocas instrucciones; evitar cli/sti es
crucial porque el ISR del teclado entra a `pipeWrite → semWait` y un `_sti`
final rompería IF en el medio de la atención.

```c
// Kernel/semaphores/semaphore.c
#define ACQUIRE(lock_ptr) \
    while (__atomic_exchange_n((lock_ptr), (uint8_t)1, __ATOMIC_ACQUIRE)) { }
#define RELEASE(lock_ptr) \
    __atomic_store_n((lock_ptr), (uint8_t)0, __ATOMIC_RELEASE)

int semWait(Semaphore sem) {
    if (!sem) return -1;
    ACQUIRE(&sem->lock);
    sem->value--;
    int mustBlock = (sem->value < 0);
    if (mustBlock) {
        int16_t currentPID = getCurrentPID();
        setCurrentBlocked();
        enqueue(sem->blockedQueue, (void*)(intptr_t)currentPID);
    }
    RELEASE(&sem->lock);
    if (mustBlock) forceSchedulerCall();
    return 0;
}
```

#### Función principal del context switch

Los procesos viven en `processTable[MAX_PROCESSES]`, indexados por PID para
acceso O(1). Una `readyQueue` FIFO guarda los PIDs listos. El "nivel de
prioridad" es la cantidad de quantums que el scheduler le entrega al proceso
cuando lo despacha: un proceso de prioridad 3 corre tres ticks antes de ceder.

```c
// Kernel/process_contextswitch/scheduler.c
uint64_t schedule(uint64_t prevRSP) {
    timer_handler();
    if (!schedulerEnabled) return prevRSP;
    if (!currentProcess) {
        pid_t nextPid = getNextPID();
        currentProcess = processTable[nextPid];
        setState(currentProcess, RUNNING);
        resetQuantums(currentProcess);
        useQuantum(currentProcess);
        return (uint64_t) getRSP(currentProcess);
    }
    setRSP(currentProcess, (uint64_t *) prevRSP);

    if (getState(currentProcess) == RUNNING) {
        if (hasQuantums(currentProcess)) {
            useQuantum(currentProcess);
            return prevRSP;
        }
        setState(currentProcess, READY);
        enqueue(readyQueue, (void *)(intptr_t) getPID(currentProcess));
    }
    pid_t nextPid = getNextPID();
    currentProcess = processTable[nextPid];
    setState(currentProcess, RUNNING);
    resetQuantums(currentProcess);
    useQuantum(currentProcess);
    return (uint64_t) getRSP(currentProcess);
}
```

#### Terminación de procesos y reaping

Cada proceso tiene un `terminated` (semáforo que postea cuando el proceso
muere) y un `childTerminated` que el padre usa para enterarse de la muerte de
algún hijo. `terminateProcess` cierra fds, marca al proceso como ZOMBIE,
postea ambos semáforos y transfiere los huérfanos a `init` (que vive en un
loop de `wait_any`). El shell llama a `sys_reap_zombies` entre prompts para
limpiar los procesos en background que hayan terminado.

```c
// Kernel/process_contextswitch/scheduler.c
int terminateProcess(pid_t pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    if (isUnkillable(pid)) return -1;
    Process process = processTable[pid];
    if (!process) return -1;

    pid_t   ppid   = getPPID(process);
    Process parent = (ppid >= 0 && ppid < MAX_PROCESSES) ? processTable[ppid] : 0;
    if (!parent) return -1;

    closeFdsOf(pid);
    foregroundSet[pid] = 0;
    notifyTermination(process, parent, pid);
    transferOrphanChildren(process);

    if (currentProcess && getPID(currentProcess) == pid) forceSchedulerCall();
    return 0;
}
```

#### Buddy con árbol

El Buddy System se implementa con un pool estático de `BuddyNode`. Cada nodo
representa un sub-bloque potencia de 2 del pool y se materializa cuando hace
falta partir un bloque más grande. Al fusionar buddies, los nodos hijos van a
una free-list interna para reciclarse.

```c
// Kernel/memory/memoryManagerBuddy.c
static int findAndAllocate(int nodeIdx, uint8_t requiredOrder) {
    if (nodeIdx < 0 || nodeIdx >= nodeCount) return -1;
    BuddyNode *node = &nodes[nodeIdx];

    if (node->order < requiredOrder) return -1;

    int exactResult = tryAllocateExactSize(nodeIdx, requiredOrder);
    if (exactResult != -1) return exactResult;

    if (node->leftChild == -1)
        return trySplitAndAllocate(nodeIdx, requiredOrder);

    int result = findAndAllocate(node->leftChild, requiredOrder);
    if (result != -1) { updateNodeStatus(nodeIdx); return result; }

    result = findAndAllocate(node->rightChild, requiredOrder);
    if (result != -1) updateNodeStatus(nodeIdx);
    return result;
}
```

---

**Autor:** Octavio Roberts Sanchez — Grupo 18

**Base:** x64BareBones (Rodrigo Rearden, Augusto Nizzo McIntosh) sobre
Pure64.
