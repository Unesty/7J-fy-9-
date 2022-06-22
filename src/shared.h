#include <stdbool.h>
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
enum Inputs {
    inp_none,
    inp_right,
    inp_left,
};
struct SharedMem {
//////////////////////////////////////////
// ui_logic

    struct DBFileValues {
        FILE* f;
        struct stat statbuf; // unix-like specific
        char** buf;
    }dv;
// end ui_logic
//////////////////////////////////////////

//////////////////////////////////////////
// communication with other processes


    enum Inputs input;

    bool run;

    struct Pids pids;
//
//////////////////////////////////////////
}*shm;

char inpfd;
char* inpname;
