# FunCov
  Funcov is a tool to measure function pair coverage using LLVM Sanitizer Coverage.
  Function pair coverage consists of callee function, caller function, and called location.

  This tool implements the function before adding the function coverage measurement function to AFL++.

#How to build
------------------------------------
```bash  
  $git clone https://github.com/Hwangwoosam/FunCov.git
  $cd FunCov
  $make
```

#Example
-----------------------------------
##1. To use this tool, compile the object file of trace-pc-guard.c with the target program to be tested and the -rdynamic and -g options.
```bash
  $clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute_file_name trace-pc-guard.o
```
