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

unsigned short hash16(char* name){
  unsigned hash_val = 0;
  
  int length = strlen(name);
  
  for(int i = 0; i < length; i++){
    hash_val = (unsigned char)name[i] + 23131*hash_val;
  }

  return (hash_val&0xffff);
}

void add_elem(char* func_line ,int hash_val){

  strcpy(shm_info->func_coverage[hash_val].func_line,func_line);

  shm_info->func_coverage[hash_val].hit_cnt = 1;
  shm_info->cnt++;
}

void lookup(char* callee,char* caller, char* caller_line){
  
  char func_line[512];
  sprintf(func_line,"%s,%s,%s",callee,caller,caller_line);

  int hash_val = hash16(func_line);
  int i = hash_val;

  do
  {
    if(shm_info->func_coverage[i].hit_cnt == 0){
      add_elem(func_line,i);    
      
      break;
    }else{
 
      if(strcmp(shm_info->func_coverage[i].func_line,func_line) == 0){

        shm_info->func_coverage[i].hit_cnt++;
        break;
      }

    }

    i++;
    
    if(i >= HASH_SIZE){
      i = 0;
    }

  }while(i != hash_val);

}

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start,uint32_t *stop) {
  static uint64_t N;  // Counter for the guards.
  if (start == stop || *start) return;  // Initialize only once.
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

  void* buffer[1024];
  int addr_num = backtrace(buffer,1024);

  char **strings;
  strings = backtrace_symbols(buffer,addr_num);
  
  if(strstr(strings[2],"+") == NULL || strstr(strings[3],"+") == NULL){
    return ;
  }
  
  callee = strtok(strings[2],"(");
  callee = strtok(NULL,"+");

  if(strcmp(callee,"main") == 0){
    return ;
  }

  caller = strtok(strings[3],"(");
  caller = strtok(NULL,"+");

  caller_line = strtok(NULL,"[");
  caller_line = strtok(NULL,"]");
  
  lookup(callee,caller,caller_line);
  
  free(strings);

  shm_dettach(shm_info);

}

