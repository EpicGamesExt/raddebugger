; Copyright (c) 2024 Epic Games Tools
; Licensed under the MIT license (https://opensource.org/license/mit/)
; $ c:\devel\projects\bin\win32\nasm src\pe\dos_program.asm -fbin -o dos_program.bin

BITS 16

SEGMENT CODE
 push cs         ; copy psp segment address to ds
 pop  ds
 mov  dx, msg    ; set print string
 mov  ah, 9h     ; print to stdout
 int  21h
 mov  ax, 0x4c01 ; terminate with return code 1 in al
 int  0x21

msg: DB "This program cannot be run in DOS mode.$",0
ALIGN 8, DB
