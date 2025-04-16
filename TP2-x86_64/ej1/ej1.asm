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
extern strdup

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
    push rcx
    push r8
    push r9
    push r10

    mov rbx, rdi         ; rbx = list
    mov r12d, esi        ; r12d = type
    mov rcx, rdx         ; rcx = prefix

    ; Verificar si la lista es NULL
    cmp rbx, 0
    je .ret_null

    ; Obtener longitud de prefix
    mov rdi, rcx
    call strlen
    mov r8, rax          ; r8 = len_prefix

    ; Reservar memoria para string inicial
    lea rdi, [r8 + 1]
    call malloc
    mov r9, rax          ; r9 = result
    cmp r9, 0
    je .ret_null

    ; Copiar prefix a result
    mov rdi, r9
    mov rsi, rcx
    call strcpy

    ; Empezar recorrido de la lista
    mov r10, [rbx]       ; r10 = current = list->first

.loop_start:
    cmp r10, 0
    je .done

    movzx eax, byte [r10 + 16]  ; type del nodo

    ; Comparar con type buscado
    xor edx, edx
    test eax, eax
    setz dl               ; dl = 1 si eax == 0
    cmp eax, r12d
    jne .skip_node        ; si no es del tipo deseado, saltar

    ; Concatenar string
    mov rdi, r9           ; rdi = destino
    mov rsi, [r10 + 24]   ; rsi = nodo->hash
    call str_concat
    test rax, rax
    je .skip_node

    ; Liberar viejo string
    mov rdi, r9
    mov r9, rax
    call free

.skip_node:
    mov r10, [r10]        ; avanzar a siguiente nodo
    jmp .loop_start

.done:
    mov rax, r9
    jmp .end

.ret_null:
    xor eax, eax

.end:
    pop r10
    pop r9
    pop r8
    pop rcx
    pop r12
    pop rbx
    leave
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
