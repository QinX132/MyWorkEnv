#ifndef _MY_INCLUDE_H_
#define _MY_INCLUDE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <signal.h>
#include <sys/file.h>

#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "event2/thread.h"

#include "myErrno.h"

#define BOOL                                unsigned char
#define uint8_t                             unsigned char
#define uint16_t                            unsigned short
#define uint32_t                            unsigned int
#define uint64_t                            unsigned long long
#define __in
#define __in_ecout(size)
#define __out
#define __out_ecout(size)
#define __inout
#define __inout_ecout(size)
#define TRUE                                1
#define FALSE                               0
#define UNUSED(_p_)                         ((void)(_p_))
#define MUST_CHECK                          __attribute__((warn_unused_result))
#define likely(x)                           __builtin_expect(!!(x), 1)
#define unlikely(x)                         __builtin_expect(!!(x), 0)

#define MY_BUFF_16                                  16
#define MY_BUFF_32                                  32
#define MY_BUFF_64                                  64
#define MY_BUFF_128                                 128
#define MY_BUFF_256                                 256
#define MY_BUFF_512                                 512
#define MY_BUFF_1024                                1024

#define MY_TCP_SERVER_PORT                          15000
#define MY_MAX_CLIENT_NUM_PER_SERVER                128
#define MY_KILL_SIGNAL                              SIGUSR1

#define MY_UATOMIC_INC(addr)                           __sync_fetch_and_add((addr), 1)
#define MY_UATOMIC_DEC(addr)                           __sync_fetch_and_add((addr), -1)

#endif /* _MY_INCLUDE_H_ */
