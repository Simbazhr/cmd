#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "msg.h"
#include <errno.h>
#include <sys/msg.h>


void main() {
    int running = 1;//flag为0退出接收消息遇到exit
    int msg_type = 0;
    FILE *pipe;
    char buffer[BUFFER_SIZE];
    char temp[BUFFER_SIZE], *str;
    struct msg_wrapper data;
    int filedsc, msgid;//filedescription,get FIFO returns

    //创建消息队列
    msgid = msgget((key_t) MESSAGE_ID, 0666 | IPC_CREAT);
    if (-1 == msgid) {
        perror("create message queue");
        exit(errno);
    }
    //阻塞写方式打开FIFO文件
    filedsc = open(FIFO_NAME, O_WRONLY);
    if(-1==filedsc){
        perror("server open FIFO");
        exit(errno);
    }
    while (running) {
        //接受消息
        msg_del(&data);
        msgrcv(msgid, (void *) &data, BUFFER_SIZE, msg_type, 1);
//exit和cd不能使用popen,因为 popen 相当于创建了一个子进程
//子进程中执行 cd 命令，对父进程的环境变量没有影响
//因此应当检查是否为 cd 指令，并且使用 chdir 函数来改变当前目录到str  
        if (0 == strcmp(data.text, "exit")) {
            running = 0;
            continue;
        } else if (0 == strncmp(data.text, "cd", 2)) {////cd包含两步，一是先用chdir(str)改当前目录，再用pwd显示当前所处目录          
            strtok(data.text, " ");
            str = strtok(NULL, "\n ");//get the 2nd args for the string(address wanna go)
            printf("%s\n", str);
            if (chdir(str) == -1) {
                perror("change directory");
            }
            strcpy(data.text, "pwd");
        }
        
        //执行命令，读取结果，写入FIFO
        pipe = popen(data.text, "r");//popen执行shell命令data.text，返回管道      
        memset(buffer, 0, BUFFER_SIZE);//buffer指向的空间全换成0     
        while (fgets(temp, BUFFER_SIZE, pipe) != NULL) {
//fgets逐行从【文件指针】管道pipe读取shell的执行结果到temp中，再拼成buffer     
            strcat(buffer, temp);
        }
        pclose(pipe);
        write(filedsc, buffer, BUFFER_SIZE);////把执行结果buffer作为消息发回去  
    }
    //关闭FIFO文件
    if (-1 == close(filedsc)) {
        perror("server fifo close");
        exit(errno);
    }
    //用于显示前后台退出的顺序
    printf("server exited\n");
}
