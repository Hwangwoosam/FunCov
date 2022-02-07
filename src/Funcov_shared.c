#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>

#include "../include/Funcov_shared.h"

static pid_t child;

static int in_pipes[2] ;
static int out_pipes[2] ;
static int err_pipes[2] ;

prog_info_t* info;

SHM_info_t* fun_cov;
SHM_info_t* shm_info = NULL;

char* fun_cov_union[HASH_SIZE][200];
int union_func_num[HASH_SIZE];
int union_cnt;

void * shared_memory = (void*) 0;

int shmid;

void
time_handler(int sig){
	if(sig == SIGALRM){
		perror("timeout ");
		kill(child, SIGINT);
	}
}

unsigned hash(char* name){
  unsigned hash_val = 0;
  
  int length = strlen(name);
  
  for(int i = 0; i < length; i++){
    hash_val = name[i] + 31*hash_val;
  }

  return hash_val%HASH_SIZE;
}

void usage(){
  printf("  ===========input format============\n"); 
  printf("funcov -b [excute_binary path] -i [inp-dir path] -o [output-dir path]\n");
}

void get_option(int argc,char* argv[],prog_info_t* info){
  int c;
  int inp_flag = 0,b_flag =0,out_flag = 0;

  while( (c= getopt(argc,argv,"b:i:o:")) != -1){
    switch (c)
    {

    case 'b':
      b_flag = 1;
      if(access(optarg,X_OK) == 0){
        memcpy(info->binary,optarg,strlen(optarg));
      }else{
        fprintf(stderr,"Wrong binary path\n");
        exit(1);
      }
      break;
      
    case 'i':
      inp_flag = 1;
      if(access(optarg,X_OK) == 0){
        memcpy(info->inp_dir,optarg,strlen(optarg));
      }else{
        fprintf(stderr,"Wrong input directory path\n");
        exit(1);
      }
      break;

    case 'o':
      out_flag = 1;
      if(access(optarg,X_OK) == 0){
        memcpy(info->out_dir,optarg,strlen(optarg));
      }else{
        fprintf(stderr,"Wrong output directory path\n");
        exit(1);
      }
      break;

    default:
      usage();
      exit(1);
      break;
    }
  }

  if(inp_flag == 0 || b_flag == 0){
    fprintf(stderr,"input binary path and input directory path\n");
    usage();
    exit(1);
  }

  if(out_flag == 0){
    sprintf(info->out_dir,"./output");
    if(mkdir(info->out_dir,0776) != 0){
      free(info);
      fprintf(stderr,"failed to create output directory\n");
      exit(1);
    }
  }
  
  printf("init SUCCESS\n");
}

prog_info_t* info_init(){

  prog_info_t* new_info = (prog_info_t*)malloc(sizeof(prog_info_t));

  memset(new_info->binary,0,MAX_ADDR);
  memset(new_info->inp_dir,0,MAX_ADDR);
  memset(new_info->out_dir,0,MAX_ADDR);

  new_info->inputs_num = 0;
  new_info->trial = 0;

  return new_info;
}

void shm_alloc(){
  shmid = shmget((key_t)SHM_KEY,sizeof(SHM_info_t),IPC_CREAT|IPC_EXCL|0666);

  if(shmid == -1){
    fprintf(stderr,"SHM exist!!\n");
    exit(1);
  }

  shared_memory = shmat(shmid,(void*)0,0);
  shm_info =(SHM_info_t*) shared_memory;

  memset(shm_info,0,sizeof(SHM_info_t));

  printf("shm alloc end\n");
}

void shm_dettach(){
  
  if(shmdt(shm_info) == -1){
      fprintf(stderr,"Shmdt failed\n");
      exit(1);
  }

}

void shm_dealloc(){

  if(shmctl(shmid,IPC_RMID,0) == -1){
      fprintf(stderr,"Shmctl failed\n");
      exit(1);
  }

}

void find_input(prog_info_t* info){
  DIR * dir = opendir(info->inp_dir);
  struct dirent * dp;
  
  if(dir == NULL){

    fprintf(stderr,"opendir failed\n");
    exit(1);
  
  }else{
    
    while(dp = readdir(dir)){
      if(dp->d_name[0] == '.' || strcmp(dp->d_name,"..")==0){
        continue;
      }
      info->inputs[info->inputs_num] =(char*)malloc(sizeof(char)*(dp->d_reclen+1));
      strcpy(info->inputs[info->inputs_num],dp->d_name);
      info->inputs_num++;
    }

    if(info->inputs_num > MAX_NUM){
      fprintf(stderr,"too many input files\n");
      exit(1);
    }

  }
  fun_cov = (SHM_info_t*)malloc(sizeof(SHM_info_t)*(info->inputs_num));

  printf("Find input SUCCESS\n");
}

