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
