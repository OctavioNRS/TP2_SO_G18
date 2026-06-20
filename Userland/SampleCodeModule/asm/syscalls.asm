GLOBAL sys_read, sys_write, sys_draw_rect, sys_draw_circle, sys_draw_char
GLOBAL sys_screen_width, sys_screen_height, sys_background, sys_video_aux, sys_render_aux
GLOBAL sys_sleep, sys_draw_string, sys_get_key, sys_play_sound, sys_date, sys_time, sys_kernel_time
GLOBAL test_invalid_opcode, sys_print_registers
GLOBAL sys_malloc, sys_free, sys_mem_status
GLOBAL sys_getpid, sys_create_process, sys_kill, sys_nice, sys_block, sys_yield, sys_waitpid, sys_ps
GLOBAL sys_exit, sys_unblock
GLOBAL sys_sem_open, sys_sem_close, sys_sem_wait, sys_sem_post
GLOBAL sys_pipe, sys_close, sys_dup, sys_create_process_with_fds, sys_foreground, sys_reopen, sys_scroll_up
GLOBAL sys_reap_zombies
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
sys_malloc:
	syscall 18
sys_free:
	syscall 19
sys_mem_status:
	syscall 20
sys_getpid:
	syscall 21
sys_create_process:
	syscall 22
sys_kill:
	syscall 23
sys_nice:
	syscall 24
sys_block:
	syscall 25
sys_yield:
	syscall 26
sys_waitpid:
	syscall 27
sys_ps:
	syscall 28
sys_exit:
	syscall 29
sys_unblock:
	syscall 30
sys_sem_open:
	syscall 31
sys_sem_close:
	syscall 32
sys_sem_wait:
	syscall 33
sys_sem_post:
	syscall 34
sys_pipe:
	syscall 35
sys_close:
	syscall 36
sys_dup:
	syscall 37
sys_create_process_with_fds:
	syscall 38
sys_foreground:
	syscall 39
sys_reopen:
	syscall 40
sys_scroll_up:
	syscall 41
sys_reap_zombies:
	syscall 42

test_invalid_opcode:
	int 0x06
