all:
	clang -fsanitize=address -fsanitize-coverage=func,trace-pc-guard tommath.a fuzz_tommath.c -o fuzz_tom trace-pc-guard.o