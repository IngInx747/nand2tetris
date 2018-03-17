
CC=g++ -std=c++14
TC=g++ -Wall -g -std=c++14 -pthread
CL=g++ -std=c++14 -c
RM=/bin/rm -r

all: compiler

compiler: compiler.cpp
	$(CC) compiler.cpp -o compiler

xml: compiler_xml.cpp
	$(CC) compiler_xml.cpp -o xml

clean:
	$(RM) compiler xml
	