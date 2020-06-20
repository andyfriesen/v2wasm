CXX=clang++
# LDFLAGS= -s
# CFLAGS= -O -g
CXXFLAGS=-std=c++14 -g
# CFLAGS= -o3 -m486     #GCC
#CFLAGS= -o6 -mpentium #PGCC

OBJ = compile.o  funclib.o  lexical.o  linked.o  memstr.o  preproc.o  str.o  tokenize.o  vcc.o

vcc: $(OBJ)
	$(CXX) $(OBJ) -o vcc

clean:
	rm *.o vcc || true

all: clean vcc