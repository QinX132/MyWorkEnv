#ifndef _UNIT_TEST_H_
#define _UNIT_TEST_H_

#include "myErrno.h"
#include "gtest/gtest.h"
#include "include.h"

#define UTLog(Fmt, ...)                 printf("%s %d:" Fmt, __func__, __LINE__, ##__VA_ARGS__)

#endif
