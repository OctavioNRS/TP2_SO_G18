# TP2 — Sistemas Operativos Grupo 18
Implementación de un kernel monolítico de 64 bits sobre x64BareBones (Pure64) con
manejo de interrupciones, system calls, drivers de teclado y video gráfico,
administración de memoria intercambiable, scheduler Round Robin con prioridades,
semáforos, pipes y un shell de usuario.

## Integrantes

- Octavio Roberts Sanchez

## Compilación y ejecución

### Requisitos del entorno

Para compilar, se necesitan las siguientes dependencias instaladas:

```bash
sudo apt-get install nasm qemu-system-x86 gcc make
```

### Compilacion

Las reglas `make`, `make all`, `make buddy` están reservadas para la
compilación dentro del contenedor; cualquier otra tarea (bajar la imagen,
iniciar Docker, arrancar QEMU) se hace con scripts/recetas dedicadas.

| Comando | Acción |
| --- | --- |
| `make all` (default) | Compila con el memory manager por defecto (**free list** con coalescing). |
| `make buddy`         | Compila con el **Buddy System** (`-DUSE_BUDDY`). |
| `make clean`         | Limpia los `.o`, `.bin` y la imagen. |

### Construir + arrancar QEMU

Desde el host:

```bash
docker run --rm -v "$PWD":/src -w /src agodio/itba-so-multiarch:3.1 make clean all
./run.sh           # arranca QEMU con la imagen recién compilada
./run.sh buddy     # idem, pero usando el Buddy System como memory manager
```

`run.sh` lanza `qemu-system-x86_64 -hda Image/x64BareBonesImage.qcow2 -m 4096`
con audio para soporte del speaker (comando `music`).

## Arquitectura

- **Bootloader**: Pure64 carga el kernel en `0x100000`. El primer objeto del
  link es `Kernel/asm/loader.o` (la directiva `OUTPUT_FORMAT("binary")` del
  linker script obliga a que el entry esté al inicio del binario).
- **Kernel** monolítico de 64 bits en modo largo (ring 0 sin separación de
  privilegios). Provee drivers de teclado, video gráfico, RTC, speaker y
  manejo de excepciones (división por cero, opcode inválido).
- **Procesos** en kernel-space comparten el espacio de direcciones; cada uno
  tiene su propio stack de 32 KiB.
- **Userland**: un único módulo (`0000-sampleCodeModule.bin`) que cargado en
  `0x400000` arranca el shell. El bootstrap del sistema es:

  ```text
  kernel main → init (PID 1, kernel) → shell (PID 2, userland) → terminal (PID 3, userland)
  ```

  El proceso `init` crea el pipe del teclado, conecta el ISR al lado de
  escritura del pipe y lanza el shell. El shell, a su vez, crea un pipe
  hacia un proceso `terminal` que se encarga del render TTY al framebuffer.

### Mecanismos implementados

- **Administradores de memoria intercambiables**: free list (default) y buddy
  system (`make buddy`). Ambos comparten la interfaz `mem_init`/`mem_alloc`/
  `mem_free`/`mem_status` definida en `Kernel/include/memoryManager.h`. La
  selección se hace por preprocesador con `#ifdef USE_BUDDY`.
- **Scheduler Round Robin con prioridades**: cada proceso tiene un valor de
  prioridad que define la cantidad de quantums por turno. Ready queue FIFO.
  PID 0 (`idle`) corre cuando no hay nadie listo; PID 1 (`init`) reapea
  huérfanos mediante `wait_any` en loop.
- **System calls**: int `0x80`. La tabla completa está en
  `Kernel/syscallDispatcher.c` (43 syscalls).
- **Semáforos**: bloqueantes, sin busy-wait, atomicidad con spinlock atómico
  (`__atomic_exchange_n`). Pueden ser compartidos entre procesos no
  relacionados acordando un identificador por nombre (`sys_sem_open`).
- **Pipes**: unidireccionales, lectura y escritura bloqueantes, buffer
  circular (`PIPE_BUF_SIZE = 1024`). El TAD está dividido en capas:
  - `CircularBuffer` (FIFO puro de bytes, sin sincronización).
  - `Pipe` (CircularBuffer + 4 semáforos: `dataAvailable`, `freeSpaces`,
    `readMutex`, `writeMutex` + contadores `refCount` y `writerCount`).
  - `FileAccess` (Pipe + flags `FILE_READ`/`FILE_WRITE`/`FILE_NONBLOCK` +
    refcount propio). Es lo que vive en la tabla de fds del proceso.

  Cada proceso tiene una tabla de file descriptors (`MAX_FDS = 16`) donde
  cada slot apunta a un `FileAccess`. `read`/`write` son transparentes: para
  un proceso es indistinto leer/escribir de un pipe o de la "terminal" (todo
  va vía pipes, terminal incluido). Cuando el último writer cierra,
  `removePipeWriter` despierta a los readers bloqueados para que detecten
  EOF.
