all:
	clang -fsanitize=address -c trace-pc-guard.c
#	clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute file name trace-pc-guard.o

