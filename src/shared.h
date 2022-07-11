#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h> // TODO: support other than unix-like

enum GOType {
    player,
    enemy,
    interactive,
};
struct GameObject {
    enum GOType type;
    float pos;
    bool hidden;
    char model[30];
};
struct Pids {
    int ui_logic, graphics, sound, db_server;
};



struct SharedMem {
    //////////////////////////////////////////
    // ui_logic


    //
    //////////////////////////////////////////



    //////////////////////////////////////////
    // IO
    struct{
        uint8_t reload_db;
        uint8_t lmb;
    }keys;

    float iCamPos[2];
    struct{
        float dx; // position difference
        float dy;
        float cx; // cursor position
        float cy;
    }mouse;

    //
    //////////////////////////////////////////

    //////////////////////////////////////////
    // shared graphs metadata
    struct GraphInfo {
        uint32_t id;
        uint32_t start;
        uint32_t len;
        uint32_t vertexcnt;
        uint32_t type;
        uint32_t idshifts_len;
        uint32_t idshifts_off;
    }gri;
    //
    //////////////////////////////////////////

    //////////////////////////////////////////
    // communication with other processes

//     char gris_name[10];
//     char ids_name[10];
    char db_name[256]; // will be multiple names in future

    bool run;

    struct Pids pids;
    //
    //////////////////////////////////////////
}*shm;

struct DBFileValues {
    struct stat statbuf; // unix-like specific
}dv;
// Now stored inside db file.
// Each process has it's own pointer to it.
// Can also be accessed as &buf[shm->gri.idshifts_off].
uint32_t *idshifts;

//gris = mmap(sizeof(struct GraphInfo)); // later, or never if saving this in
uint32_t gris_len = 1;
// uint32_t grlen = 0;
uint32_t grnl = 4; // sizeof(uint32_t) graph type7 data array element length in bytes
uint32_t *buf;

uint8_t db_fds[100]; // to open multiple database files to connect them
uint32_t db_sz = 0;

char inpfd;
char* inpname;
