#include "systemcalls.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

bool do_system(const char *cmd)
{
    int ret = system(cmd);
    if (ret == -1 || WEXITSTATUS(ret) != 0)
        return false;
    return true;
}

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    fflush(stdout);  // avoid duplicate output after fork
    pid_t pid = fork();
    if (pid == -1) {
        va_end(args);
        return false;
    }
    if (pid == 0) {
        // child process
        execv(command[0], command);
        exit(1);  // only reached if execv fails
    }
    // parent process
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        va_end(args);
        return false;
    }

    va_end(args);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    fflush(stdout);
    pid_t pid = fork();
    if (pid == -1) {
        va_end(args);
        return false;
    }
    if (pid == 0) {
        // child: redirect stdout to outputfile
        int fd = open(outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if (fd == -1) exit(1);
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execv(command[0], command);
        exit(1);
    }
    // parent
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        va_end(args);
        return false;
    }

    va_end(args);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
