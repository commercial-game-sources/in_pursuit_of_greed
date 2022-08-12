ideal
p386
model flat,c

extrn   scode:byte

global kisr:near


SEGMENT kbdriver USE32



proc kisr near
 pushad
 push es

 mov     ax, DGROUP
 mov     es, ax
 mov     ebx, offset scode

 in      al,60h
 cmp     al,7fh
 ja      kb_release
 mov     [es:ebx], al
 jmp     xit_kisr
kb_release:
 mov     [byte es:ebx], 0

xit_kisr:
 in      al, 61h             ;read in 8255 port PB
 or      al, 80h             ;set keybrd acknowledge
 out     61h, al             ;send it
 and     al, 7fh             ;reset it
 out     61h, al             ;restore original condition

 mov     dx, 20h
 mov     al, dl              ;end the interrput
 out     dx, al

 pop es
 popad
 iretd
endp
 
ends kbdriver
end

