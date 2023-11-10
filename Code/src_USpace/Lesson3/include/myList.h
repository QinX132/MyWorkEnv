#ifndef _MY_LIST_H_
#define _MY_LIST_H_

#include "include.h"

typedef struct _MY_LIST_NODE{
    struct _MY_LIST_NODE *Prev;
    struct _MY_LIST_NODE *Next;
}
MY_LIST_NODE;

#define MY_LIST_NODE_INIT(_NODE_)  \
        do{ \
            (_NODE_)->Prev = (_NODE_); \
            (_NODE_)->Next = (_NODE_); \
        }while(0)
    
#define MY_LIST_IS_EMPTY(_NODE_HEAD_) \
        ((_NODE_HEAD_)->Prev == (_NODE_HEAD_) && (_NODE_HEAD_)->Next == (_NODE_HEAD_))
    
#define MY_LIST_ADD_TAIL(_NODE_ADD_, _NODE_HEAD_)  \
        do{ \
            (_NODE_HEAD_)->Prev->Next = (_NODE_ADD_); \
            (_NODE_ADD_)->Prev = (_NODE_HEAD_)->Prev; \
            (_NODE_HEAD_)->Prev = (_NODE_ADD_); \
            (_NODE_ADD_)->Next = (_NODE_HEAD_); \
        }while(0)
    
#define MY_LIST_ENTRY(_NODE_, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_) \
        ((_NODE_)->Next != (_NODE_) ? \
        ((_NODE_STRUCT_TYPE_ *)((char *)(_NODE_) - (size_t)(&((_NODE_STRUCT_TYPE_ *)0)->_NODE_OFFSET_NAME_))) : \
        NULL)
    
#define MY_LIST_FIRST_ENTRY(_HEAD_, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_) \
        MY_LIST_ENTRY((_HEAD_)->Next, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_)
    
#define MY_LIST_FOR_EACH(_NODE_HEAD_, _NODE_LOOP_, _NODE_TMP_, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_) \
        for((_NODE_LOOP_) = MY_LIST_ENTRY((_NODE_HEAD_)->Next, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_), \
                (_NODE_TMP_) = (_NODE_LOOP_) ? MY_LIST_ENTRY((_NODE_LOOP_)->_NODE_OFFSET_NAME_.Next, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_) : NULL; \
            (_NODE_LOOP_) != NULL && (&(_NODE_LOOP_)->_NODE_OFFSET_NAME_ != _NODE_HEAD_); \
            (_NODE_LOOP_) = (_NODE_TMP_), \
                (_NODE_TMP_) = (_NODE_TMP_) ? MY_LIST_ENTRY((_NODE_TMP_)->_NODE_OFFSET_NAME_.Next, _NODE_STRUCT_TYPE_, _NODE_OFFSET_NAME_) : NULL)
    
#define MY_LIST_DEL_NODE(_NODE_) \
        do{ \
            (_NODE_)->Prev->Next = (_NODE_)->Next; \
            (_NODE_)->Next ->Prev = (_NODE_)->Prev; \
        }while(0)


#endif /* _MY_LIST_H_ */
