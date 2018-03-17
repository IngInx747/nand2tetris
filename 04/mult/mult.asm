// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)

// Put your code here.

// R[2] = 0;
// i = 0;
// 
// while (i < R[1]) {
// 	R[2] += R[0];
// 	i++;
// }

@2
M=0
@i
M=0

@1
D=M
@END
D;JEQ

(LOOP)

@i
D=M
@1
D=D-M
@END
D;JEQ

@0
D=M
@2
M=D+M

@i
M=M+1

@LOOP
0;JMP

(END)
@END
0;JMP
