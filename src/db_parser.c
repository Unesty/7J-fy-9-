#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h> // we will output to graph db instead later
#include <errno.h>
#include <string.h>

#include "someutils.h"
#include "shared.h"
#include <dlg/dlg.h>

int io_init() {
	inpfd = shm_open(inpname, O_RDWR, S_IRUSR | S_IWUSR);
	if (inpfd == -1) {
        dlg_error("can't open shm %s", inpname);
    }
	shm = (struct SharedMem*)mmap(0, sizeof(struct SharedMem), PROT_READ|PROT_WRITE, MAP_SHARED, inpfd, 0); // somehow it works without shmopen
	if (shm == MAP_FAILED) {
        dlg_error("mmap failed");
        return -1;
    }
	shm->pids.db_server = getpid();
	dlg_info("db server pid %d", shm->pids.db_server);
    return 0;
}

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
//  nodeu32
//   header length
// parser seek value from second node
//  // then we need to allocate memory for other graphs with graphs of their escape sequences for node types and compile and run their parsers
// nodeu32
//  header version
// then we need a version graph that contains changes between versions
// how to represent chages? One way is to track binary differences and what parser needs to know. Other is to keep which nodes was added/removed in which versions. Also each graph can have hash, checksum.
// chages are represented as TODO. currently not represented
//
// nodeu32
//  graph start offset
// nodeu32
//  graph size
// nodeu64
//  raw data start offset
// nodeu64
//  raw data length
// // then we need parser function for graph
// // we can write code for it in graph,
// // or get existing code
// 

