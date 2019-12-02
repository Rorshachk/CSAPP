#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAXLINE 8192
#define MAXARGS 128

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

int parseline(char *buf, char **argv){
    char *delim;
    int argc, bg;
    buf[strlen(buf)-1] = ' ';
    while(*buf && (*buf == ' ')) buf++;

    argc = 0;
    while ((delim = strchr(buf, ' '))){
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))
          buf++;
    }
    argv[argc] = NULL;
    if(argc == 0) return 1;
    if((bg = (*argv[argc-1] == '&')) != 0) argv[--argc] = NULL;
    
    return bg;
}

int builtin_command(char **argv){
    if(!strcmp(argv[0], "quit")) exit(0);
    if(!strcmp(argv[0], "&")) return 1;
    return 0;
}

void eval(char *cmdLine){
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdLine);
    bg = parseline(buf, argv);
    if (argv[0] == NULL) return ;

    if(!builtin_command(argv)){
        if((pid = fork()) == 0){
            if(execve(argv[0], argv, __environ) < 0){
                printf("%s: Command not found .\n", argv[0]);
                exit(0);
            }
        }

        if(!bg){
            int status;
            if(waitpid(pid, &status, 0) < 0)
              unix_error("waitfg: waitpid error");
        }else printf("%d %s", pid, cmdLine);
    }
    return ;
}

int main(){
    char cmdLine[MAXLINE];
    while(1){
        printf("> ");
        fgets(cmdLine, MAXLINE, stdin);
        if(feof(stdin)) exit(0);
        eval(cmdLine);
    }
}