#ifndef SHM
#define SHM

#define SHM_KEY 5555

#include "Funcov_shared.h"

typedef struct SHM_info{

    function_hit_t func_coverage[HASH_SIZE];
    int cnt;

}SHM_info_t;

int shm_alloc(int opt);
void shm_dettach(SHM_info_t* shm_info);
void* shm_attach(int shmid);
void shm_dealloc(int shmid);

#endif
