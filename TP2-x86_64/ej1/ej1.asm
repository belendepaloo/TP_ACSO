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
    ; ==== Prologue ====
    push rbx
    push r12
    push r13

    ; ==== Guardar argumentos ====
    mov rbx, rdi         ; rbx ← list
    movzx r12, sil       ; r12 ← type extendido (guardado)
    mov r13, rdx         ; r13 ← hash

    ; ==== Llamar a string_proc_node_create_asm(type, hash) ====
    ; Cargar argumentos como espera string_proc_node_create_asm:
    ; - dil ← type (uint8_t)
    ; - rsi ← hash
    mov dil, r12b
    mov rsi, r13
    call string_proc_node_create_asm
    test rax, rax
    jz .end              ; si node == NULL → return

    test rbx, rbx
    jz .end              ; si list == NULL → return

    ; ==== rax contiene el nuevo nodo ====
    ; node->next = NULL
    xor r13, r13
    mov [rax], r13       ; node->next = NULL

    ; node->previous = list->last
    mov r13, [rbx + 8]
    mov [rax + 8], r13

    ; if (list->last) list->last->next = node
    test r13, r13
    jz .no_last

    mov [r13], rax       ; list->last->next = node
    jmp .set_last

.no_last:
    mov [rbx], rax       ; list->first = node

.set_last:
    mov [rbx + 8], rax   ; list->last = node

.end:
    pop r13
    pop r12
    pop rbx
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
