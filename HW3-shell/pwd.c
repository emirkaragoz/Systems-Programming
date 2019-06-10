#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

void pwd();

int main(int argc, char *argv[]){
    if(argc == 1)
	   pwd();
    else{
        printf("Invalid usage! It must be './pwd'\n");
        exit(EXIT_FAILURE);
    }
	return 0;
}

void pwd(){
    char *path = (char*)malloc( (1024)*sizeof(char));
    if (getcwd(path, (1024)*sizeof(char)) != NULL) 
        printf("%s\n", path);
    else{
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }
    free(path);
}