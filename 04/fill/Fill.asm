// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Fill.asm

// Runs an infinite loop that listens to the keyboard input.
// When a key is pressed (any key), the program blackens the screen,
// i.e. writes "black" in every pixel;
// the screen should remain fully black as long as the key is pressed. 
// When no key is pressed, the program clears the screen, i.e. writes
// "white" in every pixel;
// the screen should remain fully clear as long as no key is pressed.

// Put your code here.

(RESTART)
@SCREEN
D=A
@0
M=D // put screen start location in RAM[0]

(KBDCHECK)

@KBD // 0x6000
D=M
@BLACK
D;JGT // jump to BLACK if any kbd keys are pressed
@WHITE
D;JEQ // else jump to WHITE

@KBDCHECK
0;JMP

(BLACK)
@1
M=-1 // black = -1 = 11111111111111
@CHANGE
0;JMP

(WHITE)
@1
M=0 // what to fill screen with
@CHANGE
0;JMP

(CHANGE)
@1 // check what to fill screen with
D=M // D contains black or white

@0
A=M	// get adress of screen pixel to fill
M=D	// fill it

@0
D=M+1	// inc to next pixel
@KBD
D=A-D	// kbd(0x6000) - screen(0x4000 - 0x5FFF) = D > 0

@0
M=M+1	// M moves to address of next pixel (OR kbd at last)
A=M

@CHANGE
D;JGT	// if M moves to 0x6000 then stop

@RESTART
0;JMP
