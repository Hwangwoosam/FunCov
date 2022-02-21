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
#include <execinfo.h>

#include "../include/Funcov_shared.h"
#include "../include/shared_memory.h"


static pid_t child;

static int in_pipes[2] ;
static int out_pipes[2] ;
static int err_pipes[2] ;

prog_info_t* prog_info;

SHM_info_t* shm_info = NULL;

indiv_coverage_t* indiv_coverage;
union_coverage_t  union_coverage;

int* table_trans;
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

int get_option(int argc,char* argv[],prog_info_t* prog_info){
  int c;
  int inp_flag = 0,b_flag =0,out_flag = 0;

  while( (c= getopt(argc,argv,"b:i:o:")) != -1){

    switch (c)
    {
    case 'b':
      if(access(optarg,X_OK) == 0){
        memcpy(prog_info->binary,optarg,strlen(optarg));  
      }
      else{
        fprintf(stderr,"Wrong binary path\n");
        return 1;
      }

      b_flag = 1;
      break;

    case 'i':  
      if(access(optarg,X_OK) == 0){
        memcpy(prog_info->inp_dir,optarg,strlen(optarg));  
      }
      else{
        fprintf(stderr,"Wrong input directory path\n");
        return 1;
      }

      inp_flag = 1;
      break;

    case 'o':

      if(access(optarg,X_OK) == 0){
        memcpy(prog_info->out_dir,optarg,strlen(optarg));  
      }
      else{
        fprintf(stderr,"Wrong output directory path\n");
        return 1;
      }

      out_flag = 1;
      break;

    default:
      goto print_usage;
    }
  }

  if(inp_flag == 0 || b_flag == 0){
    fprintf(stderr,"input binary path and input directory path\n");
    goto print_usage;
  }

  if(out_flag == 0){
    sprintf(prog_info->out_dir,"./output");

    if(mkdir(prog_info->out_dir,0776) != 0)
    {
      free(prog_info);
      fprintf(stderr,"failed to create output directory\n");
      return 1;
    }
  }

  char* idv = (char*)malloc(sizeof(char)*(strlen(prog_info->out_dir) + 10));
  sprintf(idv,"%s/coverage",prog_info->out_dir);
  
  if(mkdir(idv,0776) != 0)
  {
    free(idv);
    fprintf(stderr,"failed to coverage directory in output\n");
    return 1;
  }

  free(idv);

  printf("===OPTION CHECK===\n");
  printf("Input Path: %s\nOutput Path: %s\nBinary Path: %s\n",prog_info->inp_dir,prog_info->out_dir,prog_info->binary);
  return 0;

print_usage:
  printf("  ===========input format============\n"); 
  printf("funcov -b [excute_binary path] -i [inp-dir path] -o [output-dir path]\n");
  return 1;
}

prog_info_t* prog_info_init(){

  prog_info_t* new_info = (prog_info_t*)malloc(sizeof(prog_info_t));

  memset(new_info->binary,0,MAX_ADDR);
  memset(new_info->inp_dir,0,MAX_ADDR);
  memset(new_info->out_dir,0,MAX_ADDR);

  new_info->inputs_num = 0;
  new_info->trial = 0;

  return new_info;
}

int find_input(prog_info_t* prog_info){
  DIR * dir = opendir(prog_info->inp_dir);
  struct dirent * dp;
  
  if(dir == NULL){

    fprintf(stderr,"opendir failed\n");
    return 1;

  }else{
    
    while(dp = readdir(dir)){
      
      if(dp->d_name[0] == '.' || strcmp(dp->d_name,"..")==0){
        continue;
      }

      prog_info->inputs[prog_info->inputs_num] =(char*)malloc(sizeof(char)*(dp->d_reclen+1));
      strcpy(prog_info->inputs[prog_info->inputs_num],dp->d_name);
      
      prog_info->inputs_num++;
    }

    closedir(dir);

    if(prog_info->inputs_num > MAX_NUM)
    {
      fprintf(stderr,"too many input files\n");
      return 1;
    }

  }

  indiv_coverage = (indiv_coverage_t*)malloc(sizeof(indiv_coverage_t)*prog_info->inputs_num);
  printf("Total Input: %d\n",prog_info->inputs_num);
  return 0;
}

