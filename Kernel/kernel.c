#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <videoDriver.h>
#include <keyboardDriver.h>
#include <idtLoader.h>
#include <interrupts.h>
#include <memoryManager.h>
#include <scheduler.h>

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

int main()
{
	load_idt();          /* configura IDT + PIC y habilita interrupciones (_sti) */
	_cli();              /* deshabilito mientras armo memoria y procesos          */

	mem_init((void *)MEM_HEAP_START, MEM_HEAP_SIZE);

	initScheduler();
	/* El primer (y único) proceso de userland es el módulo cargado en 0x400000. */
	createProcess((ProcessEntry)sampleCodeModuleAddress, 0, 0, "userland");

	enableScheduler();   /* habilita el cambio de contexto recién ahora */
	_sti();              /* el primer tick del timer arranca el scheduling */
	while (1) _hlt();

	return 0;
}
