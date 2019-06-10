#include <unistd.h>
#include <stdio.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define GRN   "\x1B[1;32m"
#define BLU   "\x1B[1;34m"
#define RESET "\x1B[0m"

#define MAX_COMMAND 1024

void mypipe(char * , char *);
void redirect(char, char *,char *);
int isInPipe(char*);
char isInRedirection(char*);
void parser(char*);
char ** commandSeperator(char *);
int wcDetecter(char *);
int cd(char *);
void help();
char *lastWScleaner(char*);
void handler(int signo);

char pathOfUtilty [MAX_COMMAND];
char ** history;

int main(int argc, char  **argv){
	char  command [MAX_COMMAND], path[MAX_COMMAND], howBack[MAX_COMMAND];
	int first=0,i=0,strToInt;

	signal(SIGINT,handler);
	signal(SIGTSTP,handler);
	signal(SIGTERM,handler);

	char ** history = (char **)malloc(1000 * sizeof(char *)); 		//history can holds 1000 old commands
    for (int j=0; j<1000; ++j) 
         history[j] = (char *)malloc(MAX_COMMAND * sizeof(char)); 	

	while(1){
		if(getcwd(path, sizeof(path)) == NULL){
			perror("getcwd() error");
			exit(EXIT_FAILURE);
		}
		if(first == 0){
			strcpy(pathOfUtilty,path);
			first=1;
		}

		printf(GRN "gtushell:" RESET);
		printf(BLU "~%s$ " RESET,path );
		fgets(command, MAX_COMMAND, stdin);
		if(command[strlen(command)-1] == '\n')
			command[strlen(command)-1] = '\0';

		if(strlen(command) != 0 && command[0] == '!'){
			for (int k = 1; k < strlen(command); ++k){
				howBack[k-1] = command[k];
			}
			strToInt = strtol(howBack, NULL, 10);
			if(i>=strToInt)
				strcpy(command,history[i-strToInt]);
			else
				printf("Your shell history doesn't include %d command yet!\n",strToInt );
		}
		

		if(strlen(command) != 0){
			strcpy(history[i],command);
			++i;
			if(i == 1000)
				i=0; 	//restart history
		}

		if(strcmp(command,"exit") == 0)
			break;
		else if(strcmp(command,"help") == 0)
			help();
		else if(!cd(command))
			parser(command);
	}

	for (int j=0; j<1000; ++j) 
		free(history[j]);
	free(history);
	return 0;
}

void handler(int signo){
	if(signo == SIGINT)
		printf("  SIGINT(^C) handled!\n");

	else if(signo == SIGTSTP)
		printf("  SIGTSTP(^Z) handled!\n");
		
	else if(signo == SIGTERM)
		printf("  SIGTERM handled!\n");

	kill(0,SIGKILL);
}

void help(){
	printf("This is gtushell\n");
    printf("Supported commands are:\n");

    printf("cd : Changes directory \n");
    printf("cd has only one argument (path) .\n");
    printf("Example usages: cd .. ,  cd ./someDirectory\n");

    printf("pwd : Display current working directory \n");
    printf("pwd has no argument.\n");

    printf("cat : display given output of given argument\n");
    printf("cat has only one argument (file) .\n");
    printf("Example usage : cat grades.txt\n");

    printf("lsf: only show files(regular or special) in current directory\n");
    printf("lsf has no argument.\n");

    printf("wc : gives line number of given argument\n");
    printf("wc has only one argument (file) .\n");
    printf("Example usage : wc grades.txt\n");

    printf("bunedu : gives sizes of given directory and its subdirectories\n");
    printf("bunedu has two argument (-z flag) (path) .\n");
    printf("-z flags adds subdirectories sizes to parent directories");
    printf("Example usages : bunedu -z path , bunedu path\n");

    printf("> : redirect standart output to given file \n");
    printf("Example usage : bunedu -z path > output.txt\n");

    printf("< : redirect standart input to program \n");
    printf("Example usage : bunedu -z path < input.txt\n");

    printf("| : Pipe operation gives standart output of left to standart input of right\n");
    printf("Example usage : lsf | wc \n");

    printf("help: print help page \n");
   
    printf("exit : exit gtushell \n");
}

