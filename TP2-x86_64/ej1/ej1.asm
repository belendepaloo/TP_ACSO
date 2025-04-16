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

; string_proc_list_concat(list, type, hash)
; rdi = list
; sil = type (uint8_t)
; rdx = hash

section .text
    global string_proc_list_concat_asm
    extern strdup, strlen, realloc, strcat, fprintf, free, stderr

string_proc_list_concat_asm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; strdup(hash)
    mov rdi, rdx        ; strdup(hash)
    call strdup
    test rax, rax
    jz .error_strdup

    mov r12, rdi        ; r12 = list
    movzx r13, sil      ; r13 = type
    mov r14, rdx        ; r14 = original hash
    mov r15, rax        ; r15 = concat (current string)

    ; current_node = list->first
    mov rbx, [r12]      ; rbx = current_node

.loop:
    test rbx, rbx
    jz .done

    ; if (current_node->type == type)
    movzx eax, byte [rbx]       ; current_node->type
    cmp al, r13b
    jne .next_node

    ; strlen(concat)
    mov rdi, r15
    call strlen
    mov r8, rax                ; strlen(concat)

    ; strlen(current_node->hash)
    mov rdi, [rbx + 8]
    call strlen
    add rax, r8
    inc rax                   ; +1 for '\0'

    ; realloc(concat, new_len)
    mov rdi, r15              ; old ptr
    mov rsi, rax              ; new size
    call realloc
    test rax, rax
    jz .error_realloc

    mov r15, rax              ; concat = new_result

    ; strcat(concat, current_node->hash)
    mov rdi, r15              ; dest
    mov rsi, [rbx + 8]        ; src = current_node->hash
    call strcat

.next_node:
    ; current_node = current_node->next
    mov rbx, [rbx + 16]
    jmp .loop

.done:
    mov rax, r15              ; return concat
    jmp .end

.error_strdup:
    ; fprintf(stderr, ...)
    mov rdi, [rel stderr]
    mov rsi, err_strdup_msg
    call fprintf
    xor rax, rax
    jmp .end

.error_realloc:
    mov rdi, [rel stderr]
    mov rsi, err_realloc_msg
    call fprintf

    mov rdi, r15
    call free

    xor rax, rax

.end:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret





section .note.GNU-stack noalloc noexec nowrite progbits
