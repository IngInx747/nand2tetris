CPP=g++ -std=c++14
CC=g++ -std=c++14 -c

all: vm_translator vm_translator_single

vm_translator: vm_translator.cpp
	$(CPP) vm_translator.cpp -o vm_translator

vm_translator_single: vm_translator_single.cpp
	$(CPP) vm_translator_single.cpp -o vm_translator_single

clean:
	rm vm_translator vm_translator_single
	