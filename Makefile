all:
	gcc -o funcov_m Funcov_shared.c
	clang -fsanitize=address -c trace-pc-guard.c
#	clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute_file_name trace-pc-guard.o

