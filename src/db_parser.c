#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
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
    
    
    size_t buf_add = 10000; // reserved size
    buf = mmap(NULL, db_sz+buf_add, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED, db_fds[0], 0);
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
    uint32_t gmds = 0; //TODO: array of these for multiple graphs
    while(buf[i] != nodegraphmetadatastart) {
        i++;
        if(i*hnl > db_sz) {
            dlg_error("header exists, but graph meta data not found");
            return 1404;
        }
    }
    gmds = i;
    // Begin parsing graph
    // get graph metadata
    // graph id is known
    //TODO: use u32 pointer instead of copying to another shm
    shm->gri.id = 0; // next graph in db will have 1. maybe store graph count in header?
    dlg_debug("graph id = %d", 0);
    i++;
    if(buf[i] == nodeu32) {
        i++;
//         shm->gri.len = buf[i]; // errors can easily be here
        shm->gri.plen = i; // errors can easily be here
        dlg_debug("graph data length = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: data length node must have u32 type", i);
        return 1;
    }
    i++;
    // graph vertex count
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.pvertexcnt = i;
        dlg_debug("graph vertexcnt = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: graph vertexcnt must have u32 type", i);
        return 1;
    }
    i++;
    // graph type
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.ptype = i;
        dlg_debug("graph type = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: graph type node must have u32 type", i);
        return 1;
    }
    i++;
    if(buf[i] == nodeu32) {
        i++;
        shm->gri.pidshifts_len = i;
        dlg_debug("idshifts_len = %d at %d", buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: idshifts_len node must have u32 type", i);
        return 1;
    }
    uint32_t nids_l_o = i;
    i++;
    if(buf[i] == nodeidshifstart) {
        i++;
        shm->gri.pidshifts_off = i;
        dlg_debug("idshifts_off = %d", i);
        idshifts = &buf[i];
        dlg_debug("idshifts = %lx at %d", (uint64_t)&buf[i], i);
    } else {
        dlg_error("invalid graph metadata at %d: nodeidshifstart must have nodeidshifstart type", i);
        return 1;
    }
    i+=buf[shm->gri.pidshifts_len];
    if(buf[i] == nodegraphdatastart) {
    } else {
        dlg_error("invalid graph metadata at %d: graph data start node must have nodegraphdatastart type, not %d", i, buf[i]);
        return 1;
    }
    i++;
    shm->gri.start = i;
    dlg_debug("graph start in db file = %d", i);

    if(buf[shm->gri.ptype] == 7) {
        dlg_debug("graph type is 7");
//         raise(SIGSTOP); // wait for debugger
        //TODO: in case of error in buf[shm->gri.pidshifts_len], recalculate it
        uint32_t new_idshifts_len = buf[shm->gri.plen]/2+1; // biggest possible value, may be incorrect
        uint32_t add_len = new_idshifts_len*4-buf[shm->gri.pidshifts_len]*4;
        // allocate space if not allocated
        {
//         if(buf[shm->gri.pidshifts_len] == 0) {
            size_t new_db_sz = db_sz+add_len;

            // mremap works only for 1 thread, doesn't resize file when other process mmaped file, like in this case
            // so we send signal to other threads to mremap
            if(new_db_sz == (size_t)dv.statbuf.st_size) {
                db_sz = new_db_sz;
                if(shm->pids.graphics != 0) {
                    for(int kk=10; kk>0; --kk) {
                        if(kill(shm->pids.graphics, 0)) {
                            dlg_debug("wait for graphics process");
                            sleep(1);
                        } else {
                            break;
                        }
                    }
                    dlg_debug("SIGUSR1 to graphics process");
                    if(kill(shm->pids.graphics, SIGUSR1)) {
                        dlg_error("couldn't send SIGUSR1 signal to graphics process");
                    } else {
                        dlg_debug("sent SIGUSR1 to graphics process");
                    }
                }
//                 if(kill(shm->pids.sound, SIGBUS)) {
//                     dlg_debug("sent SIGBUS to graphics process");
//                 } else {
//                     dlg_error("couldn't send SIGBUS signal to graphics process");
//                 }
//                 if(kill(shm->pids.ui_logic, SIGBUS)) {
//                     dlg_debug("sent SIGBUS to graphics process");
//                 } else {
//                     dlg_error("couldn't send SIGBUS signal to graphics process");
//                 }
            } else {
                dlg_error("file stat says it's not resized to %ld, it was %d, now %ld", new_db_sz, db_sz, dv.statbuf.st_size);
            }
            dlg_debug("new_idshifts_len = %d\nappend, mremap space", new_idshifts_len);
            lseek(db_fds[0], 0, SEEK_END);
            uint8_t *b = malloc(add_len);
            memset(b, 0, add_len);
            write(db_fds[0], b, add_len);
            free(b);
            buf = mremap(buf, db_sz, new_db_sz, MAP_SHARED);
            dlg_debug("mremapped");

            if(buf == MAP_FAILED) {
                dlg_error("db mremap failed");
                return -3;
            }

            res = stat(path, &dv.statbuf);
            if(res == -1) {
                dlg_error("failed to stat %s", path); // we need a graph of errors, so we need to save parsing history in another graph
                return -1;
            }


            buf[shm->gri.pidshifts_len] = new_idshifts_len;
            // maybe use memmove?
//             memmove(&buf[buf[shm->gri.pidshifts_off]+1], buf[buf[shm->gri.pidshifts_off]+1+new_idshifts_len], new_idshifts_len*4);
            for(uint32_t n = db_sz-1; n >= buf[shm->gri.pidshifts_off]+new_idshifts_len; n--) {
                buf[n] = buf[n-new_idshifts_len];
                dlg_debug("buf[%d] = %d at buf[%d-%d] ? it's %d", n, buf[n-new_idshifts_len], n, new_idshifts_len, buf[n]);
            }
            buf[nids_l_o] = new_idshifts_len;
            i += new_idshifts_len;

//             uint32_t offs = buf[shm->gri.pidshifts_off];
            uint32_t n = 0;
            while(i-buf[shm->gri.pidshifts_off]-buf[shm->gri.pidshifts_len] < buf[shm->gri.plen]) {
                if(i >= db_sz) {
                    dlg_error("buf[shm->gri.plen]+shm->gri.start is bigger than db_sz %s", buf[shm->gri.plen]+shm->gri.start > db_sz?"true":"false");
                    break;
                }
                if(n > buf[shm->gri.pidshifts_len]) {
                    dlg_info("more nodes(%d) than idshifts buffer(%d), need reallocation, or fix algorithm", n, buf[shm->gri.pidshifts_len]);
                    break; //TODO: error if outside nodes, or reallocate
                }
                if(idshifts[n] == nodegraphdatastart) {
                    dlg_error("nodegraphdatastart at idshifts at %d", n);
                }
                dlg_debug("idshifts[%d] %d", n, idshifts[n]);
                idshifts[/*offs+*/n] = i;
                dlg_debug("idshifts[%d] = %d", n, i);
                buf[shm->gri.pvertexcnt]++;
                uint32_t inlen = buf[i];
                i++;
                uint32_t outlen = buf[i];
                i++;
                i+=inlen+outlen;
                n++;
            }
//         } else {
//             // add all idshifts from all graphs
//             buf[shm->gri.pvertexcnt] = buf[shm->gri.pidshifts_len];
        }
    }
    dlg_info("i = %d after setting idshifts", i);

    // parsing graph
    if(buf[shm->gri.plen] == 0) {
        dlg_info("empty graph");
//         buf[shm->gri.pidshifts_len] = 0;
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
        nodetype, // To mark, that node has a type. Also it must have additional type data including edge count, TODO. Data can be used by operations by iterating ins and processing outs from 0.
        nodebit, // This type has 1 bit of information stored in edge section as 0 or 1. Other types also store information in out edges.
        nodeopcode, // This type that node has opcode type. Opcode contain code number, arguments with specific types. For example mov_u8 is 0, nodeu8, noderegisteru8.
        nodeu8,
        nodeu32,
        nodearray,
        nodef32,
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
//     uint32_t offs = buf[shm->gri.pidshifts_off];
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
