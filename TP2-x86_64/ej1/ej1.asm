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
    ; Prologue con stack frame (estilo distinto)
    push rbp
    mov rbp, rsp
    push rbx
    push r14
    push r15

    ; Guardar argumentos
    mov rbx, rdi        ; rbx ← list
    mov r14b, sil       ; r14b ← type
    mov r15, rdx        ; r15 ← hash

    ; Preparar llamada a string_proc_node_create_asm(type, hash)
    movzx rsi, r14b     ; rsi ← type (uint8_t)
    mov rdx, r15        ; rdx ← hash
    call string_proc_node_create_asm
    test rax, rax
    jz .done            ; si node == NULL, salir

    ; si list->first == NULL (lista vacía)
    cmp qword [rbx], 0
    je .init_list

    ; lista no vacía → enlazar al final
    mov rcx, [rbx + 8]      ; rcx ← list->last
    mov [rcx], rax          ; rcx->next = node
    mov [rax + 8], rcx      ; node->previous = rcx
    mov [rbx + 8], rax      ; list->last = node
    jmp .done

.init_list:
    ; lista vacía → first y last apuntan al nuevo nodo
    mov [rbx], rax          ; list->first = node
    mov [rbx + 8], rax      ; list->last = node

.done:
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
    
    mov rbx, rdi           
    mov r12d, esi           
    mov r13, rdx           
    

    test rbx, rbx
    jz .return_null
    
    mov rdi, r13
    call strlen
    mov r14, rax           
    
    lea rdi, [r14 + 1]
    call malloc
    mov r15, rax           
    test r15, r15
    jz .return_null
    
    mov rdi, r15
    mov rsi, r13
    call strcpy
    
    mov r14, [rbx]          
.loop:
    test r14, r14
    jz .done
    
    movzx eax, byte [r14 + 16]
    cmp eax, r12d
    jne .next_node
    
    mov rdi, r15
    mov rsi, [r14 + 24]     
    call str_concat
    test rax, rax
    jz .next_node
    
    mov rdi, r15
    mov r15, rax
    call free
    
.next_node:
    mov r14, [r14]         
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
