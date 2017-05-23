//
//  Hello World �ͻ���
//  ����REQ�׽����� tcp://localhost:5555
//  ����Hello������ˣ�������World
//
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
 
int main (void)
{
    void *context = zmq_init (1);
 
    //  ����������˵��׽���
    printf ("����������hello world�����...\n");
    void *requester = zmq_socket (context, ZMQ_REQ);
    zmq_connect (requester, "tcp://10.10.22.138:5555");
 
    int request_nbr;
    for (request_nbr = 0; request_nbr != 10; request_nbr++) {
        zmq_msg_t request;
        zmq_msg_init_size (&request, 5);
        memcpy (zmq_msg_data (&request), "Hello", 5);
        printf ("���ڷ��� Hello %d...\n", request_nbr);
        zmq_send (requester, &request,sizeof(request), 0);
        zmq_msg_close (&request);
 
        zmq_msg_t reply;
        zmq_msg_init (&reply);
        zmq_recv (requester, &reply,sizeof(reply), 0);
        printf ("���յ� World %d\n", request_nbr);
        zmq_msg_close (&reply);
    }
    zmq_close (requester);
    zmq_term (context);
    return 0;
}
