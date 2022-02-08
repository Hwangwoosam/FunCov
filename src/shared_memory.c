#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "Funcov_shared.h"
#include "shared_memory.h"

unsigned hash(char* name){
  unsigned hash_val = 0;
  
  int length = strlen(name);
  
  for(int i = 0; i < length; i++){
    hash_val = name[i] + 31*hash_val;
  }

  return hash_val%HASH_SIZE;
}

int shm_alloc(int opt){
  int shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|IPC_EXCL|0666);
  
  if(shmid == -1){
    if(opt == 0){
      fprintf(stderr,"SHM exist!!\n");
    
      shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|0666);
      shm_dealloc(shmid);

      exit(1);
    }else{
      shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|0666);
    }
  }

  return shmid;
}

void shm_dettach(SHM_info_t* shm_info){
  
  if(shmdt(shm_info) == -1){
      fprintf(stderr,"Shmdt failed\n");
      exit(1);
  }

}

void shm_dealloc(int shmid){

  if(shmctl(shmid,IPC_RMID,0) == -1){
      fprintf(stderr,"Shmctl failed\n");
      exit(1);
  }

}
