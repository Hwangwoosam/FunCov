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
  $clang -rdynamic -g -fsanitize=address -fsanitize-coverage=func,trace-pc-guard execute_file_name trace-pc-guard.o
```

### 2. Execute `Funcov`
```bash
# usage: ./funcov_m -i [input_dir] -o [output_dir] -b [binary file]

# required
# -i : input directory path
# -b : executable binary path 

# optional
# -o : output directory path
# If there is no argument about the output directory, the directory called output is set as a default value. 
```
### 3. Result directory

* **coverage/** : This directory stores the function coverage for each input.
* **union.csv** : This file stores the function coverage for the entire input.

`this is test case(mpc)`

<img width="498" alt = "image" src="https://user-images.githubusercontent.com/61729954/154293590-7ef9b8e4-3a1f-48ff-8da7-f1d4edca936a.png">

