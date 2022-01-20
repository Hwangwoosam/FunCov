#define MAX_ADDR 2048
#define MAX_BUF 4096
#define MAX_NUM 1000
#define MAP_SIZE 1024

typedef struct prog_info{
    char out_dir[MAX_ADDR];
    char inp_dir[MAX_ADDR];
    char binary[MAX_ADDR];
    char* inputs[MAX_NUM];
    int** result;
    int* result_func_num;
    char* uni;
    int max_size;
    int inputs_num;
    int index;
}prog_info_t;
