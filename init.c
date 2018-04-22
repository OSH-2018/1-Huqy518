#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

//char *env[128] = {"TERM", "SHELL", "USER", "LS_COLORS", "SUDO_USER", "SUDO_UID", "USERNAME", "MAIL", "PATH", "PWD", "LANG", "SHLVL", "SUDO_COMMAND", "HOME", "LANGUAGE", "LESSOPEN", "SUDO_GID", "DISPLAY", "LESSCLOSE", "XAUTHORITY", "OLDPWD"};


void export_env(char *args){
	char *delim = "=";	//分隔符 
	char *p1, *p2;
	p1 = strtok(args, delim);
	p2 = strtok(NULL, delim);
	setenv(p1, p2, 1); 
}


int pipe_test(char *args[]){
	int i, count = 0;
	for(i = 0; args[i]; i ++){
		if(strcmp(args[i], "|") == 0)
			count = count + 1;
	}
	return count;
}


int main(){
	char cmd[256];
	char *args[128];
	while(1){
		printf("#");
		fflush(stdin);
		fgets(cmd, 256, stdin);
		int i;
		int pipe_num = 0;
		int flag_output = 0, flag_input = 0;	//检测重定向 
		for(i = 0; cmd[i] != '\n'; i ++);
		cmd[i] = '\0';
		args[0] = cmd;
		for(i = 0; *args[i]; i ++){
			for(args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
				if(*args[i+1] == ' '){
					*args[i+1] = '\0';
					args[i+1]++;
					break;
				}
		}
		args[i] = NULL;
		
		if(!args[0])	//没有输入命令
			continue;
		if(strcmp(args[0], "cd") == 0){
			if(args[1])
				chdir(args[1]);
			continue;
		}	
		if(strcmp(args[0], "pwd") == 0){
			char wd[4096];
			puts(getcwd(wd, 4096));
			continue;
		}
		if(args[1] && strcmp(args[1], "|") == 0){
			pipe_num = pipe_test(args);
			printf("pipe_num is %d\n", pipe_num);
		}
		
		if(strcmp(args[0], "exit") == 0){
			return 0;
		}
		if(strcmp(args[0], "export") == 0){
			export_env(args[1]);
			continue;
		}
		if(args[1] && args[2] && strcmp(args[1], ">") == 0){
			printf("输出重定向\n");
			flag_output = 1;			
		} 
		if(args[1] && args[2] && strcmp(args[1], "<") == 0){
			printf("输入重定向\n");
			flag_input = 1;
		}
		
		int cmd_num = pipe_num + 1;
		int pipe_fd[10][2];
		int j;
		int fd_in, fd_out;
		pid_t pid;
		for(i = 0; i < cmd_num; ++i) {  
      		if((pid = fork()) < 0) {  
        		printf("fork fail!\n");  
        		exit(1);  
      		}  
      		if(pid == 0){  
        	break;  
      		}  
    	}  
  
    /* 有多少个命令就会执行多少个子进程，最终调用exec函数族 */  
    	if(pid == 0){  
      /* 上面循环中，子进程break，所以执行下面的语句，此时i就和循环变量i一样 */  
      		if(flag_input) {  
        /* 重定向输入 */  
        		if((fd_in = open(args[2], O_RDONLY)) < 0){  
          			perror("open fail!\n");  
        		}  
        	dup2(fd_in, STDIN_FILENO);  
        	close(fd_in);  
      		}	  
  
      			if(flag_output){  
        /* 重定向输出 */  
     			   if((fd_out = open(args[2], O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0){  
          				perror("open fail!\n");  
        			}  
       			dup2(fd_out, STDOUT_FILENO);  
        		close(fd_out);  
      			}  
      
      
      if(pipe_num){  
        /* 第一个子进程，读入写出，关闭读端，把标准输出重定向到写端*/  
        if(0 == i){  
          close(pipe_fd[i][0]);  
          dup2(pipe_fd[i][1], STDOUT_FILENO); // 本来执行结果是在标准输出上  
          close(pipe_fd[i][1]);  
  
          /* 关闭掉多余的管道 */  
          for(j = 1; j < pipe_num; ++j){  
            close(pipe_fd[j][0]);  
            close(pipe_fd[j][1]);  
          }  
        }  
        /* 最后一个子进程，关闭写端，把标准输入重定向到读端 */  
        else if(pipe_num == i){  
          close(pipe_fd[i - 1][0]);  
          dup2(pipe_fd[i - 1][0], STDIN_FILENO);  
          close(pipe_fd[i - 1][0]);  
          /* 关闭掉多余的管道读写端 */  
          for(j = 0; j < pipe_num - 1; ++j){  
            close(pipe_fd[j][0]);  
            close(pipe_fd[j][1]);  
          }  
        }  
        /* 1~pipe_num-1, */  
        else {
          dup2(pipe_fd[i - 1][0], STDIN_FILENO);  
          close(pipe_fd[i - 1][0]);  
  
          dup2(pipe_fd[i][1], STDOUT_FILENO);  
          close(pipe_fd[i][1]);  
  
          for(j = 0; j < pipe_num; ++j) {  
            if((j != i - 1) || j != i) {  
              close(pipe_fd[j][0]);  
              close(pipe_fd[j][1]);  
            }  
          }  
        }  
      }  
      
      
      
      
      /* 管道是进程间通信的一种方式，输入命令中有管道 */ 
	if(flag_input || flag_output){
		args[1] = args[2];
		args[2] = NULL;
	}
	if(pipe_num){
		for(int i = 0; args[i]; i ++){
			if(args[i] && strcmp(args[i], "|") == 0)
				args[i] = args[i+1];
		}
		args[i] = NULL;
	} 
		execvp(args[0], args);
    return 255; 
    }  
  
      /* arg第1个参数是命令，后面的参数是命令选项如:-l */    
    
    else {// 父进程阻塞    
      for(i = 0; i < pipe_num; ++i){  
        close(pipe_fd[i][0]);  
        close(pipe_fd[i][1]);  
      }  
      for(i = 0; i < cmd_num; ++i)  
        
        wait(NULL);  
        
    }	  
   }  		
	return 0;
}
