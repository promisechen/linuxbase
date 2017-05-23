
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define FILESIZE 10240
void readjson(char *filename,char* dst)
{
    FILE *pFile=fopen(filename,"r"); //��ȡ�ļ���ָ��

    fseek(pFile,0,SEEK_END); //��ָ���ƶ����ļ��Ľ�β ����ȡ�ļ�����
    int len=ftell(pFile); //��ȡ�ļ�����
    if(len>FILESIZE)
    {
        printf("len=%d,FILESIZE=%d\n",len,FILESIZE);
        exit(-1);

    }
    rewind(pFile); //��ָ���ƶ����ļ���ͷ ��Ϊ����һ��ʼ��ָ���ƶ�����β��������ƶ����� �����
    fread(dst,1,len,pFile); //���ļ�
    dst[len]=0; //�Ѷ������ļ����һλ дΪ0 Ҫ��Ȼϵͳ��һֱѰ�ҵ�0��Ž���

    fclose(pFile); // �ر��ļ�    
}
int main (int argc,char ** argv)
{
    void *context = zmq_init (1);


    printf ("..\n");
    void *requester = zmq_socket (context, ZMQ_REQ);
    //zmq_connect (requester, "tcp://127.0.0.1:5555");
    zmq_connect (requester, "tcp://10.10.22.138:5555");

    int request_nbr;
    //for (request_nbr = 0; request_nbr != 10; request_nbr++) 
    {
        zmq_msg_t request;
        zmq_msg_t reply;
        // json 
        char *jsonSend;
        char *jsonRecv;
        jsonSend  = (char*)malloc(sizeof(char)*FILESIZE); 
        jsonRecv  = (char*)malloc(sizeof(char)*FILESIZE); 
        memset(jsonSend,0,sizeof(char)*FILESIZE); 
        memset(jsonRecv,0,sizeof(char)*FILESIZE); 
        // hello
        strcpy(jsonSend,"hello");
        zmq_msg_init_size (&request,strlen(jsonSend)+1);
        memcpy (zmq_msg_data (&request), jsonSend, strlen(jsonSend)+1);
        printf ("send %d:%s\n",strlen(jsonSend),zmq_msg_data (&request));
        zmq_msg_send (&request,requester, 0);
        zmq_msg_close (&request);

        zmq_msg_init (&reply);
        zmq_msg_recv (&reply,requester, 0);
        printf ("recv %d:%s\n",zmq_msg_size(&reply),zmq_msg_data (&reply));
        zmq_msg_close (&reply);


        printf(".........................................\n");
        readjson(argv[1],jsonSend);
        zmq_msg_init_size (&request,strlen(jsonSend)+1);
        memcpy (zmq_msg_data (&request), jsonSend, strlen(jsonSend)+1);
        printf ("send %d:%s\n",strlen(jsonSend)+1,zmq_msg_data (&request));
        zmq_msg_send (&request,requester, 0);
        zmq_msg_close (&request);

        zmq_msg_init (&reply);
        zmq_msg_recv (&reply,requester, 0);
        printf ("recv %d:%s\n",zmq_msg_size(&reply),zmq_msg_data (&reply));
        zmq_msg_close (&reply);
        printf(".........................................\n");

    }
    zmq_close (requester);
    zmq_term (context);
    return 0;
}
