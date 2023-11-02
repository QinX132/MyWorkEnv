#include "myModuleCommon.h"

static const char* sg_ModulesName[MY_MODULES_ENUM_MAX] = 
{
    [MY_MODULES_ENUM_LOG]       =   "Log",
    [MY_MODULES_ENUM_MSG]       =   "Msg",
    [MY_MODULES_ENUM_TPOOL]     =   "TPool",
    [MY_MODULES_ENUM_CMDLINE]   =   "CmdLine",
    [MY_MODULES_ENUM_MHEALTH]   =   "MHealth",
    [MY_MODULES_ENUM_MEM]       =   "Mem",
    [MY_MODULES_ENUM_TIMER]     =   "Timer"
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
    
    ret = MemModuleInit();
    if (ret)
    {
        LogErr("Mem module init failed! %d %s", ret, My_StrErr(ret));
        goto CommonReturn;
    }

    if (ModuleInitParam.CmdLineArg)
    {
        ret = CmdLineModuleInit(ModuleInitParam.CmdLineArg);
        if (ret)
        {
            if (MY_ERR_EXIT_WITH_SUCCESS != ret)
            {
                LogErr("Cmd line init failed! %d %s", ret, My_StrErr(ret));
            }
            goto CommonReturn;
        }
    }
    
    if (ModuleInitParam.LogArg)
    {
        ret = LogModuleInit(ModuleInitParam.LogArg);
        if (ret)
        {
            LogErr("Init log failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        LogInfo("---------------------------------------------------------");
        LogInfo("|-------------------%12s -----------------------|", ModuleInitParam.LogArg->RoleName);
        LogInfo("---------------------------------------------------------");
        LogInfo("---------------- Log Module init success ----------------");
    }

    if (ModuleInitParam.InitMsgModule)
    {
        MsgModuleInit();
        LogInfo("---------------- Msg Module init success ----------------");
    }

    if (ModuleInitParam.TPoolArg)
    {
        ret = TPoolModuleInit(ModuleInitParam.TPoolArg);
        if (ret)
        {
            LogErr("Init TPool failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        LogInfo("---------------- TPool Module init success ---------------");
    }

    if (ModuleInitParam.InitHealthModule)
    {
        ret = HealthModuleInit();
        if (ret)
        {
            LogErr("Init HealthModule failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        LogInfo("---------------- Health Module init success --------------");
    }

    if (ModuleInitParam.InitTimerModule)
    {
        ret = TimerModuleInit();
        if (ret)
        {
            LogErr("Init Timer module failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        LogInfo("---------------- Timer Module init success --------------");
    }

CommonReturn:
    return ret;
}

void
MyModuleCommonExit(
    void
    )
{
    CmdLineModuleExit();

    MsgModuleExit();
    LogInfo("-------------------- Msg Module exit ---------------------");

    TPoolModuleExit();
    LogInfo("-------------------- TPool Module exit -------------------");

    HealthModuleExit();
    LogInfo("-------------------- Health Module exit ------------------");

    TimerModuleExit();
    LogInfo("-------------------- Timer Module exit -------------------");

    LogInfo("----------------------------------------------------------");
    LogModuleExit();

    MemModuleExit();
}
