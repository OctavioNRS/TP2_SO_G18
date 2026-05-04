GLOBAL sys_read, sys_write, sys_draw_rect, sys_draw_circle, sys_draw_char
GLOBAL sys_screen_width, sys_screen_height, sys_background, sys_video_aux, sys_render_aux
GLOBAL sys_sleep, sys_draw_string, sys_get_key, sys_play_sound, sys_date, sys_time, sys_kernel_time
GLOBAL test_invalid_opcode, sys_print_registers
section .text

%macro syscall 1
	push rbp
	mov rbp, rsp

	mov rax, %1
	mov r10, rcx
	int 0x80

	leave
	ret
%endmacro

sys_read:
	syscall 0

sys_write:
	syscall 1

sys_draw_rect:
	syscall 2

sys_draw_circle:
	syscall 3

sys_draw_char:
	syscall 4

sys_screen_width:
	syscall 5

sys_screen_height:
	syscall 6

sys_background:
	syscall 7

sys_video_aux:
	syscall 8

sys_render_aux:
	syscall 9
	
sys_sleep:
	syscall 10

sys_draw_string:
	syscall 11

sys_get_key:
	syscall 12

sys_play_sound:
	syscall 13

sys_date:
	syscall 14

sys_time:
	syscall 15

sys_kernel_time:
	syscall 16

sys_print_registers:
	syscall 17

test_invalid_opcode:
	int 0x06