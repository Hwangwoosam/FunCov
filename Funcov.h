#define MAX_ADDR 2048
#define MAX_BUF 4096
#define MAX_NUM 1000
#define HASH_SIZE 1000;

typedef struct function_hit{
    struct function_hit* next;
    char* function_name;
    int hit;
}function_hit_t;

typedef struct prog_info{
    char out_dir[MAX_ADDR];
    char inp_dir[MAX_ADDR];
    char binary[MAX_ADDR];
    char* inputs[MAX_NUM];

    int inputs_num;
    int trial;

    int* func_size;
    function_hit_t** func_union;
    
}prog_info_t;
