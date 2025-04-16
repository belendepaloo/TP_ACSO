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

    ; Guardar argumentos
    mov r12, rdi            ; r12 = list
    movzx r13, sil          ; r13 = type (uint8_t)
    mov r14, rdx            ; r14 = hash (char*)

    ; strlen(hash)
    mov rdi, r14
    call strlen
    mov r8, rax             ; r8 = strlen(hash)
    inc r8                  ; +1 para null terminator

    ; malloc(strlen(hash) + 1)
    mov rdi, r8
    call malloc
    test rax, rax
    jz .error
    mov r15, rax            ; r15 = concat

    ; strcpy(concat, hash)
    mov rdi, r15
    mov rsi, r14
.copy_hash:
    mov al, byte [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    test al, al
    jnz .copy_hash

    ; current_node = list->first
    mov rbx, [r12]          ; rbx = current_node

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
    mov r9, rax

    ; new_size = strlen(concat) + strlen(current_node->hash) + 1
    add r8, r9
    inc r8

    ; malloc(new_size)
    mov rdi, r8
    call malloc
    test rax, rax
    jz .error_cleanup
    mov rdx, rax            ; rdx = new_result

    ; strcpy(new_result, concat)
    mov rdi, rdx
    mov rsi, r15
.copy_concat:
    mov al, byte [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    test al, al
    jnz .copy_concat

    ; strcat(new_result, current_node->hash)
    dec rdi                 ; volver al null terminator
    mov rsi, [rbx + 24]     ; current_node->hash
.copy_hash_append:
    mov al, byte [rsi]
    mov [rdi], al
    inc rsi
    inc rdi
    test al, al
    jnz .copy_hash_append

    ; free(concat) y actualizar concat
    mov rdi, r15
    call free
    mov r15, rdx

.next_node:
    mov rbx, [rbx]          ; current_node = current_node->next
    jmp .loop

.done:
    mov rax, r15            ; return concat
    jmp .end

.error:
    xor rax, rax
    jmp .end

.error_cleanup:
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
