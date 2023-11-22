#ifndef _SERVER_MSG_HANDLER_H_
#define _SERVER_MSG_HANDLER_H_

#include "include.h"

#ifdef __cplusplus
extern "C"{
#endif

int 
ServerHandleMsg(
    int Fd,
    MY_MSG *Msg
    );

#ifdef __cplusplus
}
#endif

#endif /* _SERVER_MSG_HANDLER_H_ */
