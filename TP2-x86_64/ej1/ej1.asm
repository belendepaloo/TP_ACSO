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
extern strcat
extern realloc

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
    push r14
    push r15

    ; strdup(hash)
    mov rdi, rdx            ; rdx = hash
    call strdup
    test rax, rax
    jz .error

    mov r12, rdi            ; r12 = list
    movzx r13, sil          ; r13 = type
    mov r14, rdx            ; r14 = hash (original)
    mov r15, rax            ; r15 = concat (resultado acumulado)

    ; rbx = list->first
    mov rbx, [r12]          

.loop:
    test rbx, rbx
    jz .done

    ; if (current_node->type == type)
    movzx eax, byte [rbx + 16]   ; offset 16 = type
    cmp al, r13b
    jne .next_node

    ; strlen(concat)
    mov rdi, r15
    call strlen
    mov r8, rax

    ; strlen(current_node->hash)
    mov rdi, [rbx + 24]
    call strlen
    add rax, r8
    inc rax                      ; +1 para null terminator

    ; realloc(concat, total_len)
    mov rdi, r15
    mov rsi, rax
    call realloc
    test rax, rax
    jz .error_cleanup

    mov r15, rax                 ; concat = realloc result

    ; strcat(concat, current_node->hash)
    mov rdi, r15
    mov rsi, [rbx + 24]
    call strcat

.next_node:
    mov rbx, [rbx]               ; avanzar al siguiente nodo
    jmp .loop

.done:
    mov rax, r15                 ; return concat
    jmp .end

.error:
    xor rax, rax                 ; return NULL
    jmp .end

.error_cleanup:
    mov rdi, r15
    call free
    xor rax, rax                 ; return NULL

.end:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret


section .note.GNU-stack noalloc noexec nowrite progbits
