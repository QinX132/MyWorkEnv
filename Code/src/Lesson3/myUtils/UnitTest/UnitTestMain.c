#include "UnitTest.h"

/*
int
UnitTestDemo(
    void
    )
{
    UTLog("Demo running...\n");
    return 0;
}

TEST(UnitTest, Demo)
{
    EXPECT_EQ(0, UnitTestDemo());
}
*/

GTEST_API_ 
int 
main(
    int argc,
    char **argv
    ) 
{
    printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
