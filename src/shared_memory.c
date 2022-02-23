#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "../include/Funcov_shared.h"
#include "../include/shared_memory.h"

int shm_alloc(int opt){
  int shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|IPC_EXCL|0666);
  
  if(shmid == -1){

    if(opt == 0){
      shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|0666);
      shm_dealloc(shmid);

      return -1;
    }else{
      shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|0666);
    }
  }

  return shmid;
}

void* shm_attach(int shm_id){
  void * ptr = shmat(shm_id,0,0);
  if(ptr == (void*)(-1)){
    fprintf(stderr,"shm_attach failed\n");
    return NULL;
  }

  return ptr;
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
