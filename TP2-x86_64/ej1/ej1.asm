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

global string_proc_list_add_node_asm
extern string_proc_node_create_asm

section .text

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
    push r14

    ; Argumentos:
    ; rdi = dst
    ; rsi = src

    ; Guardamos argumentos en registros callee-saved
    mov rbx, rdi        ; dst
    mov r12, rsi        ; src

    ; Chequear si dst o src son NULL
    test rbx, rbx
    jz .fin
    test r12, r12
    jz .fin

    ; src->first == NULL → no hay nada que concatenar
    mov r13, [r12]      ; r13 = src->first
    test r13, r13
    jz .fin

    ; dst->first == NULL → dst está vacío → copiar src directo
    mov r14, [rbx]      ; r14 = dst->first
    test r14, r14
    jnz .append

    ; dst vacío → hacer dst->first = src->first; dst->last = src->last
    mov [rbx], r13          ; dst->first = src->first
    mov r14, [r12 + 8]      ; src->last
    mov [rbx + 8], r14      ; dst->last = src->last
    jmp .clear_src

.append:
    ; conectar las listas:
    ; dst->last->next = src->first
    ; src->first->previous = dst->last

    mov r14, [rbx + 8]      ; r14 = dst->last
    mov [r14], r13          ; dst->last->next = src->first
    mov [r13 + 8], r14      ; src->first->previous = dst->last

    ; dst->last = src->last
    mov r14, [r12 + 8]      ; r14 = src->last
    mov [rbx + 8], r14      ; dst->last = src->last

.clear_src:
    ; src->first = NULL; src->last = NULL
    xor r14, r14
    mov [r12], r14
    mov [r12 + 8], r14

.fin:
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
