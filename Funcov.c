#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> 
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include "Funcov.h"

static int in_pipes[2] ;	
static int out_pipes[2] ;
static int err_pipes[2] ;

static pid_t child;

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

void init(int argc,char* argv[],prog_info_t* info){
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

    info->result = (int**)malloc(sizeof(int*)*info->inputs_num);
    for(int i = 0; i < info->inputs_num; i++){
      info->result[i] = (int*)calloc(100,sizeof(int)); // size?
    }
  }
  
  printf("Find input SUCCESS\n");
}

void execute_prog(prog_info_t* info){
  int inp;
  int path_length = strlen(info->inp_dir) + strlen(info->inputs[info->index]) + 2;
  char* path = (char*)malloc(sizeof(char)*path_length);
  sprintf(path,"%s/%s",info->inp_dir,info->inputs[info->index]);

  ssize_t length;
  char buf[MAX_BUF];

  if( (inp = open(path,O_RDONLY)) == -1){
    fprintf(stderr,"input file open failed\n");
    exit(1);
  }

  while((length = read(inp,buf,MAX_BUF)) > 0){
    write(in_pipes[1],buf,length);
  }

  close(in_pipes[1]);
  close(inp);

  dup2(in_pipes[0],0);

  close(in_pipes[0]);
  close(out_pipes[0]);
  close(err_pipes[0]);

  dup2(out_pipes[1],1);
  dup2(err_pipes[1],2);

  free(path);

  execl(info->binary,info->binary,NULL);

  perror("[excute_prog] - Execution Error\n");
	exit(1);
}

int get_info(prog_info_t* info){
  close(in_pipes[0]);
  close(out_pipes[1]);
  close(err_pipes[1]);
  close(in_pipes[1]);
  
  ssize_t s;
  char buf[MAX_BUF];
  FILE *fp = fdopen(out_pipes[0],"r");

  if(fp == NULL){
    fprintf(stderr,"fdopen failed\n");
    exit(1);
  }

  while(fgets(buf,MAX_BUF,fp)!= NULL){
    char* ptr;
    if((ptr = strstr(buf,"guard:"))!= NULL ){
      ptr = strtok(ptr," ");
      ptr = strtok(NULL," ");
      int num = atoi(ptr);
      if(num >= 100){
        continue;
      }
      info->result[info->index][num]++;
    } 
  }
  
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
    info->index++;
  }else
  {
    fprintf(stderr,"Fork failed\n");
    exit(1);

  }

  return 0;
}

void info_free(prog_info_t* info){
  for(int i = 0; i < info->inputs_num;i++){
    free(info->inputs[i]);
    free(info->result[i]);
  }

  free(info->result);
  free(info);
}

void save_result(prog_info_t* info){


  for(int i = 0; i < info->inputs_num; i++){

    int length = strlen(info->out_dir) + strlen(info->inputs[i]) + 6;
    char* path = (char*)malloc(sizeof(char)*length);
    sprintf(path,"%s/%s.csv",info->out_dir,info->inputs[i]);

    FILE* fp = fopen(path,"wb");
    fprintf(fp,"function,hit\n");
    int cnt = 0;
    for(int j = 0; j < 100; j++){
      if(info->result[i][j] != 0){
        cnt ++;
        fprintf(fp,"%d,%d\n",j,info->result[i][j]);
      }
    }
    fprintf(fp,"total,%d",cnt);
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
  new_info->index = 0;

  return new_info;
}

int main(int argc,char* argv[]){
  prog_info_t* info = info_init();

  init(argc,argv,info);
	find_input(info);

  while(info->inputs_num > info->index){
    run(info);
  }

  save_result(info);
  info_free(info);

  return 0; 
}
