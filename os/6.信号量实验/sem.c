#include<linux/sem.h>
#include<linux/sched.h>
#include<unisted.h>
#include<asm/segment.h>
#include<linux/tty.h>
#include<linux/kerner.h>
#include<linux/fdreg.h>
#include<asm/system.h>
#include<asm/io.h>

sem_t semtable[SEMTABLE_LEN];   /*用来存信号量的数组,存的是名字*/
int cnt = 0;                    /*用来存信号量数组的长度*/

sem_t *sys_sem_open(const char *name,unsigned int value)
{
    char kernelname[100];   /*用来存信号量名字*/
    int isExist = 0;
    int i = 0;
    int name_cnt = 0;
    while(get_fs_byte(name+name_cnt) != '\0')   /*name是一个初地址，用这个方法可以求出name的长度，长度返回给了name_cnt*/
    name_cnt++;
    if(name_cnt > SEM_NAME_LEN)
    return NULL;
    for(i=0;i<name_cnt;i++)
    kernelname[i] = get_fs_byte(name+i);
    int name_len = strlen(kernelname);
    int sem_name_len = 0;
    sem_t *p=NULL;
    for(i=0;i<cnt;i++)
    {
        sem_name_len = strlen(semtable[i].name);
        if(sem_name_len == name_len)    /*如果需要查找的信号量的长度等于信号量数组中某个信号量的长度，那么就进一步判断这两个的名字是不是相等的*/
        {
            if(!strcmp(kernelname,semtable[i].name))
            {
                isExist = 1;
                break;
            }
        }
    }
    if(isExist == 1)
    {
        p = (sem_t*)((&semtable[i]))    /*因为这里i是全局变量，所以跳出循环后也是可以用的*/
    }
    else        /*当不存在这个信号量的时候，就新建一个（建在信号量数组的最后一个）*/
    {
        i =0;
        for(i=0;i<name_len;i++)
        {
            semtable[cnt].name[i] = kernelname[i];
        }
        semtable[cnt].value = value;    /*value是传过来的值，即创建这个信号量的初值*/
        p=(sem_t*)(semtable[cnt]);
        cnt++;
    }
    return p;   /*返回的是这个信号量的地址，*p就是这个信号量*/
}
