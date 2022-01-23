#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <regex.h>
#include <fcntl.h>
#include <unistd.h>

#include "Funcov.h"

static int in_pipes[2] ;	
static int out_pipes[2] ;
static int err_pipes[2] ;

static pid_t child;

prog_info_t* info;


void
time_handler(int sig){
	if(sig == SIGALRM){
		perror("timeout ");
		kill(child, SIGINT);
	}
}

void usage(){
  printf("  ===========input format============\n"); 
  printf("funcov -b [excute_binary path] -i [inp-dir path] -o [output-dir path]\n");
}

unsigned hash(char* name){
  unsigned hash_val = 0;
  
  int length = strlen(name);
  
  for(int i = 0; i < length; i++){
    hash_val = name[i] + 31*hash_val;
  }

  return hash_val%HASH_SIZE;
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

void find_input(prog_info_t* info){
  DIR * dir = opendir(info->inp_dir);
  struct dirent * dp;
  
  if(dir == NULL){

    fprintf(stderr,"opendir failed\n");
    exit(1);
  
  }else{
    
    while(dp = readdir(dir)){
      if(strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..")==0){
        continue;
      }
      info->inputs[info->inputs_num] =(char*)malloc(sizeof(char)*dp->d_reclen);
      strcpy(info->inputs[info->inputs_num],dp->d_name);
      info->inputs_num++;
    }

  }
  info->func_size = (int*)malloc(sizeof(int)*info->inputs_num);
  info->func_union = (function_hit_t**)malloc(sizeof(function_hit_t*)*info->inputs_num);

  for(int i = 0; i < info->inputs_num; i++){
    info->func_size[i] = 0;
    info->func_union[i] = (function_hit_t*)calloc(MAX_NUM,sizeof(function_hit_t));
  }
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
    buf[length]='\0';
    int s = write(in_pipes[1],buf,length);
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

int get_info(prog_info_t* info){
  close(in_pipes[0]);
  close(out_pipes[1]);
  close(err_pipes[1]);
  close(err_pipes[0]);
  close(in_pipes[1]);


  ssize_t s;
  char buf[MAX_BUF];

  FILE *fp = fdopen(out_pipes[0],"rb");

  if(fp == NULL){
    fprintf(stderr,"fdopen failed\n");
    exit(1);
  }

  int trial = info->trial;
  int cnt = 0;

  while((fgets(buf,MAX_BUF,fp))!= NULL){
    cnt++;
    char* ptr = strstr(buf,"log-function: ");


    if(ptr != NULL){
  
      ptr = strtok(ptr," ");
      ptr = strtok(NULL," ");

      if(ptr == NULL){
        continue;
      }

      unsigned index = hash(ptr);
      
      function_hit_t* p;

      int find = 0;
      if(info->func_union[trial][index].function_name != NULL){

        for(p = &info->func_union[trial][index]; p->next != NULL; p = p->next){
          if(strcmp(ptr,p->function_name) == 0){
            find = 1;
            p->hit++;
            break;
          }
        }

        if(find == 0){
          
          function_hit_t* new = (function_hit_t*)malloc(sizeof(function_hit_t));
          new->function_name = (char*)malloc(sizeof(char)*strlen(ptr));
          strncpy(new->function_name,ptr,strlen(ptr)-1);

          new->hit = 1;
          new->next = NULL;
          p->next = new;
          info->func_size[trial] ++;
        }

      }else{
        info->func_union[trial][index].function_name = (char*)malloc(sizeof(char)*strlen(ptr));
        strcpy(info->func_union[trial][index].function_name,ptr);
          
        info->func_union[trial][index].hit = 1;
        info->func_union[trial][index].next = NULL;
        info->func_size[trial]++;
      }
    }
    memset(buf,0,sizeof(char)*MAX_BUF);
  }

  fclose(fp);
  close(out_pipes[0]);
  close(err_pipes[0]);
}

int run(prog_info_t* info){

  if (pipe(in_pipes) != 0) 
  {
		perror("IN Pipe Error\n") ;
		exit(1) ;
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

  if(child == 0)
  { 
    alarm(1);
    execute_prog(info);

  }else if(child > 0)
  {
    get_info(info);
    info->trial++;
  }else
  {
    fprintf(stderr,"Fork failed\n");
    exit(1);

  }
  int ret;
  wait(&ret);
  
  return 0;
}

void info_free(prog_info_t* info){

  for(int i = 0; i < info->inputs_num;i++){
    free(info->inputs[i]);
    free(info->func_union[i]);
  }
   free(info->func_size);
  free(info->func_union);
  free(info);
}

void save_result(prog_info_t* info){

  
  for(int i = 0; i < info->inputs_num; i++){

    int length = strlen(info->out_dir) + strlen(info->inputs[i]) + 6;
    char* path = (char*)malloc(sizeof(char)*length);
    sprintf(path,"%s/%s.csv",info->out_dir,info->inputs[i]);

    FILE* fp = fopen(path,"wb");
    
    if(fp == NULL)
    {
      fprintf(stderr,"fopen failed\n");
      exit(1);
    }
    
    fprintf(fp,"Function,hit\n");
    int max = HASH_SIZE;
    for(int j = 0; j < max; j++){
      if(info->func_union[i][j].function_name != NULL){
     
        function_hit_t* p;
        for(p = &info->func_union[i][j]; p != NULL; p = p->next){
          fprintf(fp,"%s,%d\n",p->function_name,p->hit); 
        }
     
      }
    }
    fprintf(fp,"total,%d\n",info->func_size[i]);
    fclose(fp);
    free(path);
  }
  
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

int main(int argc,char* argv[]){
  signal(SIGALRM, time_handler);
  
  info = info_init();

  get_option(argc,argv,info);
	find_input(info);

  while(info->inputs_num > info->trial){
      printf("index: %d\n",info->trial);
      run(info);
  }

  save_result(info);
  info_free(info);

  return 0; 
}
