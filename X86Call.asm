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
	db 6Ah, 33h			    ; push  33h
	db 0E8h, 0, 0, 0, 0		; call  $+5
	db 83h, 4, 24h, 5		; add   dword ptr [esp], 5
	db 0CBh				    ; retf
ENDM

.data
brsp qword 0
brdi qword 0
brbp qword 0
brax qword 0

.code
_x86call PROC
	push rsp
	push rdi
	push rbp
	
	X86_Start

	;PUSH EBP
	db 55h
	mov ebp, esp

	;db 0cch
	;SUB ESP,DWORD PTR SS:[ESP-8]
	db 2bh, 64h, 24h, 0f8h

	;CALL DWORD PTR SS:[ESP-4]
	db 0ffh, 54h, 24h, 0fch

	mov esp, ebp
	;PUSH EBP
	db 5Dh

	X86_End

	pop rbp
	pop rdi
	pop rsp

	ret  
_x86call ENDP

_x86push PROC
	;RCX、RDX、R8、R9
	;XMM0、XMM1、XMM2、XMM3

	sub rsp, rcx
	mov dword ptr [rsp], edx
	add rsp, rcx
	ret
_x86push ENDP

END
