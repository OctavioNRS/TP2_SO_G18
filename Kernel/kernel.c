/*
 * kernel.c — Entry point en C del kernel.
 *
 * `main` corre dentro de la pila del stub del bootloader. Inicializa, en orden:
 *   1. el memory manager (mem_init sobre MEM_HEAP_START/SIZE);
 *   2. el scheduler (registrando `initProcessEntry` como PID 1);
 *   3. la IDT (`load_idt` habilita IRQs con _sti al final);
 *   4. el scheduler (lo "destraba" para que el próximo timer-tick haga el primer
 *      context switch).
 *
 * Después entra en un `_hlt` infinito: cualquier IRQ pisará el hlt y disparará
 * el scheduling. El proceso init arma el pipe del teclado y lanza al shell.
 */
#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <videoDriver.h>
#include <keyboardDriver.h>
#include <idtLoader.h>
#include <interrupts.h>
#include <memoryManager.h>
#include <scheduler.h>
#include <pipe.h>
#include <fileAccess.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;


void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//Tamaño del stack, 32KiB
		- sizeof(uint64_t)			//Arranco de la parte superior
	);
}

void * initializeKernelBinary()
{
	void * moduleAddresses[] = {
		sampleCodeModuleAddress,
		sampleDataModuleAddress
	};
	loadModules(&endOfKernelBinary, moduleAddresses);
	clearBSS(&bss, &endOfKernel - &bss);
	return getStackBase();
}

/*
 * Proceso init (PID 1).
 *
 *  1. Crea el pipe del teclado y registra a init como su único lector (su fd 0).
 *  2. Conecta el ISR del teclado al lado de escritura del pipe.
 *  3. Lanza al shell (sampleCodeModule, cargado en 0x400000), que hereda los fds de
 *     init. Como init tiene fd 0 conectado al pipe del teclado y fd 1/2 vacíos, el
 *     shell arranca con STDIN = teclado y STDOUT/STDERR = /dev/null; el propio shell
 *     se encarga de cablear STDOUT/STDERR a un pipe que va al proceso Terminal (que
 *     queda como hijo del shell).
 *  4. Entra en wait_any en loop reapeando huérfanos (procesos cuyo padre murió).
 */
static int initProcessEntry(int argc, char **argv)
{
	(void)argc; (void)argv;

	/* Pipe del teclado: init mantiene el read-end (fd 0 = STDIN); el driver del
	 * teclado mantiene el write-end. Cualquier hijo de init hereda fd 0 vía
	 * copyFdTable. */
	Pipe       kbdPipe   = newPipe();
	FileAccess kbdRead   = newFileAccess(kbdPipe, FILE_READ);
	FileAccess kbdWrite  = newFileAccess(kbdPipe, FILE_WRITE);
	addCurrentFd(kbdRead);
	initializeKeyboardDriver(kbdWrite);

	startNewProcess((ProcessEntryPoint)sampleCodeModuleAddress, "shell", 1, 0, 0);

	while (1) wait_any();
	return 0;
}

int main()
{
	/* Orden: mem_init antes de initializeScheduler (que mem_alloc-ea PCBs).
	 * load_idt habilita IRQs (_sti al final), pero schedule() chequea
	 * schedulerEnabled antes de conmutar, así que es seguro. enableScheduler
	 * destraba el cambio de contexto desde el próximo tick. */
	mem_init((void *)MEM_HEAP_START, MEM_HEAP_SIZE);
	initializeScheduler(initProcessEntry);
	load_idt();
	enableScheduler();

	while (1) _hlt();

	return 0;
}