int cd(char * newpath){
	int returnFlag=0;
	char * arg1 = (char*)malloc(strlen(newpath)*sizeof(char));
	char * arg2 = (char*)malloc(strlen(newpath)*sizeof(char));

	sscanf(newpath,"%s %s",arg1,arg2);

	if(strcmp(arg1,"cd")==0){
		if(chdir(arg2) == -1)
			printf("cd: %s: No such file or directory\n", arg2);

		returnFlag = 1;
	}

	free(arg1);
	free(arg2);
	return returnFlag;
}

char *lastWScleaner(char* ch){
	int i,j,key=0;
	char tmp;
	char * result = (char*)malloc(strlen(ch)*sizeof(char));
	
    for (i = strlen(ch)-1, j=0 ; i>=0 ; --i){      // cleaned white spaces end of string
        if(ch[i] == ' '){
            if(key == 1){       
                result[j] = ch[i];
                ++j;
            }
        }
        else{
            result[j] = ch[i];
            ++j;
            key = 1;
        }
    }
    result [j] = '\0';

    for (i = 0 , j=strlen(result)-1 ; i < strlen(result)/2 ; ++i,--j){
    	tmp = result[j];
    	result[j] = result[i];
    	result[i] = tmp;
    }

    return result;
}

void mypipe(char * L , char * R){
	int fd[2] , pid ,stat;
	char arg0 [MAX_COMMAND], ** argumentsL, ** argumentsR ;

	if(pipe(fd)<0){
		perror("Could not open pipe");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if(pid == 0){
		argumentsL = commandSeperator(L);
		if( (strcmp(argumentsL[0],"wc") != 0)  &&
			(strcmp(argumentsL[0],"bunedu") != 0) &&
			(strcmp(argumentsL[0],"lsf") != 0) &&
			(strcmp(argumentsL[0],"cat") != 0) &&
			(strcmp(argumentsL[0],"pwd") != 0) 
		  ){
			printf("Pipe's write end can only use with wc,bunedu,lsf,cat and pwd utilities.\n");
			for (int k=0; k<3; ++k)
				free(argumentsL[k]);
			free(argumentsL);
			exit(37);
		}

		if(close(fd[0]) == -1){
			perror("Could not close read end of pipe");
			exit(EXIT_FAILURE);
		}

		if(fd[1] != STDOUT_FILENO){
			if(dup2(fd[1],STDOUT_FILENO) == -1){
				perror("dup2");
				exit(EXIT_FAILURE);
			}
			if(close(fd[1]) == -1){
				perror("Could not close write end of pipe");
				exit(EXIT_FAILURE);
			}
		}
		
		strcpy(arg0,pathOfUtilty);
		strcat(arg0,"/");
		strcat(arg0,argumentsL[0]);
		if(execv(arg0,argumentsL) == -1){
			perror("command not found\n");
			exit(EXIT_FAILURE);
		}
		
	}

	else if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}

	wait(&stat);
	if (WEXITSTATUS(stat) == 37)		// if left side of | invalid dont execute right side
		return;
	

	pid = fork();
	if(pid == 0){
		argumentsR = commandSeperator(R);
		if( (strcmp(argumentsR[0],"wc") != 0)  &&
			(strcmp(argumentsR[0],"bunedu") != 0) &&
			(strcmp(argumentsR[0],"cat") != 0)
		  ){
			printf("Pipe's read end can only use with wc,bunedu and cat utilities.\n");
			for (int k=0; k<3; ++k)
				free(argumentsR[k]);
			free(argumentsR);

			exit(EXIT_FAILURE);
		}

		if(close(fd[1]) == -1){
			perror("Could not close read end of pipe");
			exit(EXIT_FAILURE);
		}

		if(fd[1] != STDOUT_FILENO){
			if(dup2(fd[0],STDIN_FILENO) == -1){
				perror("dup2");
				exit(EXIT_FAILURE);
			}
			if(close(fd[0]) == -1){
				perror("Could not close write end of pipe");
				exit(EXIT_FAILURE);
			}
		}
		
		strcpy(arg0,pathOfUtilty);
		strcat(arg0,"/");
		strcat(arg0,argumentsR[0]);
		if(execv(arg0,argumentsR) == -1){
			perror("command not found\n");
			exit(EXIT_FAILURE);
		}

	}

	else if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}

	wait(NULL);
}

