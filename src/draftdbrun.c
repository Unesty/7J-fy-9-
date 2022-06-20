#include "db_parser.c" // header TODO

int main(int argc, char** argv, char** envp) {
    int ret = open_db(argv[1]);
    if(ret == 0) {
        ret = run_db_7(0);
    } else {
        printf("exiting\n");
    }
    return ret;
}
