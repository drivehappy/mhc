CXX				:= g++
#CXX				:= /home/mharmer/dev/afl-0.87b/afl-g++
#CXX				:= clang
#CC_FLAGS 		:= -Wall -Werror -O0 -g -std=c++11 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -I../src/ -I../src/lib/ -I/usr/include/llvm-3.5/ -I/usr/include/llvm-c-3.5/ -L/usr/lib/x86_64-linux-gnu -L/usr/lib/llvm-3.5/lib 
CC_FLAGS 		:= -Wall -Werror -O2 -std=c++11 -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -I../src/ -I../src/lib/ -I/usr/include/llvm-3.5/ -I/usr/include/llvm-c-3.5/ -L/usr/lib/x86_64-linux-gnu -L/usr/lib/llvm-3.5/lib 
LD_FLAGS 		:=  `llvm-config-3.5 --libs all` `llvm-config-3.5 --ldflags --system-libs` -lboost_program_options -lboost_system -lboost_filesystem
LD_FLAGS_TESTS	:= $(LD_FLAGS) -lgtest -lpthread
CPP_FILES		:= $(wildcard ../src/*.cpp)
CPP_FILES_LIB	:= $(wildcard ../src/lib/*.cpp)
CPP_FILES_TESTS	:= $(wildcard ../tests/*.cpp)
OBJ_FILES_LIB	:= $(patsubst ../src/lib/%.cpp,../src/lib/%.o,$(CPP_FILES_LIB))
OBJ_FILES		:= $(patsubst ../src/%.cpp,../src/%.o,$(CPP_FILES))
OBJ_FILES_TESTS	:= $(patsubst ../tests/%.cpp,../tests/%.o,$(CPP_FILES_TESTS))

#
all: build-all

build-all: main tests

../src/%.o: ../src/%.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $<

../src/lib/%.o: ../src/lib/%.cpp
	$(CXX) $(CC_FLAGS) -c -o $@ $<

../tests/%.o: ../tests/%.cpp 
	$(CXX) $(CC_FLAGS) -c -o $@ $<

main: $(OBJ_FILES) $(OBJ_FILES_LIB)
	$(CXX) $(CC_FLAGS) -o mhc $(OBJ_FILES) $(OBJ_FILES_LIB) $(LD_FLAGS)

tests: $(OBJ_FILES_TESTS) $(OBJ_FILES_LIB)
	$(CXX) $(CC_FLAGS) -o mhc-tests $(OBJ_FILES_TESTS) $(OBJ_FILES_LIB) $(LD_FLAGS_TESTS)
 
output: output.o wrapper.o
	gcc output.o -o output
	g++ -std=c++0x wrapper.cpp -o wrapper

output.o: output.asm 
	as --32 -o output.o output.asm 


#.PHONY: build-dirs
#.IGNORE: build-dirs
#build-dirs:
	#mkdir -p out gen

.PHONY: clean
.IGNORE: clean
clean: 
	rm -rf ../src/*.o ../src/lib/*.o ../tests/*.o mhc mhc-tests

