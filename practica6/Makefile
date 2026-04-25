CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic

all: validator

validator: parser.tab.cpp lex.yy.cpp ast.h
	$(CXX) $(CXXFLAGS) -o $@ parser.tab.cpp lex.yy.cpp

parser.tab.cpp parser.tab.hpp: parser.y ast.h
	bison -d -o parser.tab.cpp parser.y

lex.yy.cpp: lexer.l parser.tab.hpp ast.h
	flex -o lex.yy.cpp lexer.l

clean:
	rm -f validator parser.tab.cpp parser.tab.h parser.tab.hpp lex.yy.cpp

.PHONY: all clean