void get_coverage(int trial){

  indiv_coverage[trial].func_list = (int*)malloc(sizeof(int)*shm_info->cnt);
  indiv_coverage[trial].hit_cnt = (int*)malloc(sizeof(int)*shm_info->cnt);
  indiv_coverage[trial].func_cnt = shm_info->cnt;

  int idv_cnt = 0;

  for(int i = 0; i < HASH_SIZE; i++){
    
    if(shm_info->func_coverage[i].hit_cnt == 0){
      continue;
    }else{

      int idx = i;
      int length = strlen(shm_info->func_coverage[i].func_line);
      
      do
      { 
        if(union_coverage.total_coverage[idx] == NULL){

          indiv_coverage[trial].func_list[idv_cnt] = idx;
          indiv_coverage[trial].hit_cnt[idv_cnt] = shm_info->func_coverage[i].hit_cnt;
          idv_cnt++;

          union_coverage.total_coverage[idx] = (char*)malloc(sizeof(char)*(length+1));
          strcpy(union_coverage.total_coverage[idx],shm_info->func_coverage[i].func_line);
          union_cnt++;

          break;
        }else{
          if(strcmp(shm_info->func_coverage[i].func_line,union_coverage.total_coverage[idx]) == 0){
            
            indiv_coverage[trial].func_list[idv_cnt] = idx;
            indiv_coverage[trial].hit_cnt[idv_cnt] = shm_info->func_coverage[i].hit_cnt;
            idv_cnt++;

            break;
          }
        }

        if(idx + 1 >= HASH_SIZE){
          idx = 0;
        }

        idx++;

      }while(idx != i);

    }
  } 
  indiv_coverage[trial].union_cnt = union_cnt;
}

void execute_prog(prog_info_t* prog_info){
  alarm(5);
  int inp;
  int path_length = strlen(prog_info->inp_dir) + strlen(prog_info->inputs[prog_info->trial]) + 2;
  
  char* path = (char*)malloc(sizeof(char)*path_length);
  sprintf(path,"%s/%s",prog_info->inp_dir,prog_info->inputs[prog_info->trial]);

  if( (inp = open(path,O_RDONLY)) == -1){
    fprintf(stderr,"input file open failed\n");
    exit(1);
  }

  ssize_t length;
  char buf[MAX_BUF];

  while((length = read(inp,buf,MAX_BUF)) > 0){
    
    int s = write(in_pipes[1],buf,length);
    
    while(s <length)
    {
      s += write(in_pipes[1],buf+s,length-s);
    }

  }
  close(inp);
  free(path);

  close(in_pipes[1]);
  
  dup2(in_pipes[0],0);

  close(in_pipes[0]);
  close(out_pipes[0]);
  close(err_pipes[0]);

  dup2(out_pipes[1],1);
  dup2(err_pipes[1],2);
  
  char* args[] = {prog_info->binary, (char*)0x0};
  
  if(execv(prog_info->binary,args) == -1){
    perror("[excute_prog] - Execution Error\n");
  	exit(1);
  }

}

int run(prog_info_t* prog_info){

  if (pipe(in_pipes) != 0) goto pipe_err;
  if (pipe(out_pipes) != 0) goto pipe_err;
	if (pipe(err_pipes) != 0) goto pipe_err;

  child = fork();

  int ret ;

  if(child == 0){
    alarm(3);
    execute_prog(prog_info);

  }else if(child > 0){
    close(in_pipes[0]);
    close(in_pipes[1]);
    close(err_pipes[0]);
    close(err_pipes[1]);
    close(out_pipes[0]);
    close(out_pipes[1]);

    wait(&ret);

    get_coverage(prog_info->trial);

  }else{
    goto fork_err;    
  }

  return ret;

pipe_err:
  fprintf(stderr,"(RUN) Pipe error\n");
  goto exit;

fork_err:
  fprintf(stderr,"(RUN) Fork error\n");
  goto exit;

exit:
  shm_dealloc(shmid);
  exit(1);
}


void save_result(){
  int out_dir_len = strlen(prog_info->out_dir);
  
  char* union_path = (char*)malloc(sizeof(char)*(out_dir_len +10));
  
  sprintf(union_path,"%s/union.csv",prog_info->out_dir);
  
  FILE* fp = fopen(union_path,"w+");

  fprintf(fp,"trial,func_cnt\n");  
  for(int i = 0; i < prog_info->inputs_num; i++){

    int idv_result_len = strlen(prog_info->inputs[i]);

    char* idv_path = (char*)malloc(sizeof(char)*(out_dir_len + idv_result_len + 15));
    sprintf(idv_path,"%s/coverage/%s.csv",prog_info->out_dir,prog_info->inputs[i]);

    FILE* fp2 = fopen(idv_path,"w+");
    fprintf(fp2,"callee,caller,caller_line,hit,line number\n");
    for(int j = 0; j < indiv_coverage[i].func_cnt; j++){
      
      int idx = indiv_coverage[i].func_list[j];

      fprintf(fp2,"%s,%d,%s",union_coverage.total_coverage[idx],indiv_coverage[i].hit_cnt[j],union_coverage.line_number[idx]);

    }
    fclose(fp2);
    fprintf(fp,"%d,%d\n",i,indiv_coverage[i].union_cnt);
    free(idv_path);
  }
  fclose(fp);
  free(union_path);
}

