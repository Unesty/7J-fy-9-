#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <math.h>
#include "shared.h"

// signal handlers

void sig_handler(int signum) {
    if(!kill(shm->pids.ui_logic, 0)) {
        kill(shm->pids.ui_logic, SIGTERM);
    }
    if(!kill(shm->pids.db_server, 0)) {
        kill(shm->pids.db_server, SIGTERM);
    }
    if(!kill(shm->pids.sound, 0)) {
        kill(shm->pids.sound, SIGTERM);
    }
    if(!kill(shm->pids.graphics, 0)) {
        kill(shm->pids.graphics, SIGTERM);
    }
    shm_unlink(inpname);
    close(inpfd);
    wait(&shm->pids.graphics);
//     wait(&shm->pids.ui_logic);
//     wait(&shm->pids.sound);
    wait(&shm->pids.db_server);
    printf("kill -%d signal handled\n", signum);
    exit(signum);
}

// we can add child process signal handlers

// TODO: supervision
// struct Pids {
//     int* ui_logic, graphics, sound;
// }*pids;

int main(int argc, char** argv, char** envp) {
    // signal handlers
    signal(SIGINT, sig_handler); // Register signal handler
    signal(SIGQUIT, sig_handler); // Register signal handler
    signal(SIGHUP, sig_handler); // Register signal handler
    // signal(SIGSEGV, sig_handler); // Register signal handler
    signal(SIGTERM, sig_handler); // Register signal handler
    // init shm
    inpname = "/furrychainsaw_input0";
    inpfd = shm_open(inpname, O_CREAT|O_EXCL|O_RDWR, S_IRUSR | S_IWUSR);
    if (inpfd == -1) {
        printf("can't open shm %s\n", inpname);
        //TODO: generate new name
    }
    if (ftruncate(inpfd, sizeof(struct SharedMem)) == -1) {
        printf("can't truncate shm\n");
    }
    shm = (struct SharedMem*)mmap(0, sizeof(struct SharedMem), PROT_READ|PROT_WRITE, MAP_SHARED, inpfd, 0);
    shm->run = true;
    if (shm == MAP_FAILED) {
        printf("mmap failed\n");
    }
//     pids = (struct Pids*)mmap(0, sizeof(struct Pids), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, inpfd, 0);
    // run ui_logic
//     if(fork() == 0) {
//         char* newargv[] = {"ui_logic", inpname, NULL};
//         execvpe("src/ui_logic", newargv, envp);
//         printf("failed to open ui_logic executable\n");
//         sleep(100);
// //         pids.ui_logic =
//     }
    // run db server
    if(fork() == 0) {
        char* newargv[] = {"db_server", inpname, "/bunny/7J-fy-9-/databases/example.graph", NULL};
        execvpe("src/db_server", newargv, envp);
        printf("failed to open db_server executable\n");
        sleep(100);
//         pids.graphics =
    }
    // r
    // run graphics
    sleep(100000); // to run it separately
    while(true) {

    if(fork() == 0) {
        char* newargv[] = {"graphics", inpname, NULL};
        execvpe("src/graphics", newargv, envp);
        printf("failed to open graphics executable\n");
        sleep(100);
//         pids.graphics =
    }
    int wstatus;
    if(shm->pids.graphics == 0) {
        printf("no graphics process pid\n");
        sleep(1);
        if(shm->pids.graphics == 0) {
            break;
        }
    }
    waitpid(shm->pids.graphics, &wstatus, 0);
    int ret = WEXITSTATUS(wstatus);
    printf("WEXITSTATUS = %d\n", wstatus);
    if(wstatus!=0) {
        shm->run = false;
        break;
    } else {
        shm->run = true;
    }
    }
    // run sound

//         pids.sound =
    printf("grpid = %d\n", shm->pids.graphics);
//     wait(&shm->pids.ui_logic);
//     wait(&shm->pids.sound);
    wait(&shm->pids.db_server);
    // clean up
    shm_unlink(inpname);
    close(inpfd);
}
