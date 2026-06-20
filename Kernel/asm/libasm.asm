; libasm.asm — Primitivas en ensamblador llamables desde C:
;   - cpuVendor: CPUID con EAX=0 para obtener el vendor string.
;   - getTime / getDate: lectura del RTC vía puertos CMOS (0x70, 0x71).
;   - readPort / writePort: in/out de 8 bits sobre cualquier puerto.

GLOBAL cpuVendor, getTime, getDate, readPort, writePort

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

getTime:	;paso como parametro punteros para almacenar el valor de segundos, minutos y hora
    push rbp
    mov rbp, rsp

    ; Horas (registro 4)
    mov al, 4
    out 70h, al
    in al, 71h
    mov [rdi], al    ; *horas

    ; Minutos (registro 2)
    mov al, 2
    out 70h, al
    in al, 71h
    mov [rsi], al    ; *minutos

    ; Segundos (registro 0)
    mov al, 0
    out 70h, al
    in al, 71h
    mov [rdx], al    ; *segundos


    mov rsp, rbp
    pop rbp
    ret

getDate:
	push rbp
    mov rbp, rsp

    ; Día de la semana (registro 6)
    mov al, 6
    out 70h, al
    in al, 71h
    mov [rdi], al    ; *weekday

    ; Día del mes (registro 7)
    mov al, 7
    out 70h, al
    in al, 71h
    mov [rsi], al    ; *day

    ; Mes (registro 8)
    mov al, 8
    out 70h, al
    in al, 71h
    mov [rdx], al    ; *month

    ; Año (registro 9)
    mov al, 9
    out 70h, al
    in al, 71h
    mov [rcx], al    ; *year

    mov rsp, rbp
    pop rbp
    ret

readPort:
	push rbp
	mov rbp, rsp
	mov rax, 0
	mov rdx, rdi
	in al, dx
	leave
	ret

writePort:
	push rbp
	mov rbp, rsp
	mov rax, rsi ; valor a escribir
	mov rdx, rdi
	out dx, al
	leave
	ret