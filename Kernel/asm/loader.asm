; loader.asm — Entry point del kernel (símbolo `loader`).
; Pure64 salta acá tras cargar la imagen. `loader` setea el stack en
; `initializeKernelBinary` (que copia módulos y limpia BSS) y salta a `main`
; del kernel.

global loader
extern main
extern initializeKernelBinary

loader:
	call initializeKernelBinary	; Set up the kernel binary, and get thet stack address
	mov rsp, rax				; Set up the stack with the returned address
	call main
hang:
	cli
	hlt	; halt machine should kernel return
	jmp hang
