
; FATAL DESIGN FLAW: I can't deal with calling convetions that require a particular alignment
; Alignment Requirement -> Need be abble to shift all function argument -> Need to know exact argument types (I can't)
; Requiring user to TELL ME how big are all his arguments put together is really awkward.
; I'm retiring this library, in favor of a library of wrapper callbacks for specific functions that I need

global cb_cdecl_step
global cb_cdecl_rbr_step
global cb_ptrs
global cb_data_ptrs
global cb_argcs
global CB_PAD_COUNT

section .bss

%define PAD_COUNT 5

cb_ptrs:      RESD PAD_COUNT
cb_argcs:     RESD PAD_COUNT
cb_data_ptrs: RESD PAD_COUNT
cb_pad_rets:  RESD PAD_COUNT

section .text

; volitile  EAX, ECX, and EDX

%assign i 0
%rep PAD_COUNT
global cb_pad_cdecl_%[i]

; Callback pad pushes user data as first argument, and as a result bumps
; ESP register. ESP will need to be restored later by postamble routine.
cb_pad_cdecl_%[i]:
  mov eax, dword [esp]             ; load return address
  mov [cb_pad_rets+i*4], eax       ; save return address 
  push cb_pad_postamble_cdecl_%[i] ; replace return address with postamble
  mov eax, [cb_data_ptrs+i*4]      ; get user data ptr
  mov dword [esp+4], eax           ; push user data ptr as first argument to callback
  jmp [cb_ptrs+i*4]                ; jump to user callback

; Postamble restores ESP state and jumps to stored return address
cb_pad_postamble_cdecl_%[i]:       
  mov ecx, [cb_pad_rets+i*4]       ; use volitile ecx register
  mov [esp], ecx                   ; restore return address
  ret                              ; return to caller
%assign i i+1
%endrep

%assign i 0
%rep PAD_COUNT
global cb_pad_cdecl_rbr_%[i]

; This pad is used for functions that return value by reference.
cb_pad_cdecl_rbr_%[i]:
  mov eax, dword [esp]       ; load return address
  mov [cb_pad_rets+i], eax   ; save return address 
  mov eax, [esp+4]           ; move return reference up
  mov [esp], eax             ; 
  push cb_pad_postamble_cdecl_rbr_%[i] ; replace return address with postamble
  mov eax, [cb_data_ptrs+i]  ; get user data ptr
  mov dword [esp+8], eax     ; push user data ptr as *second* argument, following return reference
  jmp [cb_ptrs+i]            ; jump to user callback

; Postamble restores ESP state and jumps to stored return address
cb_pad_postamble_cdecl_rbr_%[i]:       
  mov ecx, [esp]             ; move down return reference (ecx is volitile)
  mov [esp+4], ecx           ;
  mov ecx, [cb_pad_rets+i]   ; load stored return address
  mov [esp], ecx             ; restore return address
  ret                        ; return to caller
%assign i i+1
%endrep

section .data

cb_cdecl_step:      dd cb_pad_cdecl_1-cb_pad_cdecl_0
cb_cdecl_rbr_step:  dd cb_pad_cdecl_rbr_1-cb_pad_cdecl_rbr_0
CB_PAD_COUNT: dd PAD_COUNT