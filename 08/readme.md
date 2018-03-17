Make:
make all

Usage:
./vm_translator_single single/file > single/target
./vm_translator your/source/files/directory/file0 your/source/files/directory/file1 .. > your/source/files/directory/target.asm

For your convenience to test multifiles:

FunctionCalls:
./vm_translator_single FunctionCalls/SimpleFunction/SimpleFunction.vm > FunctionCalls/SimpleFunction/SimpleFunction.asm
./vm_translator FunctionCalls/FibonacciElement/Sys.vm FunctionCalls/FibonacciElement/Main.vm > FunctionCalls/FibonacciElement/FibonacciElement.asm
./vm_translator FunctionCalls/NestedCall/Sys.vm > FunctionCalls/NestedCall/NestedCall.asm 
./vm_translator FunctionCalls/StaticsTest/Sys.vm FunctionCalls/StaticsTest/Class1.vm FunctionCalls/StaticsTest/Class2.vm > FunctionCalls/StaticsTest/StaticsTest.asm

ProgramFlow:
./vm_translator_single ProgramFlow/BasicLoop/BasicLoop.vm > ProgramFlow/BasicLoop/BasicLoop.asm
./vm_translator_single ProgramFlow/FibonacciSeries/FibonacciSeries.vm > ProgramFlow/FibonacciSeries/FibonacciSeries.asm 

Important:
I noticed that you concern about efficiency of asm which relates with number of assemblers directly. I have to defend myself that I add function to make comment in asm files to debug easily and make asm lines more readable.
Additionally you must notice my assembler of "pop xxx n" seems longer than it should be. That is because "pop" needs remember 2 parameters, one of which is offset to certain reserved register, in which case D register cannot help to store both of and one must use a third register to remark. Given the fact it is common for pop to pop only 1 time, it is not necessary to use the third register as one can simply write "@SP\nM=M-1\nA=M\nD=M\n@THAT\nA=M\n" and repeat "A=A+1\n" n times with "M=D\n". Usually n is small so my assembler looks longer.
I did not change my code based on what listed above because my original approach is general to all "push/pop" and I don't want to add complixity to my code. If I do so, I have to add more than 10 additional functions with ugly loops.
Hope you could understand the fact my assembler looks a litte longer than it can be when you grade. Anyway 20% lost is too heavy to carry!

P.s. vm_translator_single.cpp is inherited from pj07. As it is named, it solves cases of single source code which are included in ./ProgramFlow.