X86_Start MACRO
	LOCAL  xx, rt
	call   $+5
	xx     equ $
	mov    dword ptr [rsp + 4], 23h
	add    dword ptr [rsp], rt - xx
	retf
	rt:
ENDM

X86_End MACRO
	db 6Ah, 33h			; push  33h
	db 0E8h, 0, 0, 0, 0		; call  $+5
	db 83h, 4, 24h, 5		; add   dword ptr [esp], 5
	db 0CBh				; retf
ENDM

.data
brsp qword 0
brdi qword 0
brbp qword 0
brax qword 0

.code
_x86call PROC
	mov rax, offset x86start
	jmp x86ret

x86start:
	mov    brsp, rsp
	mov    brdi, rdi
	mov    brbp, rbp
	
	X86_Start

	mov esp, 11111111h
	mov ebp, esp
	db 40h dup(90h)
	db 89h, 04h, 24h ;mov dword ptr [esp], eax

	X86_End

	mov rax, brax
	mov rbp, brbp
	mov rdi, brdi
	mov rsp, brsp

x86ret:
	ret  
_x86call ENDP

_rsp PROC
	mov rax, rsp
	sub rax, 8 ;reserved for switch
	ret
_rsp ENDP

END
