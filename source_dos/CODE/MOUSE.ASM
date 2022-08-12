;***************************************************************************/
;*                                                                         */
;*                                                                         */
;* Raven 3D Engine                                                         */
;* Copyright (C) 1995 by Softdisk Publishing                               */
;*                                                                         */
;* Original Design:                                                        */
;*  John Carmack of id Software                                            */
;*                                                                         */
;* Enhancements by:                                                        */
;*  Robert Morgan of Channel 7............................Main Engine Code */
;*  Todd Lewis of Softdisk Publishing......Tools,Utilities,Special Effects */
;*  John Bianca of Softdisk Publishing..............Low-level Optimization */
;*  Carlos Hasan..........................................Music/Sound Code */
;*                                                                         */
;*                                                                         */
;***************************************************************************/

.386
.MODEL flat


SCREEN        = 0a0000h
SCREENWIDTH   = 320
SCREENHEIGHT  = 200
MOUSESIZE     = 16


	.DATA

EXTRN  hiding: DWORD
EXTRN  busy: DWORD
EXTRN  mousex: DWORD
EXTRN  mousey: DWORD
EXTRN  fore: DWORD
EXTRN  back: DWORD


	.CODE


public MouseHide

MouseHide proc near
  cli
  cmp [busy], 0
  je @@checkhide
  sti
  ret

@@checkhide:
  cmp [hiding], 0
  je @@mcontinue
  inc [hiding]
  sti
  ret

@@mcontinue:
  mov [busy], 1
  sti

  push esi
  push edi
  push ebx
  push ebp

  mov esi, offset back
  mov edx, [mousex]
  cmp edx, 319-MOUSESIZE
  jg  @@overflowx
  mov edx, MOUSESIZE
  jmp @@nooverflow
@@overflowx:
  mov edx, SCREENWIDTH
  sub edx, [mousex]
@@nooverflow:
  mov ebp, [mousey]
  cmp ebp, 199-MOUSESIZE
  jg  @@overflowy
  mov ebp, MOUSESIZE
  jmp @@drawset
@@overflowy:
  mov ebp, SCREENHEIGHT
  sub ebp, [mousey]
@@drawset:
  mov ebx, ebp
  mov eax, [mousey]
  imul edi, eax, 320
  add edi, [mousex]
  add edi, SCREEN
@@loop:
   mov ecx, edx
   rep movsb
   add edi, SCREENWIDTH
   sub edi, edx
   dec ebx
  jnz @@loop

  pop ebp
  pop ebx
  pop edi
  pop esi

  mov [hiding], 1
  mov [busy], 0
  ret

 endp


;==========================================================================

public MouseShow

MouseShow proc near
  cli
  cmp [busy], 0
  je @@checkhide2
  sti
  ret

@@checkhide2:
  cmp [hiding], 1
  je @@notshown2
  dec [hiding]
  sti
  ret

@@notshown2:
  mov [busy], 1
  sti

  push esi
  push edi
  push ebx
  push ebp

  mov edx, [mousex]
  mov ebx, edx
  mov ebp, [mousey]
  mov ecx, ebp
  mov edi, offset back
  cmp edx, 319-MOUSESIZE
  jg @@overflowx2
  mov edx, MOUSESIZE
  jmp @@nooverflow2
@@overflowx2:
  mov edx, SCREENWIDTH
  sub edx, ebx
@@nooverflow2:
  cmp ebp, 199-MOUSESIZE
  jg @@overflowy2
  mov ebp, MOUSESIZE
  jmp @@drawset2
@@overflowy2:
  mov ebp, SCREENHEIGHT
  sub ebp, ecx
@@drawset2:
  mov eax, ecx
  imul esi, eax, 320
  add esi, ebx
  add esi, SCREEN
  mov ebx, ebp
@@loop2:
   mov ecx, edx
   rep movsb
   add esi, SCREENWIDTH
   sub esi, edx
   dec ebx
  jnz @@loop2
  mov ebx, ebp
  mov esi, offset fore
  mov eax, [mousey]
  dec eax
  imul edi, eax, 320
  add edi, [mousex]
  add edi, SCREEN
  add edi, edx
  sub esi, MOUSESIZE
  add esi, edx
@@loopy2:
  add edi, SCREENWIDTH
  sub edi, edx
  mov ecx, edx
  add esi, MOUSESIZE
  sub esi, edx
@@loopx2:
   mov al, [esi]
   cmp al, 255
   je @@mcontinue2
   mov [edi], al
@@mcontinue2:
   inc edi
   inc esi
   dec ecx
   jnz @@loopx2
  dec ebx
  jnz @@loopy2

  pop ebp
  pop ebx
  pop edi
  pop esi

  mov [hiding], 0
  mov [busy], 0

  ret

MouseShow endp

END