// int open_db(char *path) {
//     int res = io_init();
//     if(res != 0) {
//         return res;
//     }
//     // these values are used as escape sequences for header parser
//     enum CompilerNodes {
//         nodenone,
//         rawdatastart,
//         nodegraphdatastart,
//         nodeenum,
//         nodeu32,
//         nodeu64,
//         nodef32,
//         nodefloat64,
//         nodechar,
//         nodestring, //
//         nodeptrarr,
//         nodemov,
//         nodejmp,
//         nodeje,
//         nodejne,
//         nodewhile,
//         nodeif,
//         nodeadd,
//         nodesub,
//         nodemul,
//         nodediv,
//         //add more when needed
//     };
//
//     enum HeaderSeq {
//         hs_null,
//         hs_hlen,
//         hs_version,
//     };
//
//     res = stat(path, &dv.statbuf);
//     if(res == -1) {
//         dlg_debug("failed to stat %s\n", path); // we need a graph of errors, so we need to save parsing history in another graph
//         return -1;
//     }
//     db_fds[0] = open(path, O_RDWR);
//     if(db_fds[0] == 0) {
//         dlg_debug("failed to open file %s\n", path);
//         return -2;
//     }
//     size_t sz = dv.statbuf.st_size;
//     if(sz < 32) {
//         dlg_error("file size is less than minimal\n");
//     }
//
// //     dv.buf = malloc(sz);
// //     res = readall(db_files[0], (char**)dv.buf, &sz);
//     buf = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_SHARED, db_fds[0], 0);
//     if(res != READALL_OK) {
//         dlg_debug("failed to read %s\n", path);
//         //TODO
//         return res;
//     }
//     uint32_t i = 0; // current node in header
//     const uint32_t hnl = 4; // header node data length in bytes
//     uint32_t hlen = 32; // header length in nodes, just for understandability of c file. it's same as buf[3]
//     uint32_t c;
// //     while( i < hlen) {
// //         c = dv.buf[i];
// //         // before parsing and compiling code
// //         //if(c == nodeu32) {
// //             //
// //             //i+=2*hnl;
// //             //continue;
// //         //}
// // //         if(c == nodewhile) {
// // //
// // //         }
// //         //
// //         if(c == nodemov) {
// //
// //         }
// //         if(c == rawdatastart) {
// //         }
// //         i+=hnl;
// //     }
//     // hs_null
//     i++;
//     // hs_hlen
//     c = buf[i];
//     if(c == nodeu32) {
//         i++;
//         c = buf[i];
//         hlen = c;
//         dlg_debug("hlen = %d\n", hlen);
//     } else {
//         dlg_error("invalid header: second node must have int32 type\n");
//         return 1;
//     }
//     // currently we map all file
// //    lseek(
//     i++;
//     // hs_version
//     i++;
//     // type graph isn't implemented
//     i++;
//     // version graph isn't implemented
//     i++;
//     // here somewhere should start graph
//     // TODO: loop through entire buf to create more than 1 graph
//     i = hlen;
//     c = buf[i];
//     while(c != nodegraphdatastart) {
//         i++;
//         if(i*hnl > sz) {
//             dlg_error("header exists, but graph data not found\n");
//             return 1404;
//         }
//         c = buf[i];
//     }
//     // Getting graph info
//     shm->gri.id = 0; // currently only 1 graph
//     shm->gri.start = i; // can also be pointer to buf[] element
//
//     // graph length
//     i++;
//     c = buf[i];
//     // TODO: check for buffer overflow
//     if(c == nodeu32) {
//         i++;
//         shm->gri.len = buf[i];
//         grlen = shm->gri.len;
//     } else {
//         dlg_error("invalid graph length node\n");
//         return 1132;
//     }
//     // graph type? currently we have 1. Later different will be used for different types, but parsers can be generated from graph. We still need initial graph parser, so type may still be used later. Graph type will contain node size, data, edge data, how they are stored, etc.
//     // Current graph type is a DataNode array with types defined in ins array, special, for parser, node defines "type" type
//     if(grlen > 0) {
//         idshifts = &buf[i];
//         // create an array of node id shifts. Needed because node data lengths are different.
//         uint32_t n = 0;
// //         uint32_t offs = shm->gri.idshifts_off;
//         while(i*grnl < sz) {
//             c = buf[i];
//             idshifts[/*offs+*/n] = i;
//             shm->gri.idshifts_len++;
//             uint32_t inlen = buf[i];
//             i++;
//             uint32_t outlen = buf[i];
//             i++;
//             i+=inlen+outlen;
//             n++;
//         }
//     }
//     dlg_debug("TODO: what else needed on opening graph db?\n");
//     return 0;
// }
int open_db(char* path) {
    int res = io_init();
    if(res != 0) {
        return res;
    }
    strcpy(shm->db_name, path);
    dlg_info("db path is %s", shm->db_name);
    // these values are used as escape sequences for header parser
    enum CompilerNodes {
        nodenone,
        rawdatastart,
        nodegraphmetadatastart,
        nodegraphdatastart,
        nodeenum,
        nodeu32,
        nodeu64,
        nodef32,
        nodef64,
        nodechar,
        nodestring, //
        nodeptrarr,
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
        nodeidshifstart,
        //add more when needed
    };

//     enum HeaderSeq {
//         hs_null,
//         hs_hlen,
//         hs_version,
//     };



    res = stat(path, &dv.statbuf);
    if(res == -1) {
        dlg_error("failed to stat %s", path); // we need a graph of errors, so we need to save parsing history in another graph
        return -1;
    }
    db_fds[0] = open(path, O_RDWR);
    if(db_fds[0] == 0) {
        dlg_error("failed to open file %s", path);
        return -2;
    }
    db_sz = dv.statbuf.st_size;
    if(db_sz < 32) {
        dlg_error("file size is less than minimal");
    }


    buf = mmap(NULL, db_sz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, db_fds[0], 0);

    if(buf == MAP_FAILED) {
        dlg_error("db mmap failed");
        return -3;
    }

    uint32_t i = 0; // for buffer iteration
    uint8_t hnl = 4;

    // header none node
    i++;
    // header length node
    if(buf[i] == nodeu32) {
        i++;
        dlg_debug("header length = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid header at %d: length node must have u32 type", i);
        return 1;
    }
    i++;
    // header version node
    if(buf[i] == nodeu32) {
        i++;
        dlg_debug("header version = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid header at %d: version node must have u32 type", i);
        return 1;
    }
    i++;

    // some nodes will probably be added here, skip until end
    i = buf[2];
    while(buf[i] != nodegraphmetadatastart) {
        i++;
        if(i*hnl > db_sz) {
            dlg_error("header exists, but graph meta data not found");
            return 1404;
        }
    }
    // Begin parsing graph
    // get graph metadata
    // graph id is known
    shm->gri.id = 0; // next graph in db will have 1. maybe store graph count in header?
    dlg_debug("graph id = %d", 0);
    i++;
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.len = buf[i]; // errors can easily be here
        dlg_debug("graph data length = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: data length node must have u32 type", i);
        return 1;
    }
    i++;
    // graph vertex count
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.vertexcnt = buf[i];
        dlg_debug("graph vertexcnt = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: graph vertexcnt must have u32 type", i);
        return 1;
    }
    i++;
    // graph type
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.type = buf[i];
        dlg_debug("graph type = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: graph type node must have u32 type", i);
        return 1;
    }
    i++;
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.idshifts_len = buf[i];
        dlg_debug("idshifts_len = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: idshifts_len node must have u32 type", i);
        return 1;
    }
    uint32_t nids_l_o = i;
    i++;
    if(buf[i] == nodeidshifstart) {
        i++;
        shm->gri.idshifts_off = i;
        dlg_debug("idshifts_off = %d", i);
        idshifts = &buf[i];
        dlg_debug("idshifts = %lx at %d", (uint64_t)&buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: nodeidshifstart must have nodeidshifstart type", i);
        return 1;
    }
    i+=shm->gri.idshifts_len;
    if(buf[i] == nodegraphdatastart) {
    } else {
        dlg_error("invalid graph metadata at %d: graph data start node must have nodegraphdatastart type, not %d", i, buf[i]);
        return 1;
    }
    i++;
    shm->gri.start = i;
    dlg_debug("graph start in db file = %d", i);

    if(shm->gri.type == 7) {
        dlg_debug("graph type is 7");
        uint32_t new_idshifts_len = shm->gri.len/2+1; // biggest possible value, may be incorrect
        // allocate space if not allocated
        if(shm->gri.idshifts_len == 0) {
            dlg_debug("new_idshifts_len = %d\nmremap space", new_idshifts_len);
//             sleep(10);
            buf = mremap(buf, db_sz, db_sz+new_idshifts_len, MAP_SHARED);
            db_sz += new_idshifts_len;
            dlg_debug("mremapped");
            if(buf == MAP_FAILED) {
                dlg_error("db mremap failed");
                return -3;
            }
            shm->gri.idshifts_len = new_idshifts_len;
            // maybe use memcpy?
            // memcpy(buf[shm->gri.idshifts_off+1],buf[shm->gri.idshifts_off+1+new_idshifts_len]);
            for(uint32_t n = db_sz-1; n >= shm->gri.idshifts_off+new_idshifts_len; n--) {
                buf[n] = buf[n-new_idshifts_len];
                dlg_debug("buf[%d] = %d at buf[%d-%d] ? it's %d", n, buf[n-new_idshifts_len], n, new_idshifts_len, buf[n]);
            }
            buf[nids_l_o] = new_idshifts_len;
            i += new_idshifts_len;

//             uint32_t offs = shm->gri.idshifts_off;
            uint32_t n = 0;
            while(i-shm->gri.idshifts_off-shm->gri.idshifts_len < shm->gri.len) {
                if(i >= db_sz) {
                    dlg_error("shm->gri.len+shm->gri.start is bigger than db_sz %s", shm->gri.len+shm->gri.start > db_sz?"true":"false");
                    break;
                }
                if(n > shm->gri.idshifts_len) {
                    dlg_info("more nodes(%d) than idshifts buffer(%d), need reallocation, or fix algorithm", n, shm->gri.idshifts_len);
                    break; //TODO: error if outside nodes, or reallocate
                }
                if(idshifts[n] == nodegraphdatastart) {
                    dlg_error("nodegraphdatastart at idshifts at %d", n);
                }
                dlg_debug("idshifts[%d] %d", n, idshifts[n]);
                idshifts[/*offs+*/n] = i;
                dlg_debug("idshifts[%d] = %d", n, i);
                shm->gri.vertexcnt++;
                uint32_t inlen = buf[i];
                i++;
                uint32_t outlen = buf[i];
                i++;
                i+=inlen+outlen;
                n++;
            }
        }
    }
    dlg_info("i = %d after setting idshifts", i);

    // parsing graph
    if(shm->gri.len == 0) {
        dlg_info("empty graph");
//         shm->gri.idshifts_len = 0;
        return 0;
    }
    // nodeskip0
    i+=2;
    // nodeskip1
    i+=2;
    // nodeskip2
    i+=2;
    // nodetype
    // may iterate over outs, but no reason now and slow until idshifts are made
    i++;

    dlg_debug("done parsing graph");

    return 0;
}

