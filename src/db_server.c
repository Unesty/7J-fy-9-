#include "db_parser.c" // header TODO
#include <sys/wait.h>

int main(int argc, char **argv, char **envp) {
    inpname = argv[1];
    int ret = open_db(argv[2]);
    if(ret == 0) {
        int pid = fork();
        if(pid == 0) {
            ret = run_db_7(0);
        } else {
            int wstatus;
            waitpid(pid, &wstatus, 0); // Store proc info into wstatus
            ret = WEXITSTATUS(wstatus); // Extract return value from wstatus
        }
    } else {
        printf("exiting\n");
    }
    return ret;
}
