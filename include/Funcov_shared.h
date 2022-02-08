#ifndef _FUNCOV_H_
#define _FUNCOV_H_

#define MAX_ADDR 256
#define MAX_BUF 4096
#define MAX_NUM 1000
#define HASH_SIZE 6000

typedef struct function_hit* func_link;

typedef struct function_hit{
    char func_line[512]; //calle + caller + line number;
    
    int pt_line;        

    int hit;
}function_hit_t;

typedef struct prog_info{
    char out_dir[MAX_ADDR];
    char inp_dir[MAX_ADDR];
    char binary[MAX_ADDR];
    char* inputs[MAX_NUM];

    int inputs_num;
    int trial;

}prog_info_t;

// typedef struct SHM_info{
//     function_hit_t func_union[HASH_SIZE][20];
//     int func_num[HASH_SIZE];
// }SHM_info_t;

#endif