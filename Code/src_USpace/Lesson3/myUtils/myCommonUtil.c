#include "myCommonUtil.h"

#include <sys/resource.h>
#include <sys/time.h>
void
MyUtil_MakeDaemon(
    void
    )
{
    pid_t pid = fork();
    if (pid != 0)
    {
        exit(0);
    }
    setsid();
    return;
}

int
MyUtil_IsProcessRunning(
    int Fd
    )
{
    int ret = 0;

    /*lock pid file*/
    ret = flock(Fd, LOCK_EX | LOCK_NB);
    if (ret < 0)
    {
        if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
        {
            ret = 1;
        }
    }
    return ret;
}

int
MyUtil_OpenPidFile(
    char* Path
    )
{
    return Path ? open(Path, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) : -1;
}

int
MyUtil_SetPidIntoFile(
    int Fd
    )
{
    int32_t ret = 0;
    char buf[MY_TEST_BUFF_32] = {0};
    ssize_t len = 0;

    ftruncate(Fd, 0);
    sprintf(buf, "%u", getpid());
    len = write(Fd, buf, strlen(buf) + 1);
    if (len != (ssize_t)(strlen(buf) + 1))
    {
        ret = MY_EIO;
        goto CommonReturn;
    }
    
CommonReturn:
    return ret;
}

void 
MyUtil_CloseStdFds(
    void
    )
{
    int32_t fd = -1;
    (void)close(STDERR_FILENO);
    (void)close(STDOUT_FILENO);
    (void)close(STDIN_FILENO);
    fd = open("/dev/null", O_RDONLY);   /* fd == 0: stdin.  */
    fd = open("/dev/null", O_WRONLY);   /* fd == 1: stdout. */
    fd = dup(fd);
}

int
MyUtil_GetCpuTime(
    uint64_t *TotalTime,
    uint64_t *IdleTime
    )
{
    int ret = MY_SUCCESS;
    FILE* fp = NULL;
    unsigned long long user = 0, nice = 0, system = 0, idle = 0;
    fp = fopen("/proc/stat", "r");
    if (!fp || !TotalTime || !IdleTime) 
    {
        ret = MY_EIO;
        goto CommonReturn;
    }

    if (fscanf(fp, "cpu %llu %llu %llu %llu", &user, &nice, &system, &idle) != 4) 
    {
        ret = MY_EIO;
        goto CommonReturn;
    }

    *TotalTime = user + nice + system + idle;
    *IdleTime = idle;

CommonReturn:
    if (fp)
    {
        fclose(fp);
    }
    return ret;
}
