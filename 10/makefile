
CC=g++ -std=c++14
TC=g++ -Wall -g -std=c++14 -pthread
CL=g++ -std=c++14 -c
RM=/bin/rm -r

all: compiler

compiler: compiler.cpp
	$(CC) compiler.cpp -o compiler

compiler_raw: compiler_raw.cpp
	$(CC) compiler_raw.cpp -o compiler_raw

clean:
	$(RM) compiler compiler_raw
	