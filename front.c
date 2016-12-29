#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "msg.h"
#include <wait.h>

void main() {int flag = 1;//几个参数，控制循环次数flag
int status = -1;//等待子进程（server）执行完的标志
int msgid, filedsc;char buffer[BUFFER_SIZE];char *command = NULL;//指令格式
    struct msg_wrapper data;//命令头加参数都作为一个data，连起来
    char *stack[100];//就看一个命令行有几个参数，就有几个stack[N]
    int top, i;
    pid_t fpid;//进程号
    fpid = fork();
    if (fpid == 0) {//如果是子进程
        //要执行的文件路径，参数列表（用null结束）；进程就切过去了，本篇后面不会执行
        execl("./server", "server", "", NULL);
        perror("server");
        exit(errno);
    } else if (fpid < 0) {//如果还在执行，就报错退出        
        perror("fork");
        exit(errno);
    }
    //创建消息队列
    msgid = msgget((key_t) MESSAGE_ID, 0666 | IPC_CREAT);
    if (-1 == msgid) {
        perror("create message queue");
        exit(errno);
    }
    //创建FIFO并打开
    //命名管道是通过 FIFO 文件来进行进程间通讯的方式，在文件系统中以文件名的形式存在。
    //后台进程通过 只写方式打开命名管道，作为生产者写入数据;
     //前台进程通过只读方事打开命名管道，作为消费者读取数据，实现了进程间的通信与同步。   
     if (0 != mkfifo(FIFO_NAME, 0777)) {
        perror("fifo");
        exit(errno);
    }

    filedsc = open(FIFO_NAME, O_RDONLY);
    if(-1==filedsc){
        perror("client open FIFO");
        exit(errno);
    }
    while (flag) {
        printf(">>hey Professor_Jia>>");
        msg_del(&data);
        //读入标准输入
        fgets(buffer, BUFFER_SIZE, stdin);
        //将输入拆分为各个字符串
        top = -1;
        command = strtok(buffer, " \n");
        while (command != NULL) {
            stack[++top] = command;
            command = strtok(NULL, " \n");
        }
        if (top < 0) {
            continue;
        }
        command = stack[0];
        //转化命令
        if (0 == strcmp(command, "dir")){
            msg_cmd(&data, "ls");}     //data是消息队列存一条消息用的;存“ls”     
        else if (0 == strcmp(command, "rename")) {
            msg_cmd(&data, "mv");} 
        else if (0 == strcmp(command, "move")) {
            msg_cmd(&data, "mv");}
        else if (0 == strcmp(command, "del")) {
            msg_cmd(&data, "rm");}
        else if (0 == strcmp(command, "touch")) {
            msg_cmd(&data, "touch");} 
        else if (0 == strcmp(command, "copy")) 
            {msg_cmd(&data, "cp");
        } else if (0 == strcmp(command, "md")) {
            msg_cmd(&data, "mkdir");}
        else if (0 == strcmp(command, "cd")) {
            if (top == 0) {     //即如果cd不带参数，就是没有跳转到指定的目录，就不用执行chdir，直接当成pwd去执行              
                msg_cmd(&data, "pwd");
            } else {
                msg_cmd(&data, "cd");
            }
        } else if (0 == strcmp(command, "exit")) {
            flag = 0;
            msg_cmd(&data, "exit");} 
        else { continue;}
        if (flag) {
            //合成指令 
            for (i = 1; i <= top; i++) {
                msg_add(&data, stack[i]);//msg_cmd是将Linux指令的指令名称放入&data消息体，在这里是为它后面接上参数放入同一个data    
            }
            msg_end(&data);
        }
        //发送消息
        if (-1 == msgsnd(msgid, (void *) &data, BUFFER_SIZE, 0)) {
            perror("send message");
            exit(errno);
        }
        if (flag) {
        //打开并阻塞读取FIFO数据
            read(filedsc, buffer, BUFFER_SIZE);
            printf("%s", buffer);
        }
    }
    //删除消息队列
    if (msgctl(msgid, IPC_RMID, 0) == -1) {
        perror("delete msg");
        exit(errno);
    }
    //关闭，删除FIFO文件
    if(-1==close(filedsc)){
        perror("client close FIFO");
        exit(errno);
    }
    unlink(FIFO_NAME);
    //等待后台进程退出
    wait(&status);
    //用于显示前后台退出的顺序
    printf("client exited\n");
}
