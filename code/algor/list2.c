1. 猴子 选大王 
循环链表问题：循环链表有个典型应用就是解约瑟夫问题(Josephus)，这里用的典型题目是"猴子选大王"。n只猴子要选大王，选举方法如下：所有猴子按 1，2 ……… n 编号并按照顺序围成一圈，从第 k 个猴子起，由1开始报数，报到m时，该猴子就跳出圈外，下一只猴子再次由1开始报数，如此循环，直到圈内剩下一只猴子时，这只猴子就是大王。
输入数据为猴子总数n，起始报数的猴子编号k，出局数字m。输出数据为猴子初队序列和猴子大王的编号。
解决方法：建立一个有n个节点，没有头结点的循环链表，确定第一个报数人的位置。不断从链表中删除节点，直到链表为空。
#include "stdafx.h"  
#include <stdio.h>  
#include <stdlib.h>  
  
typedef struct Node  
{  
    int data;  
    struct Node *next;  
}LNode;  
  
//总数n,起始报数的编号k,出局数字m  
void Josephus(int n,int k,int m)  
{  
    int i;  
    int totle = n;  
    LNode *p,*prevP,*head,*s;//p为当前节点，head为头节点  
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
        printf("%d 出局/n",p->data);  
        while(prevP->next!=p)  
            prevP=prevP->next;  
        prevP->next = p->next;  
        s = p;  
        p = p->next;  
        free(s);  
        totle--;  
    }  
    printf("最后剩下的节点为%d",p->data);  
}  
int main()  
{  
    Josephus(13,4,1);  
    return 0;   
}  