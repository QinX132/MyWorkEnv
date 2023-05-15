#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#define     BOOL                        unsigned char
#define     uint8_t                     unsigned char
#define     uint16_t                    unsigned short
#define     uint32_t                    unsigned int
#define     uint64_t                    unsigned long long
#define     __in
#define     __out
#define     LW_LOGE                     printf
#define     LW_SUCCESS                  0
#define     LW_ERR_T                    int
#define     TRUE                        1
#define     FALSE                       0


#define     MY_TEST_PORT                13231
#define     MY_TEST_SESSION_ID_MAX_LEN  64
#define     MY_TEST_DISCONNECT_STRING   "disconnect"

typedef struct _MY_TEST_MSG_HEAD{
    uint32_t Id;
    uint64_t TimeStamp;
    uint32_t MsgContentLen;
}MY_TEST_MSG_HEAD;

typedef struct _MY_TEST_MSG_Cont{
    uint32_t MagicVer;
    uint32_t SessionId;
    char VarLenCont[0];
}MY_TEST_MSG_CONT;

typedef struct _MY_TEST_MSG{
    MY_TEST_MSG_HEAD Head;
    MY_TEST_MSG_CONT Cont;
}MY_TEST_MSG;