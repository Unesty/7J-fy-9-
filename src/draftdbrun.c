#include "db_parser.c" // header TODO

int main(int argc, char** argv, char** envp) {
    int ret = open_db(argv[1]);
    return ret;
}
