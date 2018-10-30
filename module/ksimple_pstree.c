#include <linux/init.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/list.h>
#include <net/sock.h>
#include <net/netlink.h>
#include<linux/kernel.h>

#define NETLINK_TEST 25
#define MAX_PAYLOAD_SIZE 30000

struct sock *nl_sk = NULL;
char msg[30000]="";

void sendnlmsg(int pid)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;

    if(!nl_sk) {
        return;
    }

    skb = nlmsg_new(MAX_PAYLOAD_SIZE, GFP_KERNEL);
    if(!skb) {
        printk(KERN_ERR "nlmsg_new error!\n");
    }
    nlh = nlmsg_put(skb, 0, 0, 0, MAX_PAYLOAD_SIZE, 0);

    memcpy(NLMSG_DATA(nlh), msg, sizeof(msg));

    netlink_unicast(nl_sk, skb, pid, MSG_DONTWAIT);
}
void find_parent( int pidNum )
{
    pid_t pid = pidNum;
    struct task_struct *p;
    int count[100];
    int i = 99;
    int pidCount = 0;
    int temp;
    // init
    while(i) {
        count[i] = 0;
        i = i-1;
    }
    count[0] = 0;
    i = 0;
    // find the pidNum's task_struct
    p = pid_task(find_vpid(pid), PIDTYPE_PID);
    // if parent's pid == 0
    if(p->parent->pid==0) {
        sprintf(msg+strlen(msg),"0\n");
        sprintf(msg+strlen(msg),"\t%s(%d)\n",p->comm, p->pid);
    }
    // else condition
    else {
        // find all the pid of its parent
        while(p->parent->pid != 0) {
            if(p->parent == NULL) {
                printk("No Parent\n");
            } else {
                count[i] = p->parent->pid;
            }
            i = i+1;
            p = p->parent;
        }
        // print all the parent depend on the pid array(count[])
        i = i-1;
        while(i) {
            p = pid_task(find_vpid(count[i]), PIDTYPE_PID);
            temp = pidCount;
            while(pidCount) {
                //printk("\t");
                sprintf(msg+strlen(msg),"\t");
                pidCount = pidCount-1;
            }
            //printk("baby: %d %s\n", p->pid, p->comm);
            sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
            i = i - 1;

            pidCount = temp + 1;
        }
        // print the count[0] pid information
        temp = pidCount;
        while(pidCount) {
            //printk("\t");
            sprintf(msg+strlen(msg),"\t");
            pidCount = pidCount-1;
        }
        p = pid_task(find_vpid(count[0]), PIDTYPE_PID);
        sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
        // print itself information
        pidCount = temp +1;
        while(pidCount) {
            sprintf(msg+strlen(msg),"\t");
            pidCount = pidCount-1;
        }
        p = pid_task(find_vpid(pid), PIDTYPE_PID);
        sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
    }
}
void find_child( int pidNum, int tabNum )
{
    pid_t pid = pidNum;
    struct task_struct *p;
    struct list_head *pp=NULL;
    struct task_struct *psibling;
    int i;

    p = pid_task(find_vpid(pid), PIDTYPE_PID);

    // find all the child
    list_for_each(pp, &p->children) {
        psibling = list_entry(pp, struct task_struct, sibling);
        i = tabNum;
        // print \t depend on the para tabNum
        while(i) {
            sprintf(msg+strlen(msg),"\t");
            i = i-1;
        }
        // print the next level of child
        sprintf(msg+strlen(msg),"%s(%d)\n",psibling->comm, psibling->pid);
        // if child of child is not empty, then going to the next recursive
        if(!list_empty(&psibling->children))
            find_child(psibling->pid,tabNum+1);
    }
}
void find_sibling(int pidNum)
{
    pid_t pid = pidNum;
    struct task_struct *p;
    struct list_head *pp=NULL;
    struct task_struct *psibling;

    p = pid_task(find_vpid(pid), PIDTYPE_PID);

    // find all the sibling
    list_for_each(pp, &p->parent->children) {
        psibling = list_entry(pp, struct task_struct, sibling);
        if(psibling->pid != pidNum )
            sprintf(msg+strlen(msg),"%s(%d)\n",psibling->comm, psibling->pid);
    }

}
void nl_data_ready(struct sk_buff *__skb)
{
    struct sk_buff *skb;
    struct nlmsghdr *nlh;
    struct task_struct *p;
    char str[100]="";
    int i = 2;
    int pid = 0;


    skb = skb_get (__skb);
    if(skb->len >= NLMSG_SPACE(0)) {
        nlh = nlmsg_hdr(skb);
        // get message(str[]) from user
        memcpy(str, NLMSG_DATA(nlh), sizeof(str));
        //convert string pid to int pid
        while(str[i]!='\0') {
            pid = pid*10+str[i]-'0';
            i = i +1;
        }

        // clear the msg[]
        memset(msg,0,sizeof msg);

        // mode = 0 -> default/-c
        // mode = 1 -> -c+pid
        // mode = 2 -> -s
        // mode = 3 -> -s+pid
        // mode = 4 -> -p
        // mode = 5 => -p+pid
        switch(str[1]) {
        case 'c':
            if(str[2]) {
                // mode = 1;
                p = pid_task(find_vpid(pid), PIDTYPE_PID);
                sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
                find_child(pid,1);
            } else {
                // mode = 0;
                p = pid_task(find_vpid(1), PIDTYPE_PID);
                sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
                find_child(1,1);
            }
            break;
        case 's':
            if(str[2]) {
                // mode = 3;
                find_sibling(pid);
            } else {
                // mode = 2;
                find_sibling(current->pid);
            }
            break;
        case 'p':
            if(str[2]) {
                // mode = 5;
                find_parent(pid);
            } else {
                // mode = 4;
                find_parent(current->pid);
            }
            break;
        default:
            if(str[0] == '\0') {
                // mode = 0;
                p = pid_task(find_vpid(1), PIDTYPE_PID);
                sprintf(msg+strlen(msg),"%s(%d)\n",p->comm, p->pid);
                find_child(1,1);
            } else
                sprintf(msg+strlen(msg),"doesn't exist!\n");
        }
        sendnlmsg(nlh->nlmsg_pid);
        kfree_skb(skb);
    }
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