- **Soporte de Ctrl+C y Ctrl+D**: ver sección "Atajos de teclado".

## Replicación

### Comandos del shell

Cada comando se ejecuta como un proceso independiente. Los que aparecen como
*pipeable* leen STDIN / escriben STDOUT, y por lo tanto se pueden conectar con
`|` u operar sobre la terminal de forma transparente.

| Nombre | Parámetros | Descripción |
| --- | --- | --- |
| `help` | — | Lista todos los comandos disponibles. |
| `clear` | — | Limpia la pantalla. |
| `echo` | `<args...>` | Imprime sus argumentos separados por espacio + `\n`. |
| `datetime` | — | Hora y fecha actual (lee el RTC). |
| `registers` | — | Imprime el último snapshot de registros (ver F6 abajo). |
| `ls` | — | "No file system" (gag con la canción de Rickroll). |
| `music` | `r`\|`c` | Reproduce Rickroll (`r`) o Coffin Dance (`c`) en el speaker. |
| `zero` | — | Provoca una división por cero (testea exc. 0). |
| `invalid` | — | Provoca un opcode inválido (testea exc. 6). |
| `ps` | — | Lista los procesos (PID, PPID, prioridad, estado, nombre). |
| `mem` | — | Estado del heap: total / ocupada / libre / fragmentos. |
| `loop` | `[secs]` | Imprime su PID cada `secs` segundos (default 1) por espera **activa**. |
| `kill` | `<pid>` | Termina al proceso con ese PID. |
| `nice` | `<pid> <prio>` | Cambia la prioridad de un proceso. `prio` debe ser `> 0`. |
| `block` | `<pid>` | Toggle: si está corriendo lo bloquea; si está bloqueado lo desbloquea. |
| `cat`  | — | Imprime su STDIN tal cual lo recibe. |
| `wc`  | — | Cuenta líneas, palabras y caracteres del STDIN. |
| `filter` | — | Filtra las vocales del STDIN. |
| `mvar` | `<writers> <readers>` | MVar de Haskell (ver más abajo). |
| `test_mm` | `<max_bytes>` | Stress del memory manager. |
| `test_proc` | `<max_procs>` | Stress del scheduler. |
| `test_prio` | `<max_value>` | 3 procesos con prioridades distintas contando hasta `max_value`. |
| `test_sync` | `<n_iter>` | Race conditions con y sin semáforos. |

#### Notas particulares

- **`cat`, `wc`, `filter`** en modo interactivo (sin `|`) **no hacen eco** de
  lo tipeado: el sistema no tiene una capa TTY que duplique las pulsaciones,
  así que en cuanto el shell deja de leer y empieza a leer el comando, los
  caracteres tipeados van directo al pipe del proceso. Es esperado: en
  pipelines (`echo hola | wc`) lo recibe correctamente y para terminar la
  entrada hay que mandar EOF con Ctrl+D.
- **`registers`** muestra el snapshot tomado en el momento en que se apretó
  **F6**. F6 captura los registros del proceso interrumpido en ese instante.
  Si nunca se apretó F6 el comando imprime "No snapshot taken".

### Caracteres especiales

| Símbolo | Significado |
| --- | --- |
| `|` | Pipe entre dos procesos. Sólo se soporta **un** `|` por línea (consigna). |
| `&` al final | Lanza el comando en background (el shell devuelve el prompt inmediatamente). |

Ejemplos:

```text
[SHELL] > echo hola mundo | wc          # pipea echo a wc
[SHELL] > loop &                        # loop corriendo en background
[SHELL] > test_proc 8 &                 # test en background, sigue interactivo
```

### Atajos de teclado

| Tecla | Acción |
| --- | --- |
| Ctrl+C | Mata todos los procesos en foreground (los del último comando sin `&`). |
| Ctrl+D | Manda EOF al STDIN del proceso en foreground cuyo STDIN sea el teclado. En `cat`, `wc`, `filter` interactivos, hace que el comando termine. |
| F6 | Toma un snapshot de los registros del proceso interrumpido. Verlo con `registers`. |

### Ejemplos de demostración

A continuación, ejemplos por fuera de los tests provistos para mostrar cada
requerimiento.

#### IPC: pipes

```text
[SHELL] > echo hola que tal | wc
0 lineas, 3 palabras, 13 caracteres
[SHELL] > echo abcdefghijklmnop | filter
bcdfghjklmnp
```

Modo interactivo con EOF:

```text
[SHELL] > cat
hola      <Enter> ⇢ no se ve el eco, pero cat ya lo recibió
hola
<Ctrl+D>  ⇢ cat termina
```

#### Foreground / background

```text
[SHELL] > loop 2 &
Hola del PID 3
[SHELL] > ps
... PID 3 corriendo en background ...
[SHELL] > kill 3
```

`Ctrl+C` sobre el primer `loop` (sin `&`) lo termina al instante; sobre el
mismo `loop &` no hace nada (no está en foreground).

#### Scheduler / prioridades

