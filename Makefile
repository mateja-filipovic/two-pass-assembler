all: asembler linker

asembler: main.o assembler.o parser.o
	g++ main.o assembler.o parser.o -o asembler

linker: linker_main.o linker.o
	g++ linker_main.o linker.o -o linker

main.o: src/main.cpp
	g++ -c src/main.cpp

assembler.o: src/assembler.cpp
	g++ -c src/assembler.cpp

parser.o: src/parser.cpp
	g++ -c src/parser.cpp

linker_main.o: src/linker_main.cpp
	g++ -c src/linker_main.cpp

linker.o: src/linker.cpp
	g++ -c src/linker.cpp

clean:
	rm -f *.o asembler linker
