#include "UnitTest.h"
#include "myCommonUtil.h"

static int
_UT_CommonUtil_ByteOrderU64(
    void
    )
{
    int ret = MY_SUCCESS;
    uint64_t before = 0x12345678, afterShouldBe = 0x78563412, after = 0;

    after = MyUtil_htonll(before);
    if (after != afterShouldBe || MyUtil_ntohll(after) != before)
    {
        ret = -MY_EIO;
        goto CommonReturn;
    }
    
CommonReturn:
    return ret;
}

TEST(UnitTest, CommonUtil)
{
    EXPECT_EQ(MY_SUCCESS, _UT_CommonUtil_ByteOrderU64());
}