```text
[SHELL] > test_prio 100000
== Misma prioridad ==
  PID X termino  (los tres en orden similar)
== Prioridades distintas ==
  PID Y termino  (en orden HIGH > MED > LOW)
test_prio OK
```

#### Memory manager

```text
[SHELL] > mem
Memoria (bytes):
  total    : 268435456
  ocupada  : 81920
  libre    : 268353536
  fragments: 1
[SHELL] > test_mm 65536
test_mm: inicio ...
iter 0 OK: 14 bloques, 65535 bytes
...
test_mm OK
```

Re-compilando con `make buddy`, el mismo `test_mm` ejercita el Buddy.

#### Sincronización

```text
[SHELL] > test_sync 5000
test_sync: ronda SIN semaforo (race esperada)...
sin semaforo: valor final = 4983 (esperado: distinto de 0 (race))
test_sync: ronda CON semaforo (esperado: 0)...
con semaforo: valor final = 0 (esperado: 0)
test_sync OK
```

#### MVar (lectores/escritores)

`mvar <W> <R>` crea `W` escritores que ponen letras `'A'`, `'B'`, ... en una
variable compartida, y `R` lectores que las consumen. El comando *termina al
instante*: los writers/readers quedan corriendo y los adopta init. Para
detenerlos hay que matarlos con `kill <pid>`. Ejemplo:

```text
[SHELL] > mvar 2 1                  # un solo lector consume A y B alternando
ABABABABAB...
[SHELL] > ps                        # los mvar_w / mvar_r siguen corriendo
[SHELL] > nice <pid_de_B> 4         # priorizo a B → ahora B aparece más
ABABBBABBBABBBAB...
[SHELL] > kill <pid_de_B>           # mato a B → sólo aparece A
AAAAAA...
```

## Tests de la cátedra

| Test | Parámetros | Nota |
| --- | --- | --- |
| `test_mm` | `<max_memory>` | Ciclo "infinito" (8 rondas por ejecución) que pide/libera bloques aleatorios y chequea que no se solapen. Pasa con free-list y con buddy. |
| `test_proc` | `<max_procs>` | Crea, bloquea, desbloquea y mata procesos `endlessLoop` aleatoriamente. Sólo imprime errores. |
| `test_prio` | `<max_value>` | Tres procesos que cuentan hasta `max_value` con prio igual y luego con prio distinta. |
| `test_sync` | `<n_iter>` | Dos pares de procesos incrementan/decrementan una variable global, con y sin semáforos. |

Todos se pueden correr en foreground (`test_mm 65536`) o background
(`test_mm 65536 &`).

## Requerimientos parcialmente cubiertos / limitaciones

- **Eco interactivo de comandos pipeables**: `cat`, `wc` y `filter`, cuando se
  ejecutan sin pipeline, no muestran lo tipeado por pantalla porque el sistema
  no tiene un layer TTY. Esto es por diseño (todo I/O va por pipes y no hay
  modo "canónico"); el shell sí ecoa lo tipeado.
- **Pipelines de más de 2 etapas**: la consigna pide soportar `p1 | p2`. El
  shell limita el pipeline a 2 etapas para mantenerse fiel a ese
  requerimiento.
- **Render del MVar con colores**: el ejemplo de la consigna sugiere
  identificar a cada lector con un color. Como el terminal no soporta
  escape sequences ANSI, los lectores imprimen sólo el carácter consumido;
  se puede identificar la actividad de cada lector mirando `ps` y la
  frecuencia de aparición de cada letra escritora.
- **Persistencia de procesos en background al cerrar el shell**: el shell
  reapeas zombies entre prompts (`sys_reap_zombies`), pero los procesos
  todavía corriendo en background son hijos del shell. Si el shell muere
  son adoptados por init, que los reapea al terminar.

## Citas de fragmentos de código y uso de IA

- El esqueleto inicial (Bootloader Pure64, Toolchain, Image, _loader.c,
  prepareStack y manejo de IDT) viene del repositorio
  [x64BareBones](https://bitbucket.org/RowDaBoat/x64barebones/) provisto por
  la cátedra y del TP1 de Arquitectura de Computadoras del grupo.
- Drivers de video y teclado, mapeo de scancode set 1, RTC, speaker y
  manejo de excepciones fueron adaptados del TP1 de Arquitectura.
- Se utilizó IA (Claude/ChatGPT) puntualmente para:
  - Discutir el diseño del manejo de Ctrl+D y la propagación de EOF a través
    de pipes (filtro de `'\0'` en `readFdOf`, criterio de marcar EOF sólo
    en procesos cuyo STDIN sea el pipe del teclado).
  - Revisar el manejo de anidación de IF en ISRs (helpers
    `_save_flags_cli` / `_restore_flags`).
  - Generar el esqueleto del Buddy System (algoritmo de buddies XOR).
- Las decisiones de arquitectura, partición de responsabilidades
  (consola/terminal en userland) y la implementación final son del grupo.
