#include "myMem.h"
#include "myLogIO.h"
#include "myList.h"

#define MY_MEM_MODULE_MAX_NUM                                   16

typedef struct{
    BOOL Registered;
    uint8_t MemModuleId;
    char MemModuleName[MY_TEST_BUFF_32];
    uint64_t MemBytesAlloced;
    uint64_t MemBytesFreed;
    pthread_spinlock_t MemSpinlock;
}
MY_MEM_NODE;

typedef struct{
    uint8_t     MemModuleId;
    uint32_t    Size    : 31;
    uint32_t    Freed   : 1;
}__attribute__((packed))
MY_MEM_PREFIX;

static MY_MEM_NODE sg_MemNodes[MY_MEM_MODULE_MAX_NUM];
static int sg_MemModId = -1;

int
MemRegister(
    int *MemId,
    char *Name
    )
{
    int ret = 0;
    int loop = 0;
    BOOL registered = FALSE;
    if (!Name)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    if (*MemId >= 0 && *MemId < MY_MEM_MODULE_MAX_NUM && sg_MemNodes[*MemId].MemModuleId == *MemId && sg_MemNodes[*MemId].Registered)
    {
        LogInfo("Mem has Registered");
        goto CommonReturn;
    }
    for(loop = 0; loop < MY_MEM_MODULE_MAX_NUM; loop ++)
    {
        if (sg_MemNodes[loop].Registered)
        {
            continue;
        }
        memset(&sg_MemNodes[loop], 0, sizeof(MY_MEM_NODE));
        strcpy(sg_MemNodes[loop].MemModuleName, Name);
        pthread_spin_init(&sg_MemNodes[loop].MemSpinlock, PTHREAD_PROCESS_PRIVATE);
        sg_MemNodes[loop].Registered = TRUE;
        sg_MemNodes[loop].MemModuleId = loop;
        *MemId = loop;
        registered = TRUE;
        break;
    }
    if (!registered)
    {
        ret = MY_ENOMEM;
        goto CommonReturn;
    }
CommonReturn:
    return ret;
}

void
MemUnRegister(
    int MemId
    )
{
    if (MemId >= 0 && MemId < MY_MEM_MODULE_MAX_NUM && sg_MemNodes[MemId].MemModuleId == MemId && sg_MemNodes[MemId].Registered)
    {
        memset(&sg_MemNodes[MemId], 0, sizeof(MY_MEM_NODE));
    }
}

int
MemModuleInit(
    void
    )
{
    memset(sg_MemNodes, 0, sizeof(sg_MemNodes));
    return MemRegister(&sg_MemModId, "MemModuleCommon");
}

void
MemModuleExit(
    void
    )
{
    MemLeakSafetyCheck();
    return MemUnRegister(sg_MemModId);
}

int
MemModuleCollectStat(
    char* Buff,
    int BuffMaxLen,
    int* Offset
    )
{
    int ret = 0;
    int len = 0;
    int loop = 0;

    len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, "<%s:", ModuleNameByEnum(MY_MODULES_ENUM_MEM));
    if (len < 0 || len >= BuffMaxLen - *Offset)
    {
        ret = MY_ENOMEM;
        LogErr("Too long Msg!");
        goto CommonReturn;
    }
    else
    {
        *Offset += len;
    }

    for(loop = 0; loop < MY_MEM_MODULE_MAX_NUM; loop ++)
    {
        if (!sg_MemNodes[loop].Registered)
        {
            continue;
        }
        len = snprintf(Buff + *Offset, BuffMaxLen - *Offset, "[MemId=%u, MemName=%s, MemAlloced=%llu, MemFreed=%llu, MemInusing=%lld]",
                sg_MemNodes[loop].MemModuleId, sg_MemNodes[loop].MemModuleName, 
                sg_MemNodes[loop].MemBytesAlloced, sg_MemNodes[loop].MemBytesFreed,
                sg_MemNodes[loop].MemBytesAlloced - sg_MemNodes[loop].MemBytesFreed);
        if (len < 0 || len >= BuffMaxLen - *Offset)
        {
            ret = MY_ENOMEM;
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
        ret = MY_ENOMEM;
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
    if (!sg_MemNodes[MemId].Registered || Size > 0x7fffffff)
    {
        return NULL;
    }
    ret = calloc(Size + sizeof(MY_MEM_PREFIX), 1);
    if (ret)
    {
        ((MY_MEM_PREFIX*)ret)->MemModuleId = MemId;
        ((MY_MEM_PREFIX*)ret)->Size = Size;
        ((MY_MEM_PREFIX*)ret)->Freed = FALSE;
        pthread_spin_lock(&sg_MemNodes[MemId].MemSpinlock);
        sg_MemNodes[MemId].MemBytesAlloced += Size;
        pthread_spin_unlock(&sg_MemNodes[MemId].MemSpinlock);
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
    if (sg_MemNodes[MemId].Registered)
    {
        Ptr = (void*)(((uint8_t*)Ptr) - sizeof(MY_MEM_PREFIX));
        size = ((MY_MEM_PREFIX*)Ptr)->Size;
        ((MY_MEM_PREFIX*)Ptr)->Freed = TRUE;
        free(Ptr);
        pthread_spin_lock(&sg_MemNodes[MemId].MemSpinlock);
        sg_MemNodes[MemId].MemBytesFreed += size;
        pthread_spin_unlock(&sg_MemNodes[MemId].MemSpinlock);
        Ptr = NULL;
    }
}

inline void*
MyCalloc(
    size_t Size
    )
{
    void* Ptr = MemCalloc(sg_MemModId, Size);
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
    return MemFree(sg_MemModId, Ptr);
}

BOOL
MemLeakSafetyCheck(
    void
    )
{
    if (sg_MemNodes[sg_MemModId].Registered)
    {
        LogInfo("alloced:%lld freed:%lld", sg_MemNodes[sg_MemModId].MemBytesAlloced, 
            sg_MemNodes[sg_MemModId].MemBytesFreed);
    }
    return sg_MemNodes[sg_MemModId].MemBytesFreed == sg_MemNodes[sg_MemModId].MemBytesAlloced;
}

