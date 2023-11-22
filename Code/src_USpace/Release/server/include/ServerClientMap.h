#ifndef _SERVER_CLIENT_MAP_H_
#define _SERVER_CLIENT_MAP_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _SERVER_CONNED_CLIENT_NODE
{
    int ClientFd;
    MY_LIST_NODE List;
}
SERVER_CONNED_CLIENT_NODE;

typedef struct _SERVER_CLIENT_MAP
{
    BOOL Inited;
    MY_LIST_NODE ClientList;    // SERVER_CONNED_CLIENT_NODE
    pthread_spinlock_t Lock;
}
SERVER_CLIENT_MAP;

void
ServerClientMapInit(
    SERVER_CLIENT_MAP* Map
    );
    
void
ServerClientMapExit(
    SERVER_CLIENT_MAP* Map
    );

SERVER_CONNED_CLIENT_NODE*
ServerClientMapFetchByFd(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    );

int
ServerClientMapAddNode(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    );

void
ServerClientMapDelByFd(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    );

void
ServerClientMapCollectStat(
    SERVER_CLIENT_MAP* Map,
    __inout char* Buff,
    __in size_t BuffMaxLen,
    __inout int* Offset
    );

#ifdef __cplusplus
}
#endif

#endif /* _SERVER_CLIENT_MAP_H_ */

