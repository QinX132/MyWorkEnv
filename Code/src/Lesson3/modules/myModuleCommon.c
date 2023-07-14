#include "myModuleCommon.h"
#include "myCommonUtil.h"
#include "myLogIO.h"
#include "myMsg.h"
#include "myThreadPool.h"
#include "myCmdLine.h"
#include "myModuleHealth.h"

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
    MY_MODULES_INIT_PARAM ModuleInitParam 
    )
{
    int ret = 0;

    if (!ModuleInitParam.LogFile || !ModuleInitParam.RoleName || !ModuleInitParam.TPoolSize || 
        !ModuleInitParam.TPoolTimeout || !strlen(ModuleInitParam.LogFile) || !strlen(ModuleInitParam.RoleName) ||
        !ModuleInitParam.Argv)
    {
        ret = MY_EINVAL;
        goto CommonReturn;
    }
    
#ifdef FEATURE_CMDLINE
    ret = CmdLineModuleInit(ModuleInitParam.RoleName, ModuleInitParam.Argc, ModuleInitParam.Argv);
    if (ret)
    {
        if (MY_ERR_EXIT_WITH_SUCCESS != ret)
        {
            printf("Cmd line init failed!");
        }
        goto CommonReturn;
    }
#endif
#ifdef FEATURE_LOG
    ret = LogModuleInit(ModuleInitParam.LogFile, ModuleInitParam.RoleName, strlen(ModuleInitParam.RoleName));
    if (ret)
    {
        printf("Init log failed!");
        goto CommonReturn;
    }
    LogInfo("---------------------------------------------------------");
    LogInfo("---------------- Log Module init success ----------------");
#endif


#ifdef FEATURE_MSG
    MsgModuleInit();
    LogInfo("---------------- Msg Module init success ----------------");
#endif

#ifdef FEATURE_TPOOL
    ret = ThreadPoolModuleInit(ModuleInitParam.TPoolSize, ModuleInitParam.TPoolTimeout);
    if (ret)
    {
        LogErr("Init TPool failed!");
        goto CommonReturn;
    }
    LogInfo("---------------- TPool Module init success ---------------");
#endif

#ifdef FEATURE_MHEALTH
    ret = HealthModuleInit();
    if (ret)
    {
        LogErr("Init HealthModule failed!");
        goto CommonReturn;
    }
    LogInfo("---------------- Health Module init success --------------");
#endif

CommonReturn:
    return ret;
}

void
MyModuleCommonExit(
    void
    )
{
#ifdef FEATURE_CMDLINE
    CmdLineModuleExit();
#endif

#ifdef FEATURE_MSG
    MsgModuleExit();
    LogInfo("-------------------- Msg Module exit ---------------------");
#endif

#ifdef FEATURE_TPOOL
    ThreadPoolModuleExit();
    LogInfo("-------------------- TPool Module exit -------------------");
#endif

#ifdef FEATURE_MHEALTH
    HealthModuleExit();
    LogInfo("-------------------- Health Module exit ------------------");
#endif

#ifdef FEATURE_LOG
    LogModuleExit();
    LogInfo("--------------------- Log Module exit --------------------");
    LogInfo("---------------------------------------------------------");
#endif
}
