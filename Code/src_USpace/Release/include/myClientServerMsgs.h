#ifndef _MY_CLIENT_SERVER_MSG_H_
#define _MY_CLIENT_SERVER_MSG_H_

#define MY_DISCONNECT_STRING                                    "disconnect"

typedef enum _MY_MSG_TYPE{
    MY_MSG_TYPE_START_UNSPEC            = 0,        // min = 0x0
    MY_MSG_TYPE_EXEC_CMD                = 1,
    MY_MSG_TYPE_EXEC_CMD_REPLY,
    
    MY_MSG_TYPE_MAX                     = 65535     // max = 0xffff
}
MY_MSG_TYPE;

#endif /* _MY_CLIENT_SERVER_MSG_H_ */
