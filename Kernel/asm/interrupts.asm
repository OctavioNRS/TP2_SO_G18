; interrupts.asm — Stubs en ensamblador para IRQs, excepciones, syscalls y
; primitivas de control de interrupciones (_cli/_sti/_hlt). Cada handler pushea
; el estado, llama al despachador en C correspondiente y restaura con iretq.
; `_syscall` entra desde `int 0x80` y reenvía a `syscallDispatcher`.

GLOBAL _cli
GLOBAL _sti
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt
GLOBAL _forceTimerInt

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler
GLOBAL _syscall

GLOBAL _exception0Handler
GLOBAL _exception6Handler

EXTERN irqDispatcher
EXTERN syscallDispatcher
EXTERN exceptionDispatcher
EXTERN schedule
EXTERN main
EXTERN readPort
EXTERN createRegisterSnapshot

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1 ; pasaje de parametro
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

%macro syscallHandler 0
	push rax
	mov rcx, r10
	call syscallDispatcher ; rdi, rsi, rdx, r10, r8, r9, rax = syscall number
	add rsp, 8
	iretq
%endmacro


%macro exceptionHandler 1
	pushState

	mov rdi, %1 ; pasaje de parametro
	mov rsi, rsp ; stack pointer
	call exceptionDispatcher
	call main

	popState
	iretq
%endmacro


_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret


_sti:
	sti
	ret

picMasterMask:
	push rbp
    mov rbp, rsp
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
    mov     ax, di  ; ax = mascara de 16 bits
    out	0A1h,al
    pop     rbp
    retn


;8254 Timer (Timer Tick) — realiza el cambio de contexto (scheduler)
_irq00Handler:
	pushState

	mov rdi, rsp        ; rdi = stack del proceso saliente
	call schedule       ; rax = stack del proceso entrante
	mov rsp, rax        ; cambio de contexto

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq

;Keyboard
_irq01Handler:
	pushState
	mov rdi, rsp ; stack pointer
	call createRegisterSnapshot
	popState
	irqHandlerMaster 1

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5

;System Call
_syscall:
	syscallHandler


; Fuerza una invocación al scheduler por software (yield / exit)
_forceTimerInt:
	int 20h
	ret


;Zero Division Exception
_exception0Handler:
	exceptionHandler 0

;Invalid Opcode Exception
_exception6Handler:
	exceptionHandler 6

haltcpu:
	cli
	hlt
	ret

SECTION .bss
	aux resq 1
