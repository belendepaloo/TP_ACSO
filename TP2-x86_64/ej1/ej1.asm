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
    push r13
    push r14
    push r15

    mov rbx, rdi        ; list
    mov r12d, esi       ; type
    mov r13, rdx        ; prefix

    ; Check list != NULL
    test rbx, rbx
    jz .return_null

    ; strlen(prefix)
    mov rdi, r13
    call strlen
    mov r14, rax

    ; malloc(len + 1)
    lea rdi, [r14 + 1]
    call malloc
    mov r15, rax
    test r15, r15
    jz .return_null

    ; strcpy(result, prefix)
    mov rdi, r15
    mov rsi, r13
    call strcpy

    ; current = list->first
    mov r13, [rbx]

.loop:
    cmp r13, 0
    je .done

    ; if (node->type == type)
    movzx eax, byte [r13 + 16]
    cmp eax, r12d
    jne .next

    ; str_concat(result, node->hash)
    mov rdi, r15
    mov rsi, [r13 + 24]
    call str_concat
    test rax, rax
    jz .next

    ; free(old), result = new
    mov rdi, r15
    mov r15, rax
    call free

.next:
    mov r13, [r13]   ; node = node->next
    jmp .loop

.done:
    mov rax, r15
    jmp .return

.return_null:
    xor eax, eax

.return:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    leave
    ret



section .note.GNU-stack noalloc noexec nowrite progbits
