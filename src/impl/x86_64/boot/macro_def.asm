; ; Macro is a single Instruction that expands into a predefined
; ; set of instructions to perform a particular task. 

; ; When the program is compiled the single Instructions
; ; get automaitcally converted into the lines of code 
; ; in the block to perform the task of the macro instruction.

; ;Template:
; ;        %macro <name> <argc>
; ;            ...
; ;            <macro body>
; ;            ...
; ;        %endmacro


; section .text
; O_RDONLY    equ 0
; O_WRONLY    equ 1
; O_RDWR      equ 2
; O_CREAT     equ 64
; O_APPEND    equ 1024
; O_DIRECT    equ 16384
; O_DIRECTORY equ 65536
; O_NOFOLLOW  equ 131072
; O_NOATIME   equ 262144
; O_CLOEXEC   equ 524288
; O_SYNC      equ 1052672
; O_PATH      equ 2097152
; O_TMPFILE   equ 4259840 


; ; File handle - Read user input
; STDIN       equ 0
; ; Write output to file handle
; STDOUT      equ 1
; ; Write diagnostic output to file handle
; STDERR      equ 2


; ; Read from FIle Directory
; SYS_READ    equ 0
; ; Write to File Directory
; SYS_WRITE   equ 1 
; ; Open File Directory
; SYS_OPEN    equ 2
; ; Close File Directory
; SYS_CLOSE   equ 3
; ; Copy a File Descriptor
; SYS_DUP     equ 32
; ; Exit current Thread
; SYS_EXIT    equ 60
; ; Change a Directory Path
; SYS_CHDIR   equ 80
; ; Rename a File
; SYS_RENAME  equ 82
; ; Make a Directory
; SYS_MKDIR   equ 83
; ; Delete a Directory
; SYS_RMDIR   equ 84
; ; Permissions - File Access
; SYS_CHMOD   equ 90

