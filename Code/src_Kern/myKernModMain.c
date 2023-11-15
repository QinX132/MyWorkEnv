#include "include.h"

static struct nf_hook_ops sg_Hooker;
static struct task_struct* sg_ModProc = NULL;

static unsigned int 
_PacketHookCb(
    unsigned int hook,
    struct sk_buff **Pskb,
    const struct net_device *in,
    const struct net_device *out,
    int (*okfn)(struct sk_buff*)
    )
{
    // 返回NF_ACCEPT，允许包继续传送
    return NF_ACCEPT;
}

static int 
_ModProc(
    void* arg
    )
{
    while(!kthread_should_stop())
    {
        ssleep(1);
        LogInfo("thread : %d\n", *((int*)arg));
        (*((int*)arg)) ++;
    }
    
    return 0;
}

static int __init _myModuleInit(void)
{
    int arg = 0;

    memset(&sg_Hooker, 0, sizeof(sg_Hooker));
    sg_Hooker.hook = _PacketHookCb;         // 钩子函数
    sg_Hooker.hooknum = NF_INET_PRE_ROUTING;    // 钩子位置：网络层前路由
    sg_Hooker.pf = PF_INET;               // 协议族：IPv4
    sg_Hooker.priority = NF_IP_PRI_FIRST; // 优先级：最高
    
    // 注册钩子函数
    nf_register_hook(&sg_Hooker);
    sg_ModProc = kthread_run(_ModProc, (void*)&arg, "myModProc");

    return 0;
}

static void __exit _myModuleExit(void)
{
    if (sg_ModProc)
    {
        nf_unregister_hook(&sg_Hooker);
        kthread_stop(sg_ModProc);
        sg_ModProc = NULL;
    }
    return ;
}

module_init(_myModuleInit);
module_exit(_myModuleExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yuzb");
MODULE_DESCRIPTION("yuzb kern mod test");
MODULE_VERSION("1.0");

