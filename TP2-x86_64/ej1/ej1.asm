; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat
extern strcpy
extern strlen

string_proc_list_create_asm:
    push rdi
    mov rdi, 16
    call malloc
    test rax, rax
    je .error_malloc
    mov qword [rax], 0
    mov qword [rax+8], 0

    pop rdi
    ret

.error_malloc:
    xor rax, rax
    pop rdi
    ret


string_proc_node_create_asm:
    push rbx
    push r12

    mov rbx, rdx
    movzx r12, sil

    mov rdi, 32
    call malloc
    test rax, rax
    je .error_malloc

    xor rcx, rcx
    mov [rax], rcx
    mov [rax + 8], rcx
    mov byte [rax + 16], r12b
    mov [rax + 24], rbx

.return:
    pop r12
    pop rbx
    ret

.error_malloc:
    xor rax, rax
    jmp .return


string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r14
    push r15

    mov rbx, rdi
    mov r14b, sil
    mov r15, rdx

    movzx rsi, r14b
    mov rdx, r15
    call string_proc_node_create_asm
    test rax, rax
    jz .return

    cmp qword [rbx], 0
    je .init_list

    mov rcx, [rbx + 8]
    mov [rcx], rax
    mov [rax + 8], rcx
    mov [rbx + 8], rax
    jmp .return

.init_list:
    mov [rbx], rax
    mov [rbx + 8], rax

.return:
    pop r15
    pop r14
    pop rbx
    leave
    ret

string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14          ; nuevo registro: dst->last
    push r15          ; nuevo registro: src->first

    ; Guardar parámetros
    mov rbx, rdi       ; dst
    mov r12, rsi       ; src

    ; Si src o src->first es NULL, salir
    mov r15, [r12]     ; r15 = src->first
    test r15, r15
    jz .fin

    ; Si dst->first es NULL, simplemente copiar los punteros de src
    mov r14, [rbx]     ; r14 = dst->first
    test r14, r14
    jnz .concatenar

    ; dst está vacía, se copia src entera
    mov rax, [r12]       ; src->first
    mov [rbx], rax       ; dst->first = src->first
    mov rax, [r12 + 8]   ; src->last
    mov [rbx + 8], rax   ; dst->last = src->last
    jmp .limpiar_src

.concatenar:
    ; dst->last = [rbx + 8], src->first = r15
    mov r14, [rbx + 8]    ; r14 = dst->last
    mov [r14], r15        ; dst->last->next = src->first
    mov [r15 + 8], r14    ; src->first->prev = dst->last

    mov rax, [r12 + 8]    ; rax = src->last
    mov [rbx + 8], rax    ; dst->last = src->last

.limpiar_src:
    mov qword [r12], 0      ; src->first = NULL
    mov qword [r12 + 8], 0  ; src->last = NULL

.fin:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret




section .note.GNU-stack noalloc noexec nowrite progbits
