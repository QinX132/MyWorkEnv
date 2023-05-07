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

#define     BOOL        unsigned char
#define     uint8_t     unsigned char
#define     uint16_t    unsigned short
#define     uint32_t    unsigned int
#define     uint64_t    unsigned long long
#define     __in
#define     __out
#define     LW_LOGE     printf
#define     LW_SUCCESS  0
#define     LW_ERR_T    int
#define     TRUE        1
#define     FALSE       0