void translate_addr(){
  
  char** argv = (char**)malloc(sizeof(char*)*(union_cnt + 4));
  table_trans = (int*)malloc(sizeof(int) * union_cnt);

  argv[0] = (char*)malloc(sizeof(char)*MAX_ADDR);
  strcpy(argv[0],"/usr/bin/addr2line");

  argv[1] = (char*)malloc(sizeof(char)*4);
  strcpy(argv[1],"-e");

  argv[2] = (char*)malloc(sizeof(char)*MAX_ADDR);
  strcpy(argv[2],prog_info->binary);

  argv[union_cnt+3] = (char*)0x0;

  int index = 0;
  for(int i = 0; i < HASH_SIZE; i++){
    if(union_coverage.total_coverage[i] != NULL){

          
      table_trans[index] = i;
      int len = strlen(union_coverage.total_coverage[i]);
      int addr2_cnt = 0;

      for(int j = 0; j < len; j++){
        if(union_coverage.total_coverage[i][j] == ','){
          addr2_cnt++;
          if(addr2_cnt == 2){
            argv[index + 3] = &union_coverage.total_coverage[i][j+1];
            break;
          } 
        }
      }

      index ++;
    }
  }

  if(pipe(out_pipes) != 0){
    perror("(Translate) Pipe Error\n");
    exit(1);
  }

  pid_t addr2line;
  addr2line = fork();
  int ret;
  if(addr2line == 0){
    close(out_pipes[0]);
    dup2(out_pipes[1],1);

    execv(argv[0],argv);
    printf("execv failed\n");
  }else if(addr2line > 0){
    close(out_pipes[1]);
    wait(&ret);

    FILE * fp = fdopen(out_pipes[0],"rb");
    if(fp == 0x0){
      perror("translate failed\n");
      exit(1);
    }

    char buf[MAX_BUF];
    int number = 0;
    while(fgets(buf,MAX_BUF,fp) != 0x0){
      union_coverage.line_number[table_trans[number]] = (char*)malloc(sizeof(char)*(strlen(buf) + 1));
      strcpy(union_coverage.line_number[table_trans[number]],buf);
      number++;
    }

  }else{
    fprintf(stderr,"fork error\n");
    exit(1);
  }

  free(argv);
  free(table_trans);

}

void free_all(){
   for(int i = 0; i < prog_info->inputs_num; i++){
    free(indiv_coverage[i].func_list);
    free(indiv_coverage[i].hit_cnt);
    free(prog_info->inputs[i]);
  }

  for(int i = 0; i < HASH_SIZE; i++){
    if(union_coverage.total_coverage[i] != NULL){
      free(union_coverage.total_coverage[i]);
    }
  }

  free(indiv_coverage);
  free(prog_info);
}

int main(int argc,char* argv[]){

  prog_info = prog_info_init();

  if(get_option(argc,argv,prog_info)){
    fprintf(stderr,"Wrong option\n");
    exit(1);
  }

  if(find_input(prog_info)){
    fprintf(stderr,"Wrong input files\n");
    exit(1);
  }

  shmid = shm_alloc(0);
  
  if(shmid == -1)
  {
    fprintf(stderr,"There already exists Shared memory\n");
    exit(1);
  }else{
    shared_memory = shmat(shmid,(void*)0,0);
    shm_info =(SHM_info_t*) shared_memory;

    memset(shm_info,0,sizeof(SHM_info_t));
  }

  while(prog_info->inputs_num > prog_info->trial){

    printf("trial: %d\n",prog_info->trial);
    int ret = run(prog_info);

    memset(shm_info,0,sizeof(SHM_info_t));
    prog_info->trial++;
  }

  shm_dettach(shm_info);
  shm_dealloc(shmid);

  translate_addr();
  save_result();

  free_all();

  return 0; 
}
