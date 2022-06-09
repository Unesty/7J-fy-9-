#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h> // we will output to graph db instead later
#include <errno.h>
#include "someutils.h"

// these values are used as escape sequences for header parser
enum CompilerNodes {
    nodenone,
    nodeenum,
    nodeint32,
    nodeint64,
    nodefloat32,
    nodefloat64,
    nodechar,
    nodestring, //
    nodemov,
    nodejmp,
    nodeadd,
    nodesub,
    nodemul,
    nodediv,
    rawdatastart,
    //add more when needed
};

// this struct is unused, just for reference
struct FullNode {
    int id;
    int inlen;
    int outlen;
    int* ins;
    int* outs;
}

// general sequence is like so:
// read header
// it has 0, header length, version id, graph length, parser code
// then it executes parser code
// to parallelize parsing, node data length
// will be the same. To store connections
// 
// specific sequence:
// parser seek 12 bytes and allocates 6 bytes for node type data and 6 bytes for connection node id data and raw data
//  nodenone
//   0
//  nodeint32
//   header length
// parser seek value from second node
//  // then we need to allocate memory for other graphs with graphs of their escape sequences for node types and compile and run their parsers
// nodeint32
//  version
// nodeint32
//  graph start offset
// nodeint32
//  graph size
// nodeint64
//  raw data start offset
// nodeint64
//  raw data length
// // then we need parser function for graph
// // we can write code for it in graph,
// // or get existing code
// 

static struct DBFileValues {
    FILE* f;
    struct stat *restrict statbuf;
    char* buf;
}dv;

int open_db(char *path) {
    int res = stat(path, dv.statbuf);
    if(res == -1) {
        printf("failed to stat %s\n", path);
    }
    dv.f = fopen(path, "rw");
    if(dv.f == NULL) {
        printf("failed to open file %s\n", path);
    }
    size_t sz = UINT32_MAX;
    res = readall(dv.f, &dv.buf, &sz);
    if(res != READALL_OK) {
        printf("failed to read %s\n", path);
        //TODO
    }
    uint32_t i = 0; // current node in header
    uint32_t hnl = sizeof(uint32_t); // header node data length in bytes
    uint32_t hlen = 32; // header length in nodes
    
    while( i < hlen) {
        uint32_t c = dv.buf[i];
        // before parsing and compiling code
        if(c == nodeint32) {
            //
            i+=2*hnl;
            continue;
        }
        if(c == rawdatastart) {
        }
        i+=hnl;
    }
    return 0;
}
