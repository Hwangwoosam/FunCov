#define MAX_ADDR 2048
#define MAX_BUF 4096
#define MAX_NUM 1000

typedef struct prog_info{
    char out_dir[MAX_ADDR];
    char inp_dir[MAX_ADDR];
    char binary[MAX_ADDR];
    char* inputs[MAX_NUM];
    int inputs_num;
    int index;
}prog_info_t;