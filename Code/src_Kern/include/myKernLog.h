#ifndef _MY_KERN_LOG_H_
#define _MY_KERN_LOG_H_

#define LogInfo(Fmt, ...)               printk(KERN_INFO, Fmt, ##__VA_ARGS__)
#define LogDbg(Fmt, ...)                printk(KERN_DEBUG, Fmt, ##__VA_ARGS__)
#define LogWarn(Fmt, ...)               printk(KERN_WARNING, Fmt, ##__VA_ARGS__)
#define LogErr(Fmt, ...)                printk(KERN_ERR, Fmt, ##__VA_ARGS__)

#endif /* _MY_KERN_LOG_H_ */
