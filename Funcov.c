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

int* func_num;
function_hit_t*** func_individual;

int* uni_num;
function_hit_t* func_union[MAX_NUM];

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

  return hash_val%MAX_NUM;
}

func_link lookup(char* name,int opt){
  func_link p;
  int hash_val = hash(name);

  if(opt == 0){
    for(p = func_individual[info->trial][hash_val]; p != NULL; p = p->next){
      if(strcmp(p->function_name,name) == 0){
          return p;
      }
    }
  }else{
    for(p = func_union[hash_val]; p != NULL; p = p->next){
      if(strcmp(p->function_name,name) == 0){
          return p;
      }
    }
  }
  return NULL;
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
      info->inputs[info->inputs_num] =(char*)malloc(sizeof(char)*dp->d_reclen);
      strcpy(info->inputs[info->inputs_num],dp->d_name);
      info->inputs_num++;
    }

    if(info->inputs_num > MAX_NUM){
      fprintf(stderr,"too many input files\n");
      exit(1);
    }

    func_num = (int*)malloc(sizeof(int)*info->inputs_num);
    uni_num = (int*)malloc(sizeof(int)*info->inputs_num);
    func_individual = (function_hit_t***)malloc(sizeof(function_hit_t**)*info->inputs_num);
    for(int i = 0 ; i < info->inputs_num; i++){
      uni_num[i] = 0;
      func_num[i] = 0;
      func_individual[i] = (function_hit_t**)malloc(sizeof(function_hit_t*)*MAX_NUM);
    }
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
  while((fgets(buf,MAX_BUF,fp))!= NULL){
    
    char* ptr = strstr(buf,"log-function: ");
    if(ptr != NULL){
      ptr = strtok(ptr," ");
      ptr = strtok(NULL," ");
      if(ptr != NULL){
        func_link p;
        int hash_val;
        
        int ptr_len = strlen(ptr);

        for(int i = 0; i < ptr_len; i++){
          if(i == ptr_len-1){
            ptr[i] = '\0';
          }
        }
        
        if((p = lookup(ptr,0)) == NULL){
          
          p = (func_link)malloc(sizeof(function_hit_t));
          
          hash_val = hash(ptr);
          p->function_name = (char*)malloc(sizeof(char)*(ptr_len));
          
          strcpy(p->function_name,ptr);
          p->hit = 1;

          p->next = func_individual[trial][hash_val];
          func_individual[trial][hash_val] = p;
          func_num[trial]++;

          func_link p_u;
          if((p_u = lookup(ptr,1)) == NULL){
            p_u = (func_link)malloc(sizeof(function_hit_t));
            p_u->function_name = (char*)malloc(sizeof(char)*ptr_len);
            strcpy(p_u->function_name,ptr);

            p_u->next = func_union[hash_val];
            func_union[hash_val] = p_u;
            uni_num[trial]++;
          }
        
        }else{
          p->hit++;
        }
      }
    }

    memset(buf,0,sizeof(char)*MAX_BUF);
  }

  if(trial > 0){
    uni_num[trial] += uni_num[trial-1];
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
  func_link p;
  for(int i = 0; i < info->inputs_num;i++){
    free(info->inputs[i]);
    for(int j = 0; j < MAX_NUM; j++){
      p = func_individual[i][j];
      while(p != NULL ){
        func_link fr = p;
        p = p->next;
        free(fr);
      }
    }
    free(func_individual[i]);
  }
  for(int i = 0; i < MAX_NUM; i++){
    p = func_union[i];
    while(p != NULL){
      func_link fr = p;
      p = p->next;
      free(fr);
    }
  }
  free(uni_num);
  free(func_individual);
  free(info);
}

void save_result(prog_info_t* info){
  int output_len = strlen(info->out_dir);
  int union_len = output_len + 11;

  char* path2 = (char*)malloc(sizeof(char)*union_len);

  sprintf(path2,"%s/union.csv",info->out_dir);
  FILE* fp2 = fopen(path2,"w+");

  if(fp2 == NULL){
    fprintf(stderr,"fopen failed\n");
    exit(1);
  }

  fprintf(fp2,"trial,func_cov\n");
  
  for(int i = 0; i < info->inputs_num; i++){

    int length = output_len + strlen(info->inputs[i]) + 6;
    char* path = (char*)malloc(sizeof(char)*length);
    sprintf(path,"%s/%s.csv",info->out_dir,info->inputs[i]);

    FILE* fp = fopen(path,"wb");

    if(fp == NULL)
    {
      fprintf(stderr,"fopen failed\n");
      exit(1);
    }
    
    fprintf(fp,"Function,hit\n");
    int max = MAX_NUM;
    for(int j = 0; j < MAX_NUM; j++){
      func_link p;
      for(p = func_individual[i][j]; p != NULL; p = p->next){
        fprintf(fp,"%s,%d\n",p->function_name,p->hit);
      }
    }
    fprintf(fp2,"%d,%d\n",i,uni_num[i]);
    fprintf(fp,"total,%d\n",func_num[i]);
    fclose(fp);
    free(path);
  }
  fclose(fp2);
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

  for(int i = 0; i < info->inputs_num; i++){
    printf("%d, %s\n",i,info->inputs[i]);
  }
  info_free(info);

  return 0; 
}
