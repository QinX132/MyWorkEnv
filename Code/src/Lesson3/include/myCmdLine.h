#ifndef _MY_CMD_LINE_H_
#define _MY_CMD_LINE_H_

#include "include.h"
#include "myModuleCommon.h"

#define MY_TEST_CMDLINE_ARG_LIST                 \
        __MY_TEST_ARG("start", "start this program")  \
        __MY_TEST_ARG("showstat", "show this program's modules stats")  \
        __MY_TEST_ARG("stop", "stop this program")  \
        __MY_TEST_ARG("help", "show this page")  \

// RoleName Can Be NULL
void
CmdLineUsage(
    char* RoleName
    );

typedef enum _MY_TEST_CMDLINE_ROLE
{
    MY_TEST_CMDLINE_ROLE_SVR,
    MY_TEST_CMDLINE_ROLE_CLT,
    
    MY_TEST_CMDLINE_ROLE_UNUSED
}
MY_TEST_CMDLINE_ROLE;

typedef enum _MY_TEST_CMD_TYPE
{
    MY_TEST_CMD_TYPE_START,
    MY_TEST_CMD_TYPE_SHOWSTAT,
    MY_TEST_CMD_TYPE_STOP,
    MY_TEST_CMD_TYPE_HELP,
    
    MY_TEST_CMD_TYPE_UNUSED
}
MY_TEST_CMD_TYPE;

typedef struct _MY_TEST_CMDLINE_CONT
{
    char* Opt;
    char* Help;
}
MY_TEST_CMDLINE_CONT;

#ifndef _SG_CMDLINECONT_
#define _SG_CMDLINECONT_
static const MY_TEST_CMDLINE_CONT sg_CmdLineCont[MY_TEST_CMD_TYPE_UNUSED] = 
{
    #undef __MY_TEST_ARG
    #define __MY_TEST_ARG(_opt_,_help_) \
        {_opt_, _help_},
    MY_TEST_CMDLINE_ARG_LIST
};
#endif /* _SG_CMDLINECONT_ */

void Server_Exit(void);
void Client_Exit(void);

int
CmdLineModuleInit(
    char* RoleName,
    int Argc,
    char* Argv[]
    );

void
CmdLineModuleExit(
    void
    );

#endif /* _MY_CMD_LINE_H_ */
