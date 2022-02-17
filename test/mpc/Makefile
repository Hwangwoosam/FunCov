all: 
	clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard -o math_file_fuzzer math_file_fuzzer.c mpc.c ../../trace-pc-guard.o ../../shared_memory.o
	clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard -o math_fuzzer math_fuzzer.c mpc.c ../../trace-pc-guard.o ../../shared_memory.o

clean:
	rm *.o math_file_fuzzer math_fuzzer
