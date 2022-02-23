## FunCov
  Funcov is a tool to measure function pair coverage using LLVM Sanitizer Coverage.
  Function pair coverage consists of callee function, caller function, and called location.

  This tool implements the function before adding the function coverage measurement function to AFL++.

## How to build
------------------------------------
```bash  
  $git clone https://github.com/Hwangwoosam/FunCov.git
  $cd FunCov
  $make
```

## Example
-----------------------------------
### 1. To use this tool, compile the object file of trace-pc-guard.c with the target program to be tested and the -rdynamic and -g options.
```bash
  $clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute_file_name trace-pc-guard.o shared_memory.o
```

### 2. Execute `Funcov`
```bash
# usage: ./funcov_m -i [input_dir] -o [output_dir] -b [binary file] (optino)@@

# required
# -i : input directory path
# -b : executable binary path 

# optional
# -o : output directory path
# If there is no argument about the output directory, the directory called output is set as a default value. 
# @@ : When input is given as a file argument 
```
### 3. Result directory

* **coverage** : This directory stores the function coverage for each input.
* **union.csv** : This file stores the function coverage for the entire input.

* **This is the result of test case(mpc)**

<img width="498" alt = "image" src="https://user-images.githubusercontent.com/61729954/154293590-7ef9b8e4-3a1f-48ff-8da7-f1d4edca936a.png">

### 4. Display example result in terminal
```bash
./funcov_m -i chec -o test/mpc/output/ -b test/mpc/math_file_fuzzer @@
```
```bash
===OPTION CHECK===
Input Path: chec
Output Path: test/mpc/output/
Binary Path: test/mpc/math_file_fuzzer
Total Input: 3
[0]id:000000,time:0,execs:0,orig:math_bnf
[1]id:000001,src:000000,time:223,execs:83,op:havoc,rep:8,+cov
[2]id:000002,src:000000,time:266,execs:92,op:havoc,rep:8,+cov
===Translate address To Line number success===
===Save result success===
```

### 5. Result file format in output directory
* **union.csv**
<img width = "498" alt = "image" src = "https://user-images.githubusercontent.com/61729954/155263449-0d400369-97a7-4783-bbd8-6bd18538d81f.png">

* **coverage/individual_result**
<img width = "498" alt = "image" src = "https://user-images.githubusercontent.com/61729954/155263604-98267457-5937-4466-886f-5ca222be5f74.png">
