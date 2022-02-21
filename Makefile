all:
	gcc -o funcov_m ./src/Funcov_shared.c ./src/shared_memory.c
	clang -fsanitize=address -c ./src/trace-pc-guard.c ./src/shared_memory.c 
#	clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute_file_name trace-pc-guard.o shared_memory.o

