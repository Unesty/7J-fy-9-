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

#include <dlg/dlg.h>

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
    dlg_debug("kill -%d signal handled\n", signum);
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
        dlg_debug("can't open shm %s\n", inpname);
        //TODO: generate new name
    }
    if (ftruncate(inpfd, sizeof(struct SharedMem)) == -1) {
        dlg_debug("can't truncate shm\n");
    }
    shm = (struct SharedMem*)mmap(0, sizeof(struct SharedMem), PROT_READ|PROT_WRITE, MAP_SHARED, inpfd, 0);
    shm->run = true;
    if (shm == MAP_FAILED) {
        dlg_debug("mmap failed\n");
    }
//     pids = (struct Pids*)mmap(0, sizeof(struct Pids), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, inpfd, 0);
    // run ui_logic
//     if(fork() == 0) {
//         char* newargv[] = {"ui_logic", inpname, NULL};
//         execvpe("src/ui_logic", newargv, envp);
//         dlg_debug("failed to open ui_logic executable\n");
//         sleep(100);
// //         pids.ui_logic =
//     }
    // run db server
    if(fork() == 0) {
        // TODO to supervise more than 1 process, waitpid on another thread
        char* newargv[] = {"db_server", inpname, "/bunny/7J-fy-9-/databases/example.graph", NULL};
        execvpe("src/db_server", newargv, envp);
        dlg_debug("failed to open db_server executable\n");
        sleep(100);
//         pids.graphics =
    }
    // r
    bool restart = false;
    // run graphics
//     sleep(100000); // to run it separately
    while(shm->run == true) {
        if(restart)
            dlg_debug("restarting graphics\n");
        if(fork() == 0) {
            char* newargv[] = {"graphics", inpname, NULL};
            execvpe("src/graphics_gl", newargv, envp);
            dlg_debug("failed to open opengl graphics executable\n");
            sleep(100);
    //         pids.graphics =
        }
        int wstatus;
        if(shm->pids.graphics == 0) {
            dlg_debug("no opengl graphics process pid\n");
            sleep(1);
            if(shm->pids.graphics == 0) {
                break;
            }
        }
        sleep(1); // TODO: loop intil process appears
        waitpid(shm->pids.graphics, &wstatus, 0);
        int ret = WEXITSTATUS(wstatus);
        dlg_debug("WEXITSTATUS = %d\n", wstatus);
        if(wstatus!=0) {
//             shm->run = false;
            dlg_debug("opengl graphics process failed\n");
            break;
        } else {
            dlg_debug("opengl graphics process ended successfully\n");
            restart = true;
        }
    }
    // run sound

//         pids.sound =
    dlg_debug("grpid = %d\n", shm->pids.graphics);
//     wait(&shm->pids.ui_logic);
//     wait(&shm->pids.sound);
    wait(&shm->pids.db_server);
    // clean up
    shm_unlink(inpname);
    close(inpfd);
}