// int test_db()
int run_db_7(uint32_t grn) {
    dlg_debug("running graph %d", grn);
    sleep(10);
    if(grn >= gris_len) {
        dlg_debug("graph %d not exists", grn);
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
        nodeu8,
        nodeu32,
        nodearray,
    };
    mems = mmap(NULL,             // address
                      fnmemsz,             // size
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,               // fd (not used here)
                      0);               // offset (not used here)
    if(mems == MAP_FAILED) {
        dlg_error("mems mmap failed");
        return -3;
    }
//     uint32_t offs = shm->gri.idshifts_off;
    s = idshifts[/*offs+*/nodetype];
    inlen = buf[s];
    outlen = buf[s+1];
    ins = &buf[s+2];
    outs = &buf[s+2+inlen];
    for(size_t k = 0; k < db_sz; k++) {
        printf("%d ",buf[k]);
    }
    printf("\n");
    while(i<inlen) {
        dlg_debug("TODO: process in = %d on %d node at %d", ins[i], nodetype, i);
        i++;
    }
    i = 0;
    while(i<outlen) {
        // create type on each output node
        // then iterate over all code types
        // to generate executable code
        uint32_t s1 = idshifts[/*offs+*/outs[i]];
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
        dlg_debug("TODO: process out = %d on %d node", outs[i], nodetype);


        i++;
    }
    i = 0;
    dlg_debug("TODO: do the job");
    while(1) {
        sleep(1);
    }
    return 0;
}
