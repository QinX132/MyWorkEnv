#ifndef _MY_CLIENT_SERVER_MSG_H_
#define _MY_CLIENT_SERVER_MSG_H_

typedef enum _MY_MSG_TYPE{
    MY_MSG_TYPE_START_UNSPEC                        = 0,        // min = 0x0

// Manage Msg
    MY_MSG_TYPE_REGISTER_TO_SERVER                  = 1,
    MY_MSG_TYPE_REGISTER_TO_SERVER_REPLY            = 2,

// Business Msg
    MY_MSG_TYPE_EXEC_CMD                            = 10000,
    MY_MSG_TYPE_EXEC_CMD_REPLY                      = 10001,
    MY_MSG_TYPE_UPLOAD_FILE_TO_SERVER               = 10002,
    MY_MSG_TYPE_UPLOAD_FILE_TO_SERVER_REPLY         = 10003,
    MY_MSG_TYPE_DOWNLOAD_FILE_FROM_SERVER           = 10004,
    MY_MSG_TYPE_DOWNLOAD_FILE_FROM_SERVER_REPLY     = 10005,


    MY_MSG_TYPE_MAX                                 = 65535     // max = 0xffff
}
MY_MSG_TYPE;

#endif /* _MY_CLIENT_SERVER_MSG_H_ */
