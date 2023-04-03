#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<semaphore.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<fcntl.h>

#define M 530
#define N 5
#define BUFSIZE 10

int main()
{
	sem_t *empty,*full,*mutex;/* mutex 是用来加锁的*/
	int fd;    /*共享缓冲区文件描述符*/
	int i,j,k,child; 
	int data;       /*写入的数据*/
	pid_t pid;
	int buf_out;    /*从缓冲区读取位置*/
	int buf_in;     /*写入缓冲区的位置*/

    /*打开信号量   sys_open用来创建一个信号量，或者打开一个信号量*/
    empty = sem_open("empty",O_CREAT|O_EXCL,0644,BUFSIZE);   /*用empty信号量表示剩余资源，初始化为10，也就是整个缓冲区的大小*/
    full = sem_open("full",O_CREAT|O_EXCL,0644,0);          /*已使用的资源，初始化为0*/
    mutex = sem_open("mutex",O_CREAT|O_EXCL,0644,1);        /*互斥量，初始化为1*/
    
    fd = open("buffer.txt",O_CREAT|O_TRUNC|O_RDWR,0666);      /*打开buffer文件,并将文件描述符复制赋给fd*/
    lseek(fd,BUFSIZE*sizeof(int),SEEK_SET);             /*lseek是用来文件偏移的，相当于从文件开始处偏10*4个字节，刷新40个字节的缓冲区*/
    write(fd,(char *)&buf_out,sizeof(int));             /*注意上一句代码已经将偏移(光标)指向的第40个字节，所以这里是将buf_out强转后写到buffer的第40个字节,当我们下一个消费者进程去数字时就去buf_out处取，然后buf_out++,相当于多进程下的通信的，因为只有一个生产者进程，所以我们不用存储buf_in,直接用就可以了*/      
    /*生产者进程*/
    if((pid=fork())==0) /*这里产生了一个进程，子进程用来当生产者进程*/
    {
        printf("I am producer . pid = %d\n",getpid());
        /*只有一个生产者进程，所以这个进程需要产生所有的产品*/
        for(i = 0;i < M;i++)
        {
            sem_wait(empty);   /*消耗一个empty信号量，这里巧妙之处在于我们empty最多是减少10次，也就是说在buffer中写10个数以后就一定会让当前进程阻塞，然后指向消费者进程来读取并删除数据*/
            sem_wait(mutex);   /*mutex会变为0，所以调用会阻塞，直到后面释放mutex（也就是生产者生产一个农品后），这就保证了同一时间只有一个进程可以访问缓冲区，消费者进程不可访问*/
            /*写入一个字符*/
            lseek(fd,buf_in*sizeof(int),SEEK_SET);  /*初始光标移到第0个字节*/
            write(fd,(char *)&i,sizeof(int));      /*把i写到光标指向的位置，例如一开始是把0写到缓冲区0~3字节*/
            buf_in = (buf_in + 1) % 10;            /*文件缓存区只能容纳10个整数

            sem_post(mutex);      /*解锁*/
            sem_post(full);       /*增加一个产品*/
        }
        printf("producer end.\n");
        fflush(stdout);
        return 0;       /*生产者进程结束*/
    }
    else if(pid < 0)      /*进程创建不成功*/
    {
        perror("Fail to fork!\n");
        return -1;
    }
    /*消费者进程*/
    for(j = 0;j<N;j++)          /*创建N个进程*/
    {
        if((pid=fork())==0) 
        {
            for(k=0;k<M/N;k++)      /*每个进程执行M/N个任务*/
            {
                sem_wait(full);     /*减少一个产品*/
                sem_wait(mutex);     /*上锁*/

                lseek(fd,BUFSIZE*sizeof(int),SEEK_SET);      /*将光标指向第40个字节*/
                read(fd,(char *)&buf_out,sizeof(int));      /*将第四十个字节处的数据赋值给buf_out*/

                lseek(fd,buf_out*sizeof(int),SEEK_SET);     /*找到现在消费者进程需要找的取数位置*/
                read(fd,(char *)&data,sizeof(int));         /*date 存要取的数*/

                buf_out = (buf_out+1) % BUFSIZE;
                lseek(fd,BUFSIZE*sizeof(int),SEEK_SET);
                write(fd,(char *)&buf_out,sizeof(int));     /*这里我们要将第40字节处的buf_out加一，以便于下一个进程来取的时候自动加1*/

                sem_post(mutex);        /*解锁*/
                sem_post(empty);        /*消费一个产品，共享区中剩余资源++，唤醒生产者进程*/

            }
            printf("child-%d: pid = %d end.\n",j,getpid());
            return 0;
        }
        else if(pid<0)
        {
            perror("Fail to fork!\n");
            return -1;
        }
    }
    /*回收线程资源*/
    child = N + 1;
    while(child--)
    {
        wait(NULL);
    }
    sem_unlink("full");
    sem_unlink("empty");
    sem_unlink("mutex");
    /*释放资源*/
    close(fd);
    return 0;
}