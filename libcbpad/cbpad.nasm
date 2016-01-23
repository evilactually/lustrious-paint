; global _adder
; global _function
; global _data
; global _callback_pad
global _cb_step
; global _get_callback_pad
global _cb_ptrs
global _cb_data_ptrs
global _CB_PAD_COUNT

; section .data

section .bss

%define CB_PAD_COUNT 1024

; _function: RESD N
; _data:     RESD N

_cb_ptrs:      RESD CB_PAD_COUNT
_cb_data_ptrs: RESD CB_PAD_COUNT
_cb_pad_rets:  RESD CB_PAD_COUNT


section .text

; _adder:
;   mov eax, [esp+4]
;   add eax, [esp+8]
;   ret

; _callback_pad:
;   mov eax, dword [esp]
;   push eax
;   mov eax, [_data]
;   mov dword [esp+4], eax
;   jmp [_function]

; %assign i 0
; %rep CB_PAD_COUNT
; global _pad_postamble_%[i]
; _pad_postamble_%[i]:          ; (esp will be restored by ret)
;   push eax               ; save eax, it's needed to load return address
;   mov eax,[_cb_pad_rets+i] ; load return address into eax
;   mov [esp+4], eax       ; restore return address
;   pop eax                ; restore eax
;   ret                    ; return to caller
; %assign i i+1
; %endrep  

%assign i 0
%rep CB_PAD_COUNT
global _cb_pad_%[i]
_cb_pad_%[i]:                 ; *RET* | ARG_1 | ...
  mov eax, dword [esp]        ; *RET* | ARG_1 | ...
  mov [_cb_pad_rets+i], eax   ; save return address 
  push _cb_pad_postamble_%[i] ; set return to postamble
  mov eax, [_cb_data_ptrs+i]  ; get user data ptr
  mov dword [esp+4], eax      ; push user data ptr as first argument to callback
  jmp [_cb_ptrs+i]              ; jump to user callback
_cb_pad_postamble_%[i]:
  push eax                ; save eax, it's needed to load return address
  mov eax, [_cb_pad_rets] ; load return address into eax
  mov [esp+4], eax        ; restore return address
  pop eax                 ; restore eax
  ret                     ; return to caller
%assign i i+1
%endrep

; _get_callback_pad:
;   mov eax, [esp+4]
;   mul dword [_callback_step]
;   add eax, _callback_pad_0
;   ret

section .data

_cb_step:      dd _cb_pad_1-_cb_pad_0
_CB_PAD_COUNT: dd CB_PAD_COUNT