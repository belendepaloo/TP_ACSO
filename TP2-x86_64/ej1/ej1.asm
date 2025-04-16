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
    push r8
    push r9
    push r10
    push r11

    ; Guardar argumentos: list = rdi, type = sil, hash = rdx
    mov r8, rdi        ; r8 ← list
    movzx r9, sil      ; r9 ← type como entero (uint64)
    mov r10, rdx       ; r10 ← hash

    ; Llamar a string_proc_node_create_asm
    mov dil, r9b       ; type → dil
    mov rsi, r10       ; hash → rsi
    call string_proc_node_create_asm

    ; Si malloc falló → retornar
    test rax, rax
    jz .done

    ; rax ← nodo nuevo
    ; r8 ← list

    ; Verificar si lista vacía con TEST
    test qword [r8], [r8]
    setz r11b                ; r11b ← 1 si list->first == NULL, 0 si no

    ; Condicional: si lista vacía → first = node
    test r11b, r11b
    jz .not_empty

    ; lista vacía
    mov [r8], rax        ; list->first = node
    mov [r8 + 8], rax    ; list->last = node
    jmp .done

.not_empty:
    ; lista no vacía
    mov r10, [r8 + 8]     ; r10 ← list->last
    mov [r10], rax        ; list->last->next = node
    mov [rax + 8], r10    ; node->previous = list->last
    mov [r8 + 8], rax     ; list->last = node

.done:
    pop r11
    pop r10
    pop r9
    pop r8
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
