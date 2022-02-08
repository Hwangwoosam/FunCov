#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <execinfo.h>
#include "regex.h"
#include "sys/types.h"
#include <sanitizer/coverage_interface.h>
#include "../Funcov_shared.h"
#include "../shared_memory.h"

int shmid;
SHM_info_t* shm_info = NULL;
void* shared_memory = (void*)0;

void add_elem(char* func_line,int pt_line,int hash_val, int idx){

  strcpy(shm_info->func_union[hash_val][idx].func_line,func_line);

  shm_info->func_union[hash_val][idx].pt_line = pt_line;
  shm_info->func_union[hash_val][idx].hit = 1;

  shm_info->func_num[hash_val]++; 
}

void lookup(char* callee, char* caller, char* caller_line){
  char func_line[512];
  
  sprintf(func_line,"%s:%s:%s",callee,caller,caller_line);

  int pt_line = strlen(func_line) - strlen(caller_line) - 1;

  int hash_val = hash(func_line);

  int idx = shm_info->func_num[hash_val]; 

  if(idx == 0){
    add_elem(func_line,pt_line,hash_val,idx);
    
  }else{
    int find = 0;
    for(int i = 0; i < idx; i++){
      if(strcmp(shm_info->func_union[hash_val][i].func_line,func_line) == 0){
        find = 1;
        shm_info->func_union[hash_val][i].hit++;
      }
    }

    if(find == 0){
      add_elem(func_line,pt_line,hash_val,idx);
    }
  }

}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start,uint32_t *stop) {
  static uint64_t N;  // Counter for the guards.
  if (start == stop || *start) return;  // Initialize only once.
  // printf("INIT: %p %p\n", start, stop);
  for (uint32_t *x = start; x < stop; x++){
    *x = ++N;  // Guards should start from 1.
  }
}


void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  if (!*guard) return; 

  shmid = shm_alloc(1);

  shared_memory = shmat(shmid,(void*)0,0);
  shm_info =(SHM_info_t*) shared_memory;

  char* callee;
  char* caller;
  char* caller_line;
  //==============================================test================

  void* buffer[1024];
  int addr_num = backtrace(buffer,1024);

  char **strings;
  strings = backtrace_symbols(buffer,addr_num);

  if(strstr(strings[2],"+") != NULL && strstr(strings[3],"+") != NULL){
    callee = strtok(strings[2],"(");
    callee = strtok(NULL,"+");


    caller = strtok(strings[3],"(");
    caller = strtok(NULL,"+");


    caller_line = strtok(NULL,"[");
    caller_line = strtok(NULL,"]");

    lookup(callee,caller,caller_line);
  }

  free(strings);
  //==============================================test================

  //lookup
  shm_dettach(shm_info);

}