void parser(char* command){
	int i,j,pid,key=0;
	char inout;
	char arg0 [MAX_COMMAND];	// path + arg0
	char left [MAX_COMMAND], right [MAX_COMMAND], *cleanLeft, *cleanRight ;	
	char **argumentSingle;
	if(isInPipe(command)){
		for (i = 0, j = 0 ; i < strlen(command); ++i){	//build left of pipe
			if(command[i] != '|'){
				left[j] = command [i];	
				++j;
			}
			else{
				left[j] = '\0';	 //null
				++i;
				break;
			}
		}

		for (j = 0; i < strlen(command); ++i){ 	//build right of pipe
			right[j] = command[i];
			++j;
		}
		right[j] = '\0'; 	//null
		
		cleanLeft = lastWScleaner(left);
		cleanRight = lastWScleaner(right);

		mypipe(cleanLeft,cleanRight);

		free(cleanRight);
		free(cleanLeft);		
	}
	else if((inout = isInRedirection(command)) != '?'){
		
		for (i = 0, j = 0 ; i < strlen(command); ++i){	//build left of redirection
			if(command[i] != inout){
				left[j] = command [i];	
				++j;
			}
			else{
				left[j] = '\0';  //null
				++i;
				break;
			}
		}

		for (j = 0; i < strlen(command); ++i){ 	//build right of redirection
			if(command[i] == ' '){
				if(key == 1){		// ignore ws beginning of filename
					right[j] = command[i];
					++j;
				}
			}
			else{
				right[j] = command[i];
				++j;
				key = 1;
			}
		}
		right[j] = '\0';

		cleanRight = lastWScleaner(right);
		cleanLeft = lastWScleaner(left);

		redirect(inout,cleanLeft,cleanRight);

		free(cleanRight);
		free(cleanLeft);
	
	}
	else{
		pid = fork();
		if(pid == 0){
			argumentSingle = commandSeperator(command);
			strcpy(arg0,pathOfUtilty);
			strcat(arg0,"/");
			strcat(arg0,argumentSingle[0]);
			if(execv(arg0,argumentSingle) == -1){
				printf("command not found\n" );
				exit(EXIT_FAILURE);
			}
		}
		else if(pid == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}
	
		wait(NULL);
	}	
}


char ** commandSeperator(char * command){
	int  excluding, key=0;
	char ** result = (char **)calloc(3 , sizeof(char *)); 	//command [3][MAX_COMMAND] can be 3 argument max (for bunedu -z blalba)

    for (int j=0; j<3; ++j) 
         result[j] = (char *)calloc(MAX_COMMAND , sizeof(char)); 	//each argument can be MAX_COMMAND characters max

    
    sscanf(command,"%s %s",result[0],result[1]);
    
    excluding = strlen(result[0]) + strlen(result[1]) + wcDetecter(command);
    
    if(strlen(result[0]) == 0){		//argv 0 empty
    	result[0] = (char *) NULL;	//null for exec
    }
    else if(strlen(result[1]) == 0){	//argv 1 empty
    	result[1] = (char *) NULL;	//null for exec	
    }
    else if( strlen(command) - excluding > 0){	//argv 2 not empty
    	for (int i = excluding , j=0 ; i < strlen(command); ++i){
    		if(command[i] == ' '){			//clean white space befero filename 
    			if(key != 0){
    				result[2][j] =  command[i];
    				++j;
    			}
    		}
    		else{
    			key = 1;
    			result[2][j] =  command[i];
    			++j;
    		}
    	}
    	strcat(result[2],"\0"); //null
    	result[3] = (char *) NULL;  //null for exec
    }
    else{	//argv 2 empty
    	result[2] = (char *) NULL;	//null for exec
	}

    return result;
}

														
int wcDetecter(char * piece){
	int count=0,key=0,until=0;
	for (int i = 0; i < strlen(piece) && until != 3; ++i){
		if(piece[i] == ' '){
			++count;
			key = 0 ;
		}
		else if(key == 0){
			++until;
			key = 1;
		}
	}
	return count;
}

