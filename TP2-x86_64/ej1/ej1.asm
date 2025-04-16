; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
string_proc_list_create_asm:
    mov rdi, 16              ; sizeof(string_proc_list)
    call malloc              ; rax ← puntero a la lista

    test rax, rax            ; ¿malloc falló?
    jz .return_null_create

    mov qword [rax], 0       ; first = NULL
    mov qword [rax+8], 0     ; last = NULL
    ret

.return_null_create:
    xor rax, rax
    ret


global string_proc_node_create_asm
string_proc_node_create_asm:
    mov rdx, rdi             ; type → rdx
    mov rdi, 32              ; malloc(32 bytes)
    call malloc

    test rax, rax
    jz .return_null_node

    mov qword [rax], 0       ; next
    mov qword [rax+8], 0     ; previous

    mov r8b, dl
    mov byte [rax+16], r8b   ; type

    xor rbx, rbx             ; asegurar que esté limpio
    mov rbx, rsi             ; hash
    mov qword [rax+24], rbx  ; guardar hash

    ret

.return_null_node:
    xor rax, rax
    ret





global string_proc_list_add_node_asm
string_proc_list_add_node_asm:
    ; rdi = list
    ; rsi = type
    ; rdx = hash

    ; Llamar a string_proc_node_create_asm
    push rdi
    push rsi
    push rdx

    mov rdi, sil
    mov rsi, rdx
    call string_proc_node_create_asm

    pop rdx
    pop rsi
    pop rdi

    test rax, rax
    jz .return             ; falló la creación del nodo

    mov rcx, rax           ; rcx = new_node

    cmp QWORD [rdi], 0     ; ¿list->first == NULL?
    jne .add_to_end

    ; Lista vacía
    mov [rdi], rcx         ; list->first = new_node
    mov [rdi+8], rcx       ; list->last = new_node
    jmp .return

.add_to_end:
    mov r8, [rdi+8]        ; r8 = list->last
    mov [r8], rcx          ; last->next = new_node
    mov [rcx+8], r8        ; new_node->previous = last
    mov [rdi+8], rcx       ; list->last = new_node

.return:
    ret

global string_proc_list_concat_asm
string_proc_list_concat_asm:
    ; rdi = list
    ; rsi = type
    ; rdx = hash

    ; Copiamos el hash inicial
    mov rdi, rdx
    call strdup            ; strdup(hash)
    test rax, rax
    jz .return_null_concat
    mov r8, rax            ; r8 = result

    mov rcx, [rdi]         ; rcx = list->first

.loop:
    test rcx, rcx
    jz .done

    movzx r9, BYTE [rcx+16]    ; r9 = current->type
    cmp r9b, sil
    jne .next

    ; str_concat(result, current->hash)
    mov rdi, r8
    mov rsi, [rcx+17]          ; current->hash
    call str_concat
    test rax, rax
    jz .fail_concat

    mov r8, rax                ; actualizar resultado

.next:
    mov rcx, [rcx]             ; current = current->next
    jmp .loop

.done:
    mov rax, r8
    ret

.fail_concat:
    mov rdi, r8
    call free
.return_null_concat:
    xor rax, rax
    ret



; FUNCIONES auxiliares que pueden llegar a necesitar:
extern strdup

extern malloc
extern free
extern str_concat


