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
#include "../include/Funcov_shared.h"

int shmid;
SHM_info_t* shm_info = NULL;
void* shared_memory = (void*)0;

unsigned hash(char* name){
  unsigned hash_val = 0;
  
  int length = strlen(name);
  
  for(int i = 0; i < length; i++){
    hash_val = name[i] + 31*hash_val;
  }

  return hash_val%HASH_SIZE;
}

void add_elem(char* func_line,int pt_line,int hash_val, int idx){

  strcpy(shm_info->func_union[hash_val][idx].func_line,func_line);

  shm_info->func_union[hash_val][idx].pt_line = pt_line;
  shm_info->func_union[hash_val][idx].hit = 1;

  shm_info->func_num[hash_val]++; 
}

void lookup(char* callee, char* caller, char* caller_line){
  char func_line[512];
  
  sprintf(func_line,"%s:%s:%s",callee,caller,caller_line);
  printf("func_line: %s\n",func_line);
  // int pt_line = strlen(func_line) - strlen(caller_line) - 1;

  // int hash_val = hash(func_line);

  // int idx = shm_info->func_num[hash_val]; 

  // if(idx == 0){
  //   add_elem(func_line,pt_line,hash_val,idx);
    
  // }else{
  //   int find = 0;
  //   for(int i = 0; i < idx; i++){
  //     if(strcmp(shm_info->func_union[hash_val][i].func_line,func_line) == 0){
  //       find = 1;
  //       shm_info->func_union[hash_val][i].hit++;
  //     }
  //   }

  //   if(find == 0){
  //     add_elem(func_line,pt_line,hash_val,idx);
  //   }
  // }

}

void shm_init(){
  shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|IPC_EXCL|0666);

  if(shmid == -1){

    shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|0666);

  }else{

    if(shmctl(shmid,IPC_RMID,0) == -1){
      fprintf(stderr,"shmctl failed\n");
      exit(1);
    }
  }

  shared_memory = shmat(shmid,(void*)0,0);
  shm_info =(SHM_info_t*) shared_memory;

}

void shm_dettach(){
  
  if(shmdt(shm_info) == -1){
      fprintf(stderr,"Shmdt failed\n");
      exit(1);
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

  // shm_init();
  char callee[1024] = {0,};
  char caller[1024] = {0,};
  char caller_line[16] = {0,};
  //==============================================test================

  void* buffer[1024];
  int addr_num = backtrace(buffer,1024);

  char **strings;
  strings = backtrace_symbols(buffer,addr_num);

  // printf("%s\n%s\n",strings[2],strings[3]);

  int line_st = 0;
  for(int i = 2; i < 4; i++){
    int str_len = strlen(strings[i]);
    for(int j = 0; j < str_len; j++){
      if(strings[i][j] == '('){
        
        for(int k = j; k < str_len; k++){
          
          if(strings[i][k] == '+'){
          
            if(i == 2){
              memcpy(callee,strings[i]+j+1,k-j-1);
            }else{
              memcpy(caller,strings[i]+j+1,k-j-1);
            }

            break;
          }

        }

        if(i == 2){
          break;
        }

      }else if(strings[i][j] == '['){
        memcpy(caller_line,strings[i]+j+1,str_len-j-2);
      }
    }
  }

  free(strings);
  //==============================================test================

  //lookup
  lookup(callee,caller,caller_line);
  shm_dettach();

}

