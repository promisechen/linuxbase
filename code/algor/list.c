//https://www.cnblogs.com/qingjiaowoxiaoxioashou/p/6416649.html
//3 4 5 7 13 14 15
#include <stdio.h>
//#define NULL 0
typedef struct Lnode
{
    int val;
    struct Lnode *next;
} LNode;

LNode * mergeList(LNode *l1,LNode*l2)
{
    LNode *node1 = l1;
    LNode *node2 = l2;
    LNode * curNode = NULL;
    LNode *l3 = NULL;
    //判读入参
    if(node1->val > node2->val)
    {
        l3 = node2;
        node2 = node2->next;
    }
    else 
    {
        l3 = node1;
        curNode = node1;
        node1 = node1->next;
    }
    while(node1 != NULL && node2 != NULL)
    {
        if(node1->val > node2->val)
        {
            curNode->next = node2;
            curNode = node2;
            node2=node2->next;
        }
        else
        {
            curNode->next = node1;
            curNode = node1;
            node1=node1->next;

        }

    }
    if(node1 == NULL)
        curNode->next = node2;
    else if(node2 == NULL)
        curNode->next = node1;
    //链接
    return l3;
}

LNode * mergeList2(LNode *l1,LNode*l2)
{
    LNode *node1 = l1;
    LNode *node2 = l2;
    LNode *node2x = NULL;
    LNode *node1x = NULL;
    LNode *l3 = NULL;
    //判读入参
    if(node1->val > node2->val)
    {
        l3 = node2;
        node2 = node2->next;
        l3->next = NULL;
    }
    else 
    {
        l3 = node1;
        node1 = node1->next;
        l3->next = NULL;
    }
    while(node1 != NULL && node2 != NULL)
    {
        if(node1->val > node2->val)
        {
            node2x = node2;
            node2=node2->next;
            node2x->next = l3;
            l3 = node2x;
        }
        else
        {
            node1x = node1;
            node1=node1->next;
            node1x->next = l3;
            l3 = node1x;
        }
    }
    if(node1 != NULL)
    {
        while(node1 != NULL)
        {
            node1x = node1;
            node1=node1->next;
            node1x->next = l3;
            l3 = node1x;
        }
    }
    else if(node2 != NULL)
    {
        while(node2 != NULL)
        {
            node2x = node2;
            node2=node2->next;
            node2x->next = l3;
            l3 = node2x;
        }
    }
    return l3;
}
void printList(LNode *l,char * user)
{
    LNode *node = l;
    printf("node*****%s\n",user);
    while(node!=NULL)
    {
        printf("%d ",node->val);
        node = node->next;
    }
    printf("\n");
}
//逆序
LNode* nixu(LNode *l)
{
    LNode *node = l;
    LNode *newL = NULL;
    LNode *tmpNode;
    if(l==NULL || l->next ==NULL)
    {
        return l;
    }

    while(node!=NULL)
    {
        tmpNode = node;
        node = node->next;
        tmpNode->next = newL;
        newL=tmpNode;
    }
    return newL;
}
//查找单链表中的倒数第K个结点（k > 0）
LNode * Nodek(LNode *l,int k)
{
    LNode *node = l;
    LNode *nodek;
    int num = 0;
    while(node != NULL && num < k-1 )
    {
        num++;
        node = node->next;
    }
    if(node == NULL && num <= k-1)
    {
        return NULL;

    }

    nodek = l;
    node  = node->next;
    while(node)
    {
        nodek = nodek->next; 
        node = node->next;
    }
    return nodek;

}
//查找单链表的中间结点
LNode * MidNode(LNode *l)
{
    LNode *nodeFast = l;
    LNode *nodeSlow = l;
    if(l==NULL)
        return NULL;
    while(nodeFast != NULL && nodeFast->next != NULL)
    {
        nodeSlow = nodeSlow->next;
        nodeFast = nodeFast->next->next;
    }

    return nodeSlow;
}
//查找单链表的中间结点
int MidNode2(LNode *l,LNode **node1,LNode **node2)
{
    LNode *nodeFast = l;
    LNode *nodeSlow = l;
    LNode *nodeSlowOld = l;
    if(l==NULL)
        return -1;

    while(nodeFast != NULL && nodeFast->next != NULL)
    {
        nodeSlowOld = nodeSlow;
        nodeSlow = nodeSlow->next;
        nodeFast = nodeFast->next->next;
    }
    if(nodeFast == NULL)
    {
        *node1 = nodeSlowOld;
        *node2 = nodeSlow;
    }
    else
    {
        *node1 = nodeSlow;
        *node2 = NULL;
    }
    return 0;
}
int PrintNOdeNizhuan(LNode *l)
{
    if(l==NULL)
    {
        return 0;
    }
    PrintNOdeNizhuan(l->next);
    printf("%d ",l->val);
    return 0;
}
//判断一个单链表中是否有环
int IsCircle(LNode *l)
{
    LNode *nodeSlow = l;
    LNode *nodeFast = l;
    while(nodeFast != NULL && nodeFast->next != NULL)
    {
        nodeFast = nodeFast->next->next;
        nodeSlow = nodeSlow->next;
        if(nodeFast == nodeSlow)
        {
            return 1;
        }

    }
    return 0;
}
//已知一个单链表中存在环，求进入环中的第一个节点
//假设相遇时慢指针走了s，则快指针走了2s，两者的差s必然为环长的n倍，即s=nl（l为环长），即如果一个指针S1从链表头出发，另一个指针S2从相遇点出发（两者速度相同），S1走到S2出发点时两者一定会相遇（两者都走了nl），而他们的速度是一样的，说明他们在环入口处其实就相遇了，而且是第一次相遇（就在一起了~~~~），这样我们就能定位到环入口了

