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
    // database values
    struct DBFileValues {
        struct stat statbuf; // unix-like specific
    }dv;

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
    // communication with other processes

//     char gris_name[10];
    char ids_name[10];

    bool run;

    struct Pids pids;
    //
    //////////////////////////////////////////
}*shm;



char inpfd;
char* inpname;
