1. ���� ѡ���� 
ѭ���������⣺ѭ�������и�����Ӧ�þ��ǽ�Լɪ������(Josephus)�������õĵ�����Ŀ��"����ѡ����"��nֻ����Ҫѡ������ѡ�ٷ������£����к��Ӱ� 1��2 ������ n ��Ų�����˳��Χ��һȦ���ӵ� k ����������1��ʼ����������mʱ���ú��Ӿ�����Ȧ�⣬��һֻ�����ٴ���1��ʼ���������ѭ����ֱ��Ȧ��ʣ��һֻ����ʱ����ֻ���Ӿ��Ǵ�����
��������Ϊ��������n����ʼ�����ĺ��ӱ��k����������m���������Ϊ���ӳ������кͺ��Ӵ����ı�š�
�������������һ����n���ڵ㣬û��ͷ����ѭ������ȷ����һ�������˵�λ�á����ϴ�������ɾ���ڵ㣬ֱ������Ϊ�ա�
#include "stdafx.h"  
#include <stdio.h>  
#include <stdlib.h>  
  
typedef struct Node  
{  
    int data;  
    struct Node *next;  
}LNode;  
  
//����n,��ʼ�����ı��k,��������m  
void Josephus(int n,int k,int m)  
{  
    int i;  
    int totle = n;  
    LNode *p,*prevP,*head,*s;//pΪ��ǰ�ڵ㣬headΪͷ�ڵ�  
    head=(LNode *)malloc(sizeof(LNode));  
    head->data = 0;  
    head->next = head;  
    p = head;  
    p->data = 1;  
    p->next = p;  
    for(i=2;i<=n;i++)  
    {  
        LNode * temp = (LNode *)malloc(sizeof(LNode));  
        temp->data = i;  
        temp->next = p->next;  
        p->next = temp;  
        p=p->next;  
    }  
  
    p = head;  
    for(i=1;i<k;i++)  
        p=p->next;  
    prevP = head;  
    while(totle!=1)  
    {  
        for(i=1;i<m;i++)  
            p=p->next;  
        printf("%d ����/n",p->data);  
        while(prevP->next!=p)  
            prevP=prevP->next;  
        prevP->next = p->next;  
        s = p;  
        p = p->next;  
        free(s);  
        totle--;  
    }  
    printf("���ʣ�µĽڵ�Ϊ%d",p->data);  
}  
int main()  
{  
    Josephus(13,4,1);  
    return 0;   
}  