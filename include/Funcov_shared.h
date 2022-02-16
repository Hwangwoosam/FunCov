#ifndef _FUNCOV_H_
#define _FUNCOV_H_

#define MAX_ADDR 256
#define MAX_BUF 4096
#define MAX_NUM 1000
#define HASH_SIZE 65536

typedef struct function_hit{

    char func_line[512]; //calle + caller + caller address
    int hit_cnt;

}function_hit_t;

typedef struct union_coverage{

    char* total_coverage[HASH_SIZE]; 
    char* line_number[HASH_SIZE];

}union_coverage_t;

typedef struct indiv_coverage{
    int* func_list;
    int* hit_cnt;
    int func_cnt;
    int union_cnt;
}indiv_coverage_t;

typedef struct prog_info{
    char out_dir[MAX_ADDR];
    char inp_dir[MAX_ADDR];
    char binary[MAX_ADDR];
    char* inputs[MAX_NUM];

    int inputs_num;
    int trial;

}prog_info_t;

#endif
