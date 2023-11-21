#include "myMem.h"
#include "myLogIO.h"
#include "myList.h"
#include "myModuleHealth.h"

#define MY_MEM_MODULE_MAX_NUM                                   16

typedef struct _MY_MEM_NODE
{
    BOOL Registered;
    uint8_t MemModuleId;
    char MemModuleName[MY_BUFF_32];
    uint64_t MemBytesAlloced;
    uint64_t MemBytesFreed;
    pthread_spinlock_t MemSpinlock;
}
MY_MEM_NODE;

typedef struct _MY_MEM_PREFIX
{
    uint8_t     MemModuleId;
    uint32_t    Size    : 31;
    uint32_t    Freed   : 1;
}//__attribute__((packed))  // This will lead to serious problems related to locks
MY_MEM_PREFIX;

typedef struct _MY_MEM_WORKER
{
    MY_MEM_NODE Nodes[MY_MEM_MODULE_MAX_NUM];
    int MainModId;
    BOOL Inited;
    pthread_spinlock_t Lock;
}
MY_MEM_WORKER;

static MY_MEM_WORKER sg_MemWorker = {
        .MainModId = MY_MEM_MODULE_INVALID_ID,
        .Inited = FALSE
    };

int
MemRegister(
    int *MemId,
    char *Name
    )
{
    int ret = MY_SUCCESS;
    int loop = 0;
    BOOL registered = FALSE;
    if (!Name || !MemId || !sg_MemWorker.Inited)
    {
        ret = -MY_EINVAL;
        goto CommonReturn;
    }
    if (*MemId >= 0 && *MemId < MY_MEM_MODULE_MAX_NUM && 
        sg_MemWorker.Nodes[*MemId].MemModuleId == *MemId && sg_MemWorker.Nodes[*MemId].Registered)
    {
        LogInfo("Mem has Registered");
        goto CommonReturn;
    }
    pthread_spin_lock(&sg_MemWorker.Lock);
    {
        for(loop = 0; loop < MY_MEM_MODULE_MAX_NUM; loop ++)
        {
            if (sg_MemWorker.Nodes[loop].Registered)
            {
                continue;
            }
            memset(&sg_MemWorker.Nodes[loop], 0, sizeof(MY_MEM_NODE));
            strcpy(sg_MemWorker.Nodes[loop].MemModuleName, Name);
            pthread_spin_init(&sg_MemWorker.Nodes[loop].MemSpinlock, PTHREAD_PROCESS_PRIVATE);
            sg_MemWorker.Nodes[loop].Registered = TRUE;
            sg_MemWorker.Nodes[loop].MemModuleId = loop;
            *MemId = loop;
            registered = TRUE;
            break;
        }
    }
    pthread_spin_unlock(&sg_MemWorker.Lock);
    if (!registered)
    {
        ret = -MY_ENOMEM;
        goto CommonReturn;
    }
CommonReturn:
    return ret;
}

int
MemUnRegister(
    int* MemId
    )
{
    int ret = MY_SUCCESS;
    if (sg_MemWorker.Inited && MemId && *MemId >= 0 && *MemId < MY_MEM_MODULE_MAX_NUM && 
        sg_MemWorker.Nodes[*MemId].MemModuleId == *MemId && sg_MemWorker.Nodes[*MemId].Registered)
    {
        pthread_spin_lock(&sg_MemWorker.Lock);
        if (!MemLeakSafetyCheckWithId(*MemId))
        {
            ret = -MY_ERR_MEM_LEAK;
        }
        memset(&sg_MemWorker.Nodes[*MemId], 0, sizeof(MY_MEM_NODE));
        *MemId = MY_MEM_MODULE_INVALID_ID;
        pthread_spin_unlock(&sg_MemWorker.Lock);
    }

    return ret;
}

int
MemModuleInit(
    void
    )
{
    if (sg_MemWorker.Inited)
        return MY_SUCCESS;
    
    sg_MemWorker.Inited = TRUE;
    pthread_spin_init(&sg_MemWorker.Lock, PTHREAD_PROCESS_PRIVATE);
    memset(sg_MemWorker.Nodes, 0, sizeof(sg_MemWorker.Nodes));
    return MemRegister(&sg_MemWorker.MainModId, "MemModuleCommon");
}

int
MemModuleExit(
    void
    )
{
    int ret = MY_SUCCESS;
    if (!sg_MemWorker.Inited)
        goto CommonReturn;
    
    sg_MemWorker.Inited = FALSE;
    ret = MemUnRegister(&sg_MemWorker.MainModId);
    pthread_spin_destroy(&sg_MemWorker.Lock);

CommonReturn:
    return ret;
}

