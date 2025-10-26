.code

EXTERN HandleVmExit : proc
EXTERN g_StackPointerForReturning:QWORD
EXTERN g_BasePointerForReturning:QWORD

; To enable SVM
public EnableSVMOperation
EnableSVMOperation PROC

    push rcx
    push rdx
    push rax
    push rbx  ; Preserve RBX since CPUID modifies it

    mov ecx, 0C0000080h  ; Load MSR index
    cpuid                ; Serialize execution before MSR read
    rdmsr                ; Read MSR (Result in EDX:EAX)
    
    test eax, 1000h      ; Check if SVM is already enabled
    jnz already_enabled  ; If bit is set, skip writing

    or eax, 1000h        ; Set SVM enable bit
    lfence               ; Memory barrier to ensure correct instruction order
    wrmsr                ; Write back to MSR

already_enabled:
    cpuid                ; Serialize execution after MSR modification
    pop rbx
    pop rax
    pop rdx
    pop rcx
    ret

EnableSVMOperation ENDP


AsmSaveState PROC public

    mov g_StackPointerForReturning, rsp
    mov g_BasePointerForReturning, rbp

    ret

AsmSaveState ENDP

AsmRestoreState PROC public

    mov rsp, g_StackPointerForReturning
    mov rbp, g_BasePointerForReturning

AsmRestoreState ENDP

GuestEntry PROC Public

    mov rax, rcx
    
    vmrun rax

    call HandleVmExit

    jmp rsp
    

GuestEntry ENDP

END