void execute_prog(prog_info_t* info){
  int inp;
  int path_length = strlen(info->inp_dir) + strlen(info->inputs[info->trial]) + 2;
  
  char* path = (char*)malloc(sizeof(char)*path_length);
  sprintf(path,"%s/%s",info->inp_dir,info->inputs[info->trial]);

  ssize_t length;
  char buf[MAX_BUF];

  if( (inp = open(path,O_RDONLY)) == -1){
    fprintf(stderr,"input file open failed\n");
    exit(1);
  }

  while((length = read(inp,buf,MAX_BUF)) > 0){
    int s = write(in_pipes[1],buf,length);
    while(s <length){
      s += write(in_pipes[1],buf+s,length-s);
    }

  }
  
  close(inp);
  close(in_pipes[1]);
  
  dup2(in_pipes[0],0);

  close(in_pipes[0]);
  close(out_pipes[0]);
  close(err_pipes[0]);

  dup2(out_pipes[1],1);
  dup2(err_pipes[1],2);

  free(path);
  char* args[] = {info->binary, (char*)0x0};
  execv(info->binary,args);

  perror("[excute_prog] - Execution Error\n");
	exit(1);
}

int run(){

  if(pipe(in_pipes) != 0)
  {
    fprintf(stderr,"In Pipe Error\n");
    exit(1);
  }

  if (pipe(out_pipes) != 0) 
  {
		perror("OUT Pipe Error\n") ;
		exit(1) ;
	}

	if (pipe(err_pipes) != 0) 
  {
		perror("ERR Pipe Error\n") ;
		exit(1) ;
	}

  child = fork();

  int ret ;

  if(child == 0){
    // alarm(1);
    execute_prog(info);

  }else if(child > 0){
    close(in_pipes[0]);
    close(in_pipes[1]);
    close(err_pipes[0]);
    close(err_pipes[1]);
    close(out_pipes[0]);
    close(out_pipes[1]);
    wait(&ret);
  }else{
    fprintf(stderr,"Fork failed\n");
    exit(1);
  }

  return ret;
}

void print_func(){
  int cnt = 0;
  for(int i = 0; i < HASH_SIZE; i++){
    for(int j = 0; j < fun_cov[info->trial].func_num[i]; j++){
      // printf("%s\n",fun_cov[info->trial].func_union[i][j].func_line);
      cnt++;
    }
  }

  printf("cnt: %d\n",cnt);
}

void save_result(){
  char* path_union = (char*)malloc(sizeof(char)*MAX_ADDR);
  sprintf(path_union,"%s/union.csv",info->out_dir);

  FILE* fp_union = fopen(path_union,"w+");

  if(fp_union == NULL){
    fprintf(stderr,"union file fopen failed\n");
    exit(1);
  }

  fprintf(fp_union,"trial,func_cov\n");

  for(int i = 0; i < info->inputs_num; i++){

    char* path_indiv = (char*)malloc(sizeof(char)*MAX_ADDR);
    sprintf(path_indiv,"%s/%s",info->out_dir,info->inputs[i]);
    
    FILE* fp_idv = fopen(path_indiv,"w+");
    
    if(fp_idv == NULL){
      fprintf(stderr,"idv fopen failed\n");
      exit(1);
    }
    fprintf(fp_idv,"callee:caller:caller,line_number\n");
    for(int j = 0; j < HASH_SIZE; j++){

      for(int k = 0; k < fun_cov[i].func_num[j]; k++){
        fprintf(fp_idv,"%s,%d\n",fun_cov[i].func_union[j][k].func_line,fun_cov[i].func_union[j][k].hit);
        int length = strlen(fun_cov[i].func_union[j][k].func_line);


        if(union_func_num[j] == 0){
          fun_cov_union[j][0] = (char*)malloc(sizeof(char)*(length + 1));
          strcpy(fun_cov_union[j][0],fun_cov[i].func_union[j][k].func_line);

          union_func_num[j]++;
          union_cnt++;
        }else{
          int find = 0;
          int idx = union_func_num[j];

          for(int l = 0; l < idx; l++){
            if(strcmp(fun_cov_union[j][l],fun_cov[i].func_union[j][k].func_line) == 0){
              find = 1;
              break;
            }
          }

          if(find == 0){
            fun_cov_union[j][idx] = (char*)malloc(sizeof(char)*(length + 1));
            strcpy(fun_cov_union[j][idx],fun_cov[i].func_union[j][k].func_line);

            union_func_num[j]++;
            union_cnt++;
          }

        }

      }

    }

    fclose(fp_idv);
    free(path_indiv);
    fprintf(fp_union,"%d,%d\n",i,union_cnt);
  }

  fclose(fp_union);
  free(path_union);
}

int main(int argc,char* argv[]){
  info = info_init();

  get_option(argc,argv,info);
  find_input(info);

  shm_alloc();

  int ret ;
  while(info->inputs_num > info->trial){

    printf("trial: %d\n",info->trial);
    ret = run();

    memcpy(&fun_cov[info->trial],shm_info,sizeof(SHM_info_t));
    print_func();

    memset(shm_info,0,sizeof(SHM_info_t));
    info->trial++;

  }
  wait(&ret);
  
  shm_dettach();
  shm_dealloc();

  save_result();

  for(int i = 0; i < HASH_SIZE; i++){
    for(int j = 0; j < union_func_num[i]; j++){
      free(fun_cov_union[i][j]);
    }
  }

  return 0; 
}
