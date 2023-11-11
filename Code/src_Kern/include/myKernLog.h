#ifndef _MY_KERN_LOG_H_
#define _MY_KERN_LOG_H_

#define MY_KER_MOD_NAME                 "<myKerMod>"

#define LogInfo(Fmt, ...)               printk(KERN_INFO MY_KER_MOD_NAME"[%s-%d]:"Fmt, \
                                            __func__, __LINE__, ##__VA_ARGS__)
#define LogDbg(Fmt, ...)                printk(KERN_DEBUG MY_KER_MOD_NAME"[%s-%d]:"Fmt, \
                                            __func__, __LINE__, ##__VA_ARGS__)
#define LogWarn(Fmt, ...)               printk(KERN_WARNING MY_KER_MOD_NAME"[%s-%d]:"Fmt, \
                                            __func__, __LINE__, ##__VA_ARGS__)
#define LogErr(Fmt, ...)                printk(KERN_ERR MY_KER_MOD_NAME"[%s-%d]:"Fmt, \
                                            __func__, __LINE__, ##__VA_ARGS__)

#endif /* _MY_KERN_LOG_H_ */
