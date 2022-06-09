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

// we can add child process signal handlers

// TODO: supervision
// struct Pids {
//     int* ui_logic, graphics, sound;
// }*pids;

int main(int argc, char** argv, char** envp) {
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
    if(fork() == 0) {
        char* newargv[] = {"ui_logic", inpname, NULL};
        execvpe("src/ui_logic", newargv, envp);
        printf("failed to open ui_logic executable\n");
        sleep(100);
//         pids.ui_logic =
    }
    // run graphics
    if(fork() == 0) {
        char* newargv[] = {"graphics", inpname, NULL};
        execvpe("src/graphics", newargv, envp);
        printf("failed to open graphics executable\n");
        sleep(100);
//         pids.graphics =
    }
    // run sound

//         pids.sound =
    // wait
    sleep(1);
    printf("%d", shm->pids.graphics);
    wait(&shm->pids.graphics);
    wait(&shm->pids.ui_logic);
    // clean up
    shm_unlink(inpname);
    close(inpfd);
}