int isInPipe(char* command){
	for (int i = 0; i < strlen(command); ++i)
		if(command[i] == '|')
			return 1;
	return 0; 
}

char isInRedirection(char* command){
	for (int i = 0; i < strlen(command); ++i){
		if(command[i] == '>')
			return '>';
		if(command[i] == '<')
			return '<';
	}
	return '?'; 
}


void redirect(char inout, char * left,char *filename){
	int pid;
	char arg0[MAX_COMMAND], ** args;
	FILE * fptr;
	if(inout == '>'){
		pid = fork();
		if(pid == 0){
			args = commandSeperator(left);
			if( (strcmp(args[0],"wc") != 0)  &&
				(strcmp(args[0],"bunedu") != 0) &&
				(strcmp(args[0],"lsf") != 0) &&
				(strcmp(args[0],"cat") != 0) &&
				(strcmp(args[0],"pwd") != 0) 
			  ){
				printf(" > can only use with wc,bunedu,lsf,cat and pwd utilities.\n");
				for (int k=0; k<3; ++k)
					free(args[k]);
				free(args);
				exit(EXIT_FAILURE);
			}

			fptr = fopen(filename,"w");
			if(fptr == NULL){
				printf("%s could not open!\n", filename);
				exit(EXIT_FAILURE);
			}

			if(fileno(fptr) != STDOUT_FILENO){
				if(dup2(fileno(fptr),STDOUT_FILENO) == -1){
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			strcpy(arg0,pathOfUtilty);
			strcat(arg0,"/");
			strcat(arg0,args[0]);
			if(execv(arg0,args) == -1){
				perror("command not found\n" );
				
				exit(EXIT_FAILURE);
			}
		}

		else if(pid == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		wait(NULL);	//parent wait child

	}
	else{	// <
		pid = fork();
		if(pid == 0){
			args = commandSeperator(left);
			if( (strcmp(args[0],"wc") != 0)  &&
				(strcmp(args[0],"bunedu") != 0) &&
				(strcmp(args[0],"lsf") != 0) &&
				(strcmp(args[0],"cat") != 0) &&
				(strcmp(args[0],"pwd") != 0) 
			  ){
				printf(" < can only use with wc,bunedu,lsf,cat and pwd utilities.\n");
				for (int k=0; k<3; ++k)
					free(args[k]);
				free(args);
				exit(EXIT_FAILURE);
			}

			fptr = fopen(filename,"r");
			if(fptr == NULL){
				printf("%s could not open!\n", filename);
				exit(EXIT_FAILURE);
			}

			if(fileno(fptr) != STDIN_FILENO){
				if(dup2(fileno(fptr),STDIN_FILENO) == -1){
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			strcpy(arg0,pathOfUtilty);
			strcat(arg0,"/");
			strcat(arg0,args[0]);
			if(execv(arg0,args) == -1){
				perror("command not found\n");
				exit(EXIT_FAILURE);
			}
		}

		else if(pid == -1){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		wait(NULL);	//parent wait child
	}
}

