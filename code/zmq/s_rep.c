//
//  Hello World 服务端
//  绑定一个REP套接字至tcp://*:5555
//  从客户端接收Hello，并应答World
//
#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define FILESIZE 10240
void readjson(char *filename,char* dst)
{
    FILE *pFile=fopen(filename,"r"); //获取文件的指针

    fseek(pFile,0,SEEK_END); //把指针移动到文件的结尾 ，获取文件长度
    int len=ftell(pFile); //获取文件长度
    if(len>FILESIZE)
    {
        printf("len=%d,FILESIZE=%d\n",len,FILESIZE);
        exit(-1);

    }
    rewind(pFile); //把指针移动到文件开头 因为我们一开始把指针移动到结尾，如果不移动回来 会出错
    fread(dst,1,len,pFile); //读文件
    dst[len]=0; //把读到的文件最后一位 写为0 要不然系统会一直寻找到0后才结束

    fclose(pFile); // 关闭文件    
}
 
int main (int argc,char ** argv)
{
    void *context = zmq_init (1);
 
    //  与客户端通信的套接字
    void *responder = zmq_socket (context, ZMQ_REP);
    zmq_bind (responder, "tcp://*:5555");

    char *jsonSend;
    char *jsonRecv;
    jsonSend  = (char*)malloc(sizeof(char)*FILESIZE); 
    jsonRecv  = (char*)malloc(sizeof(char)*FILESIZE); 
    memset(jsonSend,0,sizeof(char)*FILESIZE); 
    memset(jsonRecv,0,sizeof(char)*FILESIZE); 
    zmq_msg_t request;
    zmq_msg_t reply;
    //while (1) 
    {
        //  等待客户端请求
        printf(".........................................\n");
        zmq_msg_init (&request);
        zmq_msg_recv (&request,responder, 0);
        printf ("收到 %d:%s\n",zmq_msg_size(&request),zmq_msg_data(&request));
        zmq_msg_close (&request);

        //  做些“处理”
        sleep (1);

        //  返回应答
        strcpy(jsonSend,"hello");
        zmq_msg_init_size (&reply, FILESIZE);
        memcpy (zmq_msg_data (&reply), jsonSend, strlen(jsonSend));
        printf ("send %d:%s\n",strlen(jsonSend),zmq_msg_data (&reply));
        zmq_msg_send (&reply,responder,0);
        zmq_msg_close (&reply);



        printf(".........................................\n");
        zmq_msg_init (&request);
        zmq_msg_recv (&request,responder, 0);
        printf ("收到 %d:%s\n",zmq_msg_size(&request),zmq_msg_data(&request));
        zmq_msg_close (&request);
 
        //  做些“处理”
        sleep (1);
 
        readjson(argv[1],jsonSend);
        zmq_msg_init_size (&reply, FILESIZE);
        memcpy (zmq_msg_data (&reply), jsonSend, strlen(jsonSend));
        printf ("send %d:%s\n",strlen(jsonSend),zmq_msg_data (&reply));
        zmq_msg_send (&reply,responder,0);
        zmq_msg_close (&reply);


    }
    //  程序不会运行到这里，以下只是演示我们应该如何结束
    zmq_close (responder);
    zmq_term (context);
    return 0;
}
