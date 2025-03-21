  LD D, HW           ; load string address to D
LOOP:
  LDR D, A           ; load [D] to A
  INR D              ; increment D
  CPR A, 0           ; compare char with \0 (NUL)
  BRZ END            ; branch if zero/equal
  ST A, byte #F00000 ; load the character to the interrupt argument table
  INT 1              ; output char
  BR LOOP            ; loop
END:
  STP
HW: B "Hello, World!\n\0"
