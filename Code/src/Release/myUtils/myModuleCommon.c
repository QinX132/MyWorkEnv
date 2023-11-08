#include "myModuleCommon.h"

int
MyModuleCommonInit(
    MY_MODULES_INIT_PARAM ModuleInitParam 
    )
{
    int ret = 0;
    
    if (ModuleInitParam.LogArg)
    {
        ret = LogModuleInit(ModuleInitParam.LogArg);
        if (ret)
        {
            LogErr("Init log failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
        LogInfo("Log start...");
    }
    
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
    LogInfo("---------------------------------------------------------");
    LogInfo("|-------------------%12s -----------------------|", ModuleInitParam.LogArg->RoleName);
    LogInfo("---------------------------------------------------------");
    
    if (ModuleInitParam.InitMsgModule)
    {
        ret = MsgModuleInit();
        if (ret)
        {
            LogErr("Msg module init failed! %d %s", ret, My_StrErr(ret));
            goto CommonReturn;
        }
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

    if (ModuleInitParam.HealthArg)
    {
        ret = HealthModuleInit(ModuleInitParam.HealthArg);
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

    (void)MsgModuleExit();
    LogInfo("-------------------- Msg Module exit ---------------------");

    (void)TPoolModuleExit();
    LogInfo("-------------------- TPool Module exit -------------------");

    (void)HealthModuleExit();
    LogInfo("-------------------- Health Module exit ------------------");

    (void)TimerModuleExit();
    LogInfo("-------------------- Timer Module exit -------------------");

    (void)MemModuleExit();
    LogInfo("-------------------- Mem Module exit -------------------");
    
    LogInfo("----------------------------------------------------------");
    LogModuleExit();
}
