#include "UnitTest.h"
#include "myMem.h"

#define UT_MEM_REGISTER_NAME                        "UT_MEM"

static int 
_UT_Mem_ForwardT(
    void
    )
{
    int ret = MY_SUCCESS;
    int memId = MY_MEM_MODULE_INVALID_ID;
    void* ptr = NULL;

    ret = MemModuleInit();
    if (ret)
    {
        UTLog("Init fail\n");
        goto CommonReturn;
    }

    ret = MemRegister(&memId, (char*)UT_MEM_REGISTER_NAME);
    if (ret)
    {
        UTLog("Register fail\n");
        goto CommonReturn;
    }

    ptr = MemCalloc(memId, sizeof(int));
    if (!ptr)
    {
        ret = -MY_ENOMEM;
        UTLog("calloc fail\n");
        goto CommonReturn;
    }
    
    if (ptr)
    {
        MemFree(memId, ptr);
    }
    if (!MemLeakSafetyCheckWithId(memId))
    {
        ret = -MY_EIO;
        UTLog("mem leak check fail\n");
        goto CommonReturn;
    }
    ret = MemUnRegister(&memId);
CommonReturn:
    if (MemModuleExit())
    {
        ret = -MY_EIO;
    }
    return ret;
}

TEST(UnitTest, Mem)
{
    EXPECT_EQ(MY_SUCCESS, _UT_Mem_ForwardT());
}

