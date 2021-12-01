#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>

struct sigaction *sa = NULL;

//wrapper function for waitpid to handle ECHILD errors
void wait_wrapper(pid_t pid) {
    if((waitpid(pid, NULL, 0)) == -1) {
        if(errno != ECHILD) {
            perror(strerror(errno));
            exit(1);
        }
    }
}

//Handler function for sigaction
//handles SIGINT and SIGCHLD for parent process
void my_handler(int signal) {
    pid_t pid;

    switch (signal) {
        case SIGINT:
            break;
        case SIGCHLD:
            pid = waitpid(-1, NULL, WNOHANG);
            while(pid && pid != -1) {
                if (errno != ECHILD) {
                    perror(strerror(errno));
                    exit(1);
                }
                pid = waitpid(-1, NULL, WNOHANG);
            }
            break;
        default:
            perror(strerror(errno));
            exit(1);
    }
}

//if process is a pipe, executes both children
//pipe the output of first child to the input of the second child
int piping(char **arglist, char **arglist2) {
    pid_t pidw, pidr;
    int pipefd[2], readerfd, writerfd;
    //check if pipe failed
    if(pipe(pipefd) == -1) {
        perror((strerror(errno)));
        return 0;
    }

    readerfd = pipefd[0];
    writerfd = pipefd[1];

    pidw = fork();
    //check if fork failed
    if(pidw == -1) {
        close(readerfd);
        close(writerfd);
        perror(strerror(errno));
        return 0;
    }

    if(pidw == 0) {
        //executing the first child, writing it's output into the pipe
        close(readerfd);
        //check if dup2 failed
        if(dup2(writerfd, 1) == -1) {
            perror(strerror(errno));
            exit(1);
        }
        //close(writerfd);
        execvp(arglist[0], arglist);
        close(writerfd);
        perror("Error in child process");
        exit(1);
    } else {
        //reading the pipe into the input and executing the second child
        close(writerfd);
        pidr = fork();
        //check if fork failed
        if(pidr == -1) {
            close(readerfd);
            close(writerfd);
            perror((strerror(errno)));
			wait_wrapper(pidw);
            return 0;
        }

        if (pidr == 0) {
            //check if dup2 failed
            if(dup2(readerfd, 0) == -1) {
                perror(strerror(errno));
                exit(1);
            }
            //close(readerfd);
            execvp(arglist2[0], arglist2);
            close(readerfd);
            perror("Error in child process");
            exit(1);
        } else {

            //parent waits for both children
            wait_wrapper(pidw);
            wait_wrapper(pidr);
            return 1;
        }

    }

}

//executes the command in the shell
int exec_command(int count, char **arglist) {
    pid_t pid;
    int background = 0;
    //check if the given command is a background command
    if(strcmp(arglist[count-1], "&") == 0) {
        background = 1;
        arglist[count-1] = NULL;
    }

    pid = fork();
    //check if fork failed
    if(pid == -1) {
        perror(strerror(errno));
        return 0;
    }

    //execute child process
    if (pid == 0) {
        if (background) {
            signal(SIGINT, SIG_IGN);
        }
        execvp(arglist[0], arglist);
        perror("Error in child process");
        exit(1);
    } else {
        if(!background) {
            //parent waits for child to finish
            wait_wrapper(pid);
        }
        return 1;
    }
}

//initialize sigaction struct to handle SIGINT and SIGCHLD
// also define the flag SA_RESTART to avoid EINTR errors
int prepare() {
	sa = (struct sigaction *)calloc(1, sizeof(struct sigaction));
	if(sa == NULL) {
		return 1;
	}

	sa->sa_handler = &my_handler;
	sa->sa_flags = SA_RESTART;
	if(sigaction(SIGINT, sa, NULL) == -1) {
	    perror(strerror(errno));
	}
	if(sigaction(SIGCHLD, sa, NULL) == -1) {
	    perror(strerror((errno)));
	}
	return 0;
}


//shell process
int process_arglist(int count, char **arglist) {
	//signify location of the in arglist if it exists
    int pipe_loc = 0, i;


    //check of a given command contains pipe
    for(i = 1; i < count - 1; ++i) {
        if(strcmp(arglist[i], "|") == 0) {
            pipe_loc = i;
            break;
        }
    }
    if(pipe_loc) {
        arglist[pipe_loc] = NULL;
        return piping(arglist, arglist + pipe_loc + 1);
    } else {
        return exec_command(count, arglist);
    }
}


int finalize() {
    if(sa != NULL) {
        free(sa);
    }
	return 0;
}
