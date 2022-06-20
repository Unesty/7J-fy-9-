#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h> // we will output to graph db instead later
#include <errno.h>
#include "someutils.h"


// this struct is unused, just for reference
// struct DataPtrNode {
//     int id;
//     int inlen;
//     int outlen;
//     int* ins;
//     int* outs;
// };
// this struct is unused, just for reference
// struct DataNode {
//     int id;
//     int inlen;
//     int outlen;
//     int ins[inlen];
//     int outs[outlen];
// };

size_t fnmemsz = 4096;
char **mems = NULL;
struct GraphInfo {
    uint32_t id;
    uint32_t start;
    uint32_t len;
    // uint32_t type;
    uint32_t *idshifts;
    size_t idshifts_len;
}*gris;
uint8_t gris_len = 0;

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
// how to represent chages? One way is to track binary differences and what parser needs to know. Other is to keep which nodes was added/removed in which versions. Also each graph can have hash, checksum.
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
uint32_t *buf;

int open_db(char *path) {
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

    int res = stat(path, &dv.statbuf);
    if(res == -1) {
        printf("failed to stat %s\n", path); // we need a graph of errors, so we need to save parsing history in another graph
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
    const uint32_t hnl = 4; // header node data length in bytes
    uint32_t hlen = 32; // header length in nodes, just for understandability of c file. it's same as buf[3]
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
    buf = *(uint32_t**)dv.buf;
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
    // here somewhere should start graph
    // TODO: loop through entire buf to create more than 1 graph
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
    gris = malloc(sizeof(struct GraphInfo));
    gris_len++;
    uint32_t gri = 0;
    uint32_t grlen = 0;
    uint32_t grnl = 4; // graph data array element length
    gris[gri].id=gri;
    gris[gri].start = i; // can also be pointer to buf[] element

    // graph length
    i++;
    c = buf[i];
    // TODO: check for buffer overflow
    if(c == nodeint32) {
        i++;
        gris[gri].len = buf[i];
        grlen = gris[gri].len;
    } else {
        perror("invalid graph length node\n");
        return 1132;
    }
    // graph type? currently we have 1. Later different will be used for different types, but parsers can be generated from graph. We still need initial graph parser, so type may still be used later. Graph type will contain node size, data, edge data, how they are stored, etc.
    // Current graph type is a struct DataNode array with types defined in ins array, special, for parser, node defines "type" type
    if(grlen > 0) {
        gris[gri].idshifts = malloc(32*grlen/2);// allocate biggest possibly needed memory
        // create an array of node id shifts. Needed because node data lengths are different.
        uint32_t n = 0;
        while(i*grnl < sz) {
            c = buf[i];
            gris[gri].idshifts[n] = i;
            gris[gri].idshifts_len++;
            uint32_t inlen = buf[i];
            i++;
            uint32_t outlen = buf[i];
            i++;
            i+=inlen+outlen;
            n++;
        }
    }
    printf("TODO: what else needed on opening graph db?\n");
    return 0;
}

// int test_db()
int run_db_7(uint32_t gri) {
    if(&gris[gri] ==  NULL || gri >= gris_len) {
        printf("graph by id %d not exists", gri);
        return 404;
    }
    // call procedures on specific nodes
    uint32_t i = 0, s = 0, inlen = 0, outlen = 0;
    uint32_t *ins, *outs;
    enum SpecialNodes {
        nodeskip0,
        nodeskip1,
        nodeskip2,
        nodetype,
        nodebit,
        nodeopcode,
    };
    mems = mmap(NULL,             // address
                      fnmemsz,             // size
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,               // fd (not used here)
                      0);               // offset (not used here)
    s = gris[gri].idshifts[nodetype];
    inlen = buf[s];
    outlen = buf[s+1];
    ins = &buf[s+2];
    outs = &buf[s+2+inlen];
    while(i<inlen) {
        printf("TODO: process in = %d on 7 node\n", ins[i]);
        i++;
    }
    i = 0;
    while(i<outlen) {
        // create type on each output node
        // then iterate over all code types
        // to generate executable code
        uint32_t s1 = gris[gri].idshifts[outs[i]];
        uint32_t inlen1 = buf[s1];
        uint32_t outlen1 = buf[s1+1];
        uint32_t i1 = 0;
        // here type properties are defined
        // each type has a specific procedure
        //
        while(i1<inlen1) {
            i1++;
        }
        i1 = 0;
        while(i1<outlen1) {
            i1++;
        }

        if (!mems[gri]) {
            perror("failed to allocate memory");
            return 1;
        }
        i++;
    }
    i = 0;
    return 0;
}
