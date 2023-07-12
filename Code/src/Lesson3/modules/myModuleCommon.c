#include "myModuleCommon.h"

static const char* sg_ModulesName[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   "Log",
    [MY_MODULES_ENUM_MSG]       =   "Msg",
    [MY_MODULES_ENUM_TPOOL]     =   "TPool",
    [MY_MODULES_ENUM_CMDLINE]   =   "CmdLine",
    [MY_MODULES_ENUM_MHEALTH]   =   "MHealth"
};

const char*
ModuleNameByEnum(
    int Module
    )
{
    return (Module >= 0 && Module < (int)MY_MODULES_ENUM_MAX) ? sg_ModulesName[Module] : NULL;
}

int
MyModuleCommonInit(
    char* LogFile,
    char* RoleName,
    int TPoolSize,
    int TPoolTimeout
    )
{
    int ret = 0;

    if (!LogFile || !RoleName || !TPoolSize || !TPoolTimeout || !strlen(LogFile) || !strlen(RoleName))
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    
#ifdef FEATURE_LOG
    ret = LogModuleInit(LogFile, RoleName, strlen(RoleName));
    if (ret)
    {
        printf("Init log failed!");
        goto CommonReturn;
    }
    LogInfo("---------------- Log Module init success ----------------");
#endif

#ifdef FEATURE_MSG
    MsgModuleInit();
    LogInfo("---------------- Msg Module init success ----------------");
#endif

#ifdef FEATURE_TPOOL
    ret = ThreadPoolModuleInit(TPoolSize, TPoolTimeout);
    if (ret)
    {
        LogErr("Init TPool failed!");
        goto CommonReturn;
    }
    LogInfo("---------------- TPool Module init success ----------------");
#endif

#ifdef FEATURE_MHEALTH
    ret = HealthModuleInit();
    if (ret)
    {
        LogErr("Init HealthModule failed!");
        goto CommonReturn;
    }
    LogInfo("---------------- Health Module init success ----------------");
#endif

CommonReturn:
    return ret;
}

void
MyModuleCommonExit(
    void
    )
{
#ifdef FEATURE_MSG
    MsgModuleExit();
    LogInfo("---------------- Msg Module exit ----------------");
#endif

#ifdef FEATURE_TPOOL
    ThreadPoolModuleExit();
    LogInfo("---------------- TPool Module exit ----------------");
#endif

#ifdef FEATURE_MHEALTH
    HealthModuleExit();
    LogInfo("---------------- Health Module exit ----------------");
#endif

#ifdef FEATURE_LOG
    LogModuleExit();
    LogInfo("---------------- Log Module exit ----------------");
#endif
}
