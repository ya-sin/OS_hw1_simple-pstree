#include <linux/init.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/list.h>
#include <net/sock.h>
#include <net/netlink.h>
#include<linux/kernel.h>
// #include <linux/string.h>
// #include <linux/moduleparam.h>

#define NETLINK_TEST 25
#define MAX_PAYLOAD_SIZE 1024

struct sock *nl_sk = NULL;
// static pid_t pid=482;
// module_param(pid,int,0644);

void sendnlmsg(int pid)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    char msg[30] = "Say hello from kernel!";

    if(!nl_sk) {
        return;
    }

    skb = nlmsg_new(MAX_PAYLOAD_SIZE, GFP_KERNEL);
    if(!skb) {
        printk(KERN_ERR "nlmsg_new error!\n");
    }
    nlh = nlmsg_put(skb, 0, 0, 0, MAX_PAYLOAD_SIZE, 0);

    memcpy(NLMSG_DATA(nlh), msg, sizeof(msg));
    printk("Send message '%s'.\n",(char *)NLMSG_DATA(nlh));

    netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
}
void find_parent( int pidNum )
{
    pid_t pid = pidNum;
    //module_param(pid,int,0644);
    struct task_struct *p;
    struct list_head *pp="";// why need to initilized
    struct task_struct *psibling;

    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    printk("me: %d %s\n", p->pid, p->comm);

    while(p->parent->pid != 0) {
        // 父进程
        if(p->parent == NULL) {
            printk("No Parent\n");
        } else {

            printk("Parent: %d %s\n", p->parent->pid, p->parent->comm);
        }
        p = p->parent;
    }
}
void find_child( int pidNum )
{
    pid_t pid = pidNum;
    //module_param(pid,int,0644);
    struct task_struct *p;
    struct list_head *pp="";// why need to initilized
    struct task_struct *psibling;

    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    printk("%s(%d)\n", p->comm, p->pid);

    list_for_each(pp, &p->children) {
        psibling = list_entry(pp, struct task_struct, sibling);
        printk("\t%s(%d)\n", psibling->comm, psibling->pid);
        if(!list_empty(&psibling->children))
            find_child(psibling->pid);
    }
}
void find_sibling(int pidNum)
{
    pid_t pid = pidNum;
    //module_param(pid,int,0644);
    struct task_struct *p;
    struct list_head *pp="";// why need to initilized
    struct task_struct *psibling;


    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    printk("%s(%d)\n", p->comm, p->pid);

    list_for_each(pp, &p->parent->children) {
        psibling = list_entry(pp, struct task_struct, sibling);
        if(psibling->pid != pidNum )
            printk("%s(%d)\n", psibling->comm, psibling->pid);
    }

}
void nl_data_ready(struct sk_buff *__skb)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    char str[100]="";
    int mode = 0;
    int i = 2;
    int pid = 0;
    // get current pid
    // struct pid* cur_pid = find_get_pid(current->pid);
    // struct task_struct* cur_task = pid_task(cur_pid, PIDTYPE_PID);
    // struct task_struct *p;
    // struct list_head *pp;
    // struct task_struct *psibling;
    // printk("current pid level: %d\n",cur_pid->numbers[cur_pid->level].nr);
    // printk("current pid%d\n", current->pid);
    // // get current tasks
    // printk("current task piority: %d\n",cur_task->prio);
    // printk("current task pid: %d\n",cur_task->pid);
    // printk("current task name: %s\n",cur_task->comm);

    //long int PID;
    //char pid[6];

    skb = skb_get (__skb);
    if(skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(skb);

        memcpy(str, NLMSG_DATA(nlh), sizeof(str));
        sendnlmsg(nlh->nlmsg_pid);
        kfree_skb(skb);
    }

    // while(str[i] != '\0'){
    //     number = number*10+str[i]-'0';
    // }
    while(str[i]!='\0') {
        pid = pid*10+str[i]-'0';
        i = i +1;
    }
    printk("Netlink: pid is %d\n",pid);

    // number = strlen(str);
    // printk("str: %s\n",str);
    // simple_strtoul(str+2,&(str+5),10);
    // mode = 0 -> default/-c
    // mode = 1 -> -c+pid
    // mode = 2 -> -s
    // mode = 3 -> -s+pid
    // mode = 4 -> -p
    // mode = 5 => -p+pid
    switch(str[0]) {
    case 'c':
        if(str[2]) {
            //kstrtol(str+2, 10, PID);
            mode = 1;
            find_child(pid);
        } else {
            mode = 0;
            find_child(1);
            printk("Netlink: hQQ2");
        }
        break;
    case 's':
        if(str[2]) {
            mode = 3;
            find_sibling(pid);
        } else {
            mode = 2;
            find_sibling(current->pid);
        }
        break;
    case 'p':
        if(str[2]) {
            mode = 5;
            find_parent(pid);
        } else {
            mode = 4;
            find_parent(current->pid);
        }
        break;
    default:
        if(str[0] == '\0') {
            find_child(1);
            mode = 0;
        } else
            printk("doesn't exist!");
    }
    // printf("%s\n",pid);
    // printf("%d\n",mode);

    printk("Netlink: Message received mode:%d\n",mode);
    // for_each_process(p){
    //     if(p->mm == NULL)
    //         printk(KERN_ALERT"name: %s\tpid: %d\n",p->comm,p->pid);
    // }
    // p = pid_task(find_vpid(pid), PIDTYPE_PID);
    // printk("me: %d %s\n", p->pid, p->comm);

    // // 父进程
    // if(p->parent == NULL) {
    //     printk("No Parent\n");
    // }
    // else {
    //     printk("Parent: %d %s\n", p->parent->pid, p->parent->comm);
    // }

    // list_for_each(pp, &p->parent->children)
    // {
    //     psibling = list_entry(pp, struct task_struct, sibling);
    //     printk("sibling %d %s \n", psibling->pid, psibling->comm);
    // }


    // list_for_each(pp, &p->children)
    // {
    //     psibling = list_entry(pp, struct task_struct, sibling);
    //     printk("children %d %s \n", psibling->pid, psibling->comm);
    // }
}

static int netlink_unicast_init(void)
{
    struct netlink_kernel_cfg netlink_kerncfg = {
        .input = nl_data_ready,
    };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_TEST, &netlink_kerncfg);

    if(!nl_sk) {
        printk(KERN_ERR "netlink_unicast_init: Create netlink socket error.\n");
        return -1;
    }

    printk("netlink_unicast_init: Create netlink socket ok.\n");
    return 0;
}

static void netlink_unicast_exit(void)
{
    if(nl_sk != NULL) {
        sock_release(nl_sk->sk_socket);
    }

    printk("netlink_unicast_exit!\n");
}

module_init(netlink_unicast_init);
module_exit(netlink_unicast_exit);

MODULE_AUTHOR("X-SLAM XINU");
MODULE_LICENSE("GPL");