int
MemModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = MY_SUCCESS;
    int len = 0;
    int loop = 0;

    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, "<%s:", ModuleNameByEnum(MY_MODULES_ENUM_MEM));
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = -MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }

    for(loop = 0; loop < MY_MEM_MODULE_MAX_NUM; loop ++)
    {
        if (!sg_MemWorker.Nodes[loop].Registered)
        {
            continue;
        }
        len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, 
                "[MemId=%u, MemName<%s>, MemAlloced=%"PRIu64", MemFreed=%"PRIu64", MemInusing=%"PRIu64"]",
                sg_MemWorker.Nodes[loop].MemModuleId, sg_MemWorker.Nodes[loop].MemModuleName, 
                sg_MemWorker.Nodes[loop].MemBytesAlloced, sg_MemWorker.Nodes[loop].MemBytesFreed,
                sg_MemWorker.Nodes[loop].MemBytesAlloced - sg_MemWorker.Nodes[loop].MemBytesFreed);
        if (len < 0 || len >= BuffMaxLen - *Offset)
        {
            ret = -MY_ENOMEM;
            LogErr("Too long Msg!");
            goto CommonReturn;
        }
        else
        {
            *Offset += len;
        }
    }
    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, ">");
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = -MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }
CommonReturn:
    return ret;
}

void*
MemCalloc(
    int MemId,
    size_t Size
    )
{
    void *ret = NULL;
    if (MemId < 0 || MemId > MY_MEM_MODULE_MAX_NUM - 1 || !sg_MemWorker.Nodes[MemId].Registered || Size > 0x7fffffff)
    {
        return NULL;
    }
    ret = calloc(Size + sizeof(MY_MEM_PREFIX), 1);
    if (ret)
    {
        ((MY_MEM_PREFIX*)ret)->MemModuleId = MemId;
        ((MY_MEM_PREFIX*)ret)->Size = Size;
        ((MY_MEM_PREFIX*)ret)->Freed = FALSE;
        pthread_spin_lock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
        sg_MemWorker.Nodes[MemId].MemBytesAlloced += Size;
        pthread_spin_unlock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
    }
    ret = (void*)(((uint8_t*)ret) + sizeof(MY_MEM_PREFIX));
    return ret;
}

void
MemFree(
    int MemId,
    void* Ptr
    )
{
    uint32_t size = 0;
    if (Ptr && MemId >= 0 && MemId < MY_MEM_MODULE_MAX_NUM && sg_MemWorker.Nodes[MemId].Registered)
    {
        Ptr = (void*)(((uint8_t*)Ptr) - sizeof(MY_MEM_PREFIX));
        size = ((MY_MEM_PREFIX*)Ptr)->Size;
        ((MY_MEM_PREFIX*)Ptr)->Freed = TRUE;
        free(Ptr);
        pthread_spin_lock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
        sg_MemWorker.Nodes[MemId].MemBytesFreed += size;
        pthread_spin_unlock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
    }
}

inline void*
MyCalloc(
    size_t Size
    )
{
    void* Ptr = MemCalloc(sg_MemWorker.MainModId, Size);
#ifdef DEBUG
    LogInfo("Size %zu Ptr %p", Size, Ptr);
#endif
    return Ptr;
}

inline void
MyFree(
    void* Ptr
    )
{
#ifdef DEBUG
    LogInfo("free %p", Ptr);
#endif
    return MemFree(sg_MemWorker.MainModId, Ptr);
}

BOOL
MemLeakSafetyCheckWithId(
    int MemId
    )
{
    BOOL ret = TRUE;
    if (!sg_MemWorker.Inited || 
        !(MemId >= 0 && MemId < MY_MEM_MODULE_MAX_NUM && sg_MemWorker.Nodes[MemId].Registered))
    {
        goto CommonReturn;
    }
    pthread_spin_lock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
    {
        LogInfo("%s:alloced:%"PRIu64" freed:%"PRIu64"", sg_MemWorker.Nodes[MemId].MemModuleName, 
            sg_MemWorker.Nodes[MemId].MemBytesAlloced, sg_MemWorker.Nodes[MemId].MemBytesFreed);
        ret = sg_MemWorker.Nodes[MemId].MemBytesFreed == sg_MemWorker.Nodes[MemId].MemBytesAlloced;
    }
    pthread_spin_unlock(&sg_MemWorker.Nodes[MemId].MemSpinlock);
    
CommonReturn:
    return ret;
}

BOOL
MemLeakSafetyCheck(
    void
    )
{
    return MemLeakSafetyCheckWithId(sg_MemWorker.MainModId);
}

