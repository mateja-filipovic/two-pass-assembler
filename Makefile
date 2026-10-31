asembler: main.o assembler.o parser.o
	g++ main.o assembler.o parser.o -o asembler

main.o: src/main.cpp
	g++ -c src/main.cpp

assembler.o: src/assembler.cpp
	g++ -c src/assembler.cpp

parser.o: src/parser.cpp
	g++ -c src/parser.cpp

clean:
	rm -f *.o asembler