LNode* IsCircle2(LNode *l)
{
    LNode *nodeSlow = l;
    LNode *nodeFast = l;
    while(nodeFast != NULL && nodeFast->next != NULL)
    {
        nodeFast = nodeFast->next->next;
        nodeSlow = nodeSlow->next;
        if(nodeFast == nodeSlow)
        {
            break;
        }
    }
    nodeSlow = l;
    while(nodeSlow != nodeFast)
    {
        nodeSlow = nodeSlow->next;
        nodeFast = nodeFast->next;
    }
    return nodeSlow;
}
//判断两个单链表是否相交
int IsIntersected(LNode *l1,LNode *l2)
{
    LNode *node1 = l1;
    LNode *node2 = l2;
    if(l1== NULL)
        return 0 ;

    if(l2== NULL)
        return 0;

    while(node1->next)
    {
        node1= node1->next;
    }
    while(node2->next)
    {
        node2= node2->next;
    }
    if(node1 == node2)
    {
        return 1;
    }
    return 0;
}
//相交的第一个节点
LNode * Intersected(LNode *l1,LNode *l2)
{
    LNode *node1 = l1;
    LNode *node2 = l2;
    int len1,len2,len;
    int i  = 0;
    if(l1== NULL)
        return NULL;

    if(l2== NULL)
        return NULL;
    len1 = 1;
    len2 = 1;
    while(node1->next)
    {
        len1++;
        node1= node1->next;
    }
    while(node2->next)
    {
        len2++;
        node2= node2->next;
    }
    
    if(node1 == node2)
    {
        node1 = l1;
        node2 = l2;

        if(len1>len2)      
        {
            for(i =0;i<len1-len2;i++)
            {
                node1=node1->next;
            } 
        }else
        {
            for(i =0;i<len2-len1;i++)
            {
                node2=node2->next;
            } 
            
        }
        printf("%d %d\n",node1->val,node2->val);
        while(node1)
        {
            if(node1 == node2)
            {
                return node1;
            }
            node1 = node1->next;
            node2 = node2->next;
        }
    }
    return NULL;
}
//给出一单链表头指针pHead和一节点指针pToBeDeleted，O(1)时间复杂度删除节点pToBeDeleted
void delNode(LNode *l,LNode *node)
{
    LNode * nodeTmp = l;
   if(l==NULL)
       return ;

   if(l->next == NULL)
   {
       if(l == node)
       {
           l = NULL;
           //xx??
            return;
        }
       else 
       {
           return ;
       }
   }
   if(node->next==NULL)
   {
       nodeTmp = l;
       while(nodeTmp->next->next)
       {
           nodeTmp = nodeTmp->next; 
       }
       nodeTmp->next=NULL;
       //del
   }
   else
   {
        nodeTmp = node->next;
        node->val = nodeTmp->val;
        node->next = nodeTmp->next;
   }
   return;
}
int main()
{
    LNode n1,n2,n3,n4,n5;
    LNode na,nb,nc,nd,ne;
    LNode *node1,*node2;
    n1.val = 1;
    n1.next = &n2;

    n2.val = 3;
    n2.next = &n3;

    n3.val = 8;
    n3.next = &n4;

    n4.val = 10;
    n4.next = &n5;

    n5.val = 18;
    n5.next = NULL;


    na.val = 5;
    na.next = &nb;

    nb.val = 6;
    nb.next = &nc;


    nc.val = 10;
    nc.next = &nd;

    nd.val = 11;
    nd.next = &ne;

    ne.val = 12;
    ne.next = NULL;
    printList(&n1,"n1"); 
    printList(&na,"na"); 
    LNode *l3 ;
#if 0
    l3 = mergeList2(&n1,&na);  printList(l3,"l3");
    l3 = nixu(&n1);printList(l3,"l3");
#endif
#if 0
    l3 = Nodek(&n1,3);printf("nodek=%d\n",l3->val);

    l3 = Nodek(&n1,4);printf("nodek=%d\n",l3->val);

    l3 = Nodek(&n1,5);
    if(l3) printf("nodek=%d\n",l3->val);

    l3 = Nodek(&n1,2);
    printf("nodek=%d\n",l3->val);
#endif
#if 0
    l3 = MidNode(&n1);printf("n1 mid=%d\n",l3->val);
    l3 = MidNode(&na);printf("na mid=%d\n",l3->val);
    MidNode2(&n1,&node1,&node2);
    if(node1!=NULL)
        printf("n1 mid=%d ",node1->val);
    if(node2!=NULL)
        printf(" %d\n",node2->val);
    MidNode2(&na,&node1,&node2);
    if(node1!=NULL)
        printf("na mid=%d",node1->val);
    if(node2!=NULL)
        printf(" %d\n",node2->val);
#endif
#if 0
    printf("\nnizhuan n1:");
    PrintNOdeNizhuan(&n1);
    printf("\n");
    printf("nizhuan n2:");
    PrintNOdeNizhuan(&na);
    printf("\n");
#endif
    int ret = 0;
#if 1
    n5.next = &n3;
    //    printList(&n1,"n1"); 
    ret = IsCircle(&n1);
    if(ret != 0)
        printf("circle\n");
    else
        printf("no circle\n");

    l3 = IsCircle2(&n1);
    if(l3 != NULL)
        printf("circle=%d\n",l3->val);
#endif
#if 0
    ret = IsIntersected(&n1,&na); 
    if(ret != 0)
        printf("xiangjiao\n");
    else
        printf("no xiangjiao\n");

    n3.next=&nb;
    printList(&n1,"n1"); 
    printList(&na,"na"); 
    ret = IsIntersected(&n1,&na); 
    if(ret != 0)
        printf("xiangjiao\n");
    else
        printf("no xiangjiao\n");

    l3=Intersected(&n1,&na);
    if(l3!=NULL)
        printf("le =%d\n",l3->val);

#endif 
#if 0
    delNode(&n1,&n3);
    printList(&n1,"n1"); 
    delNode(&n1,&n5);
    printList(&n1,"n1"); 
#endif
}
