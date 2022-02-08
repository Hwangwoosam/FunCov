#ifndef SHM
#define SHM

#define SHM_KEY 5555

#include "Funcov_shared.h"

typedef struct SHM_info{
    function_hit_t func_union[HASH_SIZE][20];
    int func_num[HASH_SIZE];
}SHM_info_t;

unsigned hash(char* name);
int shm_alloc(int opt);
void shm_dettach(SHM_info_t* shm_info);
void shm_dealloc(int shmid);

#endif