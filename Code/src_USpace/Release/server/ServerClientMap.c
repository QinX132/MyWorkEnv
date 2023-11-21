#include "include.h"
#include "myModuleCommon.h"
#include "ServerClientMap.h"

void
ServerClientMapInit(
    SERVER_CLIENT_MAP* Map
    )
{
    if (Map && !Map->Inited)
    {
        MY_LIST_NODE_INIT(&Map->ClientList);
        pthread_spin_init(&Map->Lock, PTHREAD_PROCESS_PRIVATE);
        Map->Inited = TRUE;
    }
}
    
void
ServerClientMapExit(
    SERVER_CLIENT_MAP* Map
    )
{
    SERVER_CONNED_CLIENT_NODE *loop = NULL, *tmp = NULL;
    
    if (Map && Map->Inited)
    {
        Map->Inited = FALSE;
        pthread_spin_lock(&Map->Lock);
        MY_LIST_FOR_EACH(&Map->ClientList, loop, tmp, SERVER_CONNED_CLIENT_NODE, List)
        {
            if (loop->ClientFd > 0)
            {
                //close(loop->ClientFd);
                //LogInfo("Close client Fd %d", loop->ClientFd);
                LogInfo("Legacy client Fd: %d", loop->ClientFd);
            }
            MY_LIST_DEL_NODE(&loop->List);
            MyFree(loop);
        }
        pthread_spin_unlock(&Map->Lock);
        pthread_spin_destroy(&Map->Lock);
    }
}

SERVER_CONNED_CLIENT_NODE*
ServerClientMapFetchByFd(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    )
{
    SERVER_CONNED_CLIENT_NODE *loop = NULL, *tmp = NULL, *ret = NULL;
    
    if (Map && Map->Inited && ClientFd >= 0)
    {
        pthread_spin_lock(&Map->Lock);
        MY_LIST_FOR_EACH(&Map->ClientList, loop, tmp, SERVER_CONNED_CLIENT_NODE, List)
        {
            if (ClientFd == loop->ClientFd)
            {
                ret = loop;
                break;
            }
        }
        pthread_spin_unlock(&Map->Lock);
    }

    return ret;
}

int
ServerClientMapAddNode(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    )
{
    int ret = MY_SUCCESS;
    SERVER_CONNED_CLIENT_NODE *node = NULL;
    
    if (!(Map && Map->Inited && ClientFd >= 0))
    {
        ret = -MY_ENOSYS;
        goto CommonReturn;
    }
    node = (SERVER_CONNED_CLIENT_NODE *)MyCalloc(sizeof(SERVER_CONNED_CLIENT_NODE));
    if (!node)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
    
    MY_LIST_NODE_INIT(&node->List);
    pthread_spin_lock(&Map->Lock);
    node->ClientFd = ClientFd;
    MY_LIST_ADD_TAIL(&node->List, &Map->ClientList);
    pthread_spin_unlock(&Map->Lock);
    
CommonReturn:
    return ret;
}

void
ServerClientMapDelByFd(
    SERVER_CLIENT_MAP* Map,
    int ClientFd
    )
{
    BOOL matched = FALSE;
    SERVER_CONNED_CLIENT_NODE *loop = NULL, *tmp = NULL;
    
    if (Map && Map->Inited && ClientFd >= 0)
    {
        pthread_spin_lock(&Map->Lock);
        MY_LIST_FOR_EACH(&Map->ClientList, loop, tmp, SERVER_CONNED_CLIENT_NODE, List)
        {
            if (ClientFd == loop->ClientFd)
            {
                MY_LIST_DEL_NODE(&loop->List);
                MyFree(loop);
                matched = TRUE;
                break;
            }
        }
        pthread_spin_unlock(&Map->Lock);
    }

    if (!matched)
        LogInfo("Mismatched client Fd %d in map.", ClientFd);
}

void
ServerClientMapCollectStat(
    SERVER_CLIENT_MAP* Map,
    __inout char* Buff,
    __in size_t BuffMaxLen,
    __inout int* Offset
    )
{
    SERVER_CONNED_CLIENT_NODE *loop = NULL, *tmp = NULL;
    
    if (Map && Map->Inited)
    {
        pthread_spin_lock(&Map->Lock);
        MY_LIST_FOR_EACH(&Map->ClientList, loop, tmp, SERVER_CONNED_CLIENT_NODE, List)
        {
            *Offset += snprintf(Buff + *Offset, BuffMaxLen - *Offset,
                        "<Fd=%d>", loop->ClientFd);
        }
        pthread_spin_unlock(&Map->Lock);
    }
}

