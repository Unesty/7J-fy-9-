#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <stdbool.h>
#include <math.h>
//////////////////////////////////////////////
//// globals
#include "shared.h"

long time = 0;
int sleep_time = 1;
struct timeval stopt, startt, overallt;
//////////////////////////////////////////
// ui_logic
struct GameObject gos[] = {{player, 0., 100, "player.glb"}, {enemy, 10.,0, "enemy.glb"}};

float player_speed = 0.1;
// end ui_logic
//////////////////////////////////////////

//// end globals
//////////////////////////////////////////////

/**
 * Cross-platform sleep function for C
 * @param int milliseconds
 */
void sleep_ms(int milliseconds)
{
    #ifdef WIN32
        Sleep(milliseconds);
    #elif _POSIX_C_SOURCE >= 199309L
        struct timespec ts;
        ts.tv_sec = milliseconds / 1000;
        ts.tv_nsec = (milliseconds % 1000) * 1000000;
        nanosleep(&ts, NULL);
    #else
        usleep(milliseconds * 1000);
    #endif
}

void io_init() {
    printf("shm init\n");
    inpfd = shm_open(inpname, O_RDWR, S_IRUSR | S_IWUSR);
    if (inpfd == -1) {
        printf("can't open shm %s\n", inpname);
    }
    shm = (struct SharedMem*)mmap(0, sizeof(struct SharedMem), PROT_READ|PROT_WRITE, MAP_SHARED, inpfd, 0); // somehow it works without shmopen
    if (shm == MAP_FAILED) {
        printf("mmap failed\n");
    }
    shm->pids.ui_logic = getpid();
    printf(" %d \n", shm->pids.ui_logic);
}

void ui_logic_init() {
    // spawn objects
//     shm->gos[0].pos = 0.;
}

void ui_logic_loop() {
    printf("\n");
    while(shm->run == true) {
        if(shm->input == inp_left) {
            shm->gos[0].pos -= player_speed;
        }
        else if(shm->input == inp_right) {
            shm->gos[0].pos += player_speed;
        }
        // crutch because of no release input events
//         {
//             shm->input = inp_none;
//         }
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b%f", shm->gos[0].pos);
        sleep_ms(sleep_time);
    }
}

int main(int argc, char** argv, char** enpv) {
	inpname = argv[1];
    io_init();
    printf("gmpid %d\n", shm->pids.ui_logic);
    printf("grpid %d\n", shm->pids.graphics);
    ui_logic_loop();
}

