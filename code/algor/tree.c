#include <stdio.h>
typedef struct BinaryTreeNode_
{
    int val;
    struct BinaryTreeNode_ * left;
    struct BinaryTreeNode_ * right;
}BinaryTreeNode;

//求二叉树中的节点个数
int GetNodeCnt(BinaryTreeNode *root)
{
    if(root == NULL)
        return 0;
    return GetNodeCnt(root->left)+GetNodeCnt(root->right)+1;
}
//求二叉树的深度
int GetDepth(BinaryTreeNode *root)
{
    if(root ==NULL)
        return 0;
    int leftCnt = GetDepth(root->left);
    int rightCnt = GetDepth(root->right);
    return leftCnt > rightCnt ? (leftCnt+1):(rightCnt+1);
}
//前序遍历，中序遍历，后序遍历
void PreOrderTraverse(BinaryTreeNode *root)
{
    if(root == NULL)
        return ;
    printf("val:%d\n",root->val);
    PreOrderTraverse(root->left);
    PreOrderTraverse(root->right);
}
//求二叉树第K层的节点个数
int GetNodeNumKthLevel(BinaryTreeNode * pRoot, int k)
{
    if(pRoot == NULL ||k<1)
        return 0;
    if(k == 1)
        return 1;
    int left = GetNodeNumKthLevel(pRoot->left,k -1);
    int right = GetNodeNumKthLevel(pRoot->right,k -1);
    return left + right ;
}
//求二叉树中叶子节点的个数
int GetLeafNodeCnt(BinaryTreeNode * pRoot)
{
    if(pRoot == NULL )
        return 0;
    if(pRoot->left == NULL && pRoot->right == NULL)
        return 1;
    int left = GetLeafNodeCnt(pRoot->left);
    int right = GetLeafNodeCnt(pRoot->right);
    return left + right ;
}
//判断两棵二叉树是否结构相同
int StructureCmp(BinaryTreeNode * pRoot1, BinaryTreeNode * pRoot2)  
{

    if(pRoot1 == NULL && pRoot2 == NULL) // 都为空，返回真  
        return 1;  
    else if(pRoot1 == NULL || pRoot2 == NULL) // 有一个为空，一个不为空，返回假  
        return 0;  
    int  resultLeft = StructureCmp(pRoot1->left, pRoot2->left); // 比较对应左子树   
    int  resultRight = StructureCmp(pRoot1->right, pRoot2->right); // 比较对应右子树  
    return (resultLeft && resultRight);  
}   
void Link(BinaryTreeNode* nodes, int parent, int left, int right)
{

    if (left != -1)
        nodes[parent].left = &nodes[left]; 

    if (right != -1)
        nodes[parent].right = &nodes[right];
}
int main()
{
     BinaryTreeNode test1[9] = {0 };
     Link(test1, 0, 1, 2);
     Link(test1, 1, 3, 4);
     Link(test1, 2, 5, 6);
     Link(test1, 3, 7, -1);
     Link(test1, 5, -1, 8);

    int ret = 0;
    ret = GetLeafNodeCnt(&test1[0]);
    printf("LeafNodeCnt=%d\n",ret);
    ret = GetNodeNumKthLevel(&test1[0],2);
    printf("GetNodeNumKthLevel=%d\n",ret);
}
