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
    rawdatastart,
    nodegraphdatastart,
    nodeenum,
    nodeint32,
    nodeint64,
    nodefloat32,
    nodefloat64,
    nodechar,
    nodestring, //
    nodemov,
    nodejmp,
    nodeje,
    nodejne,
    nodewhile,
    nodeif,
    nodeadd,
    nodesub,
    nodemul,
    nodediv,
    //add more when needed
};

enum HeaderSeq {
    hs_null,
    hs_hlen,
    hs_version,
};

// this struct is unused, just for reference
struct FullNode {
    int id;
    int inlen;
    int outlen;
    int* ins;
    int* outs;
};

uint32_t fnmemsz = 4096;
char **mems = NULL;

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
//  header version
// then we need a version graph that contains changes between versions
// how to represent chages? One way is to track binary differences and what parser needs to know. Other is to keep which nodes was added/removed in which versions. Also each graph has hash.
// chages are represented as TODO. currently not represented
//
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
    struct stat statbuf;
    char** buf;
}dv;

int open_db(char *path) {
    int res = stat(path, &dv.statbuf);
    if(res == -1) {
        printf("failed to stat %s\n", path);
        return -1;
    }
    dv.f = fopen(path, "rw");
    if(dv.f == NULL) {
        printf("failed to open file %s\n", path);
        return -2;
    }
    size_t sz = dv.statbuf.st_size;
    if(sz < 32) {
        perror("file size is less than minimal\n");
    }
    dv.buf = malloc(sz);
    res = readall(dv.f, (char**)dv.buf, &sz);
    if(res != READALL_OK) {
        printf("failed to read %s\n", path);
        //TODO
        return res;
    }
    uint32_t i = 0; // current node in header
    uint32_t hnl = sizeof(uint32_t); // header node data length in bytes
    uint32_t hlen = 32; // header length in nodes
    uint32_t c;
//     while( i < hlen) {
//         c = dv.buf[i];
//         // before parsing and compiling code
//         //if(c == nodeint32) {
//             //
//             //i+=2*hnl;
//             //continue;
//         //}
// //         if(c == nodewhile) {
// //
// //         }
//         //
//         if(c == nodemov) {
//
//         }
//         if(c == rawdatastart) {
//         }
//         i+=hnl;
//     }
    // hs_null
    i++;
    // hs_hlen
    uint32_t *buf = *(uint32_t**)dv.buf;
    c = buf[i];
    if(c == nodeint32) {
        i++;
        c = buf[i];
        hlen = c;
        printf("hlen = %d\n", hlen);
    } else {
        perror("invalid header: second node must have int32 type\n");
        return 1;
    }
    // currently we read all file
//    lseek(
    i++;
    // hs_version
    i++;
    // type graph isn't implemented
    i++;
    // version graph isn't implemented
    i++;
    // here somewhere should start graph data
    i = hlen;
    c = buf[i];
    while(c != nodegraphdatastart) {
        i++;
        if(i*hnl > sz) {
            perror("header exists, but graph data not found\n");
            return 1404;
        }
        c = buf[i];
    }
    printf("TODO\n");
    return 0;
}
