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
extern strdup


string_proc_list_create_asm:
    ; rdi = tamaño (16 bytes)
    mov rdi, 16
    call malloc              ; devuelve puntero en rax

    test rax, rax
    je .return_null

    ; inicializar estructura (dos punteros a NULL)
    mov qword [rax], 0       ; first
    mov qword [rax+8], 0     ; last

    ret

.return_null:
    xor rax, rax
    ret

string_proc_node_create_asm:
    ; rdi = type, rsi = hash
    mov rdx, rdi             ; guardar type
    mov rdi, 32              ; malloc(sizeof(node))
    call malloc
    test rax, rax
    jz .return_null_node

    mov qword [rax], 0       ; next = NULL
    mov qword [rax+8], 0     ; previous = NULL
    mov byte [rax+16], dl    ; type
    mov qword [rax+24], rsi  ; hash
    ret

.return_null_node:
    xor rax, rax
    ret

string_proc_list_add_node_asm:
    ; rdi = list, rsi = type, rdx = hash

    push rdi
    push rsi
    push rdx

    movzx rdi, sil             ; type (como uint8_t)
    mov rsi, rdx             ; hash
    call string_proc_node_create_asm

    pop rdx
    pop rsi
    pop rdi

    test rax, rax
    jz .add_node_return
    mov rcx, rax             ; new_node

    cmp qword [rdi], 0
    jne .add_to_end

    ; Lista vacía
    mov [rdi], rcx           ; list->first = new_node
    mov [rdi+8], rcx         ; list->last = new_node
    jmp .add_node_return

.add_to_end:
    mov r8, [rdi+8]          ; r8 = list->last
    mov [r8], rcx            ; last->next = new_node
    mov [rcx+8], r8          ; new_node->previous = last
    mov [rdi+8], rcx         ; list->last = new_node

.add_node_return:
    ret

string_proc_list_concat_asm:
    ; rdi = list, rsi = type, rdx = hash

    push rdi
    mov rdi, rdx             ; strdup(hash)
    call strdup
    test rax, rax
    jz .return_null_concat
    mov r8, rax              ; resultado acumulado
    pop rdi                  ; restaurar list

    mov rcx, [rdi]           ; current_node = list->first

.loop:
    test rcx, rcx
    jz .done

    movzx r9, byte [rcx+16]  ; current_node->type
    cmp r9b, sil
    jne .next

    mov rdi, r8              ; acumulador
    mov rsi, [rcx+24]        ; current_node->hash
    call str_concat
    test rax, rax
    jz .fail_concat
    mov r8, rax              ; actualizar acumulador

.next:
    mov rcx, [rcx]           ; current_node = current_node->next
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

section .note.GNU-stack noalloc noexec nowrite progbits
