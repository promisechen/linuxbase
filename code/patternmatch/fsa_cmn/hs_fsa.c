/**
 * @file 
 * @brief fsa implementation.
 * @author zzq
 * @version 1.0
 * @date 2015-09-28
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hs_fsa.h"


#define FSA_DEBUG


/**
 * @brief 模式链表项.
 */
typedef struct fsa_ptnlist
{
    fsa_pattern_t* ptn;
    uint32_t uid;
    struct fsa_ptnlist*  next;  /**< 模式表中的下一节点 */
} fsa_ptnlist_t;

typedef struct fsa_match_list
{
    uint32_t uid;
    struct fsa_match_list* next;
} fsa_match_list_t;



/**
 * @brief 状态转换链表项. 
 *  
 * @warning 这里浪费3字节.  
 */
typedef struct fsa_translist
{
    uint8_t  input;          /**< 输入 */
    uint8_t  pad[3];
    state_t   next_state;      /**< 下一状态 */
    struct fsa_translist*  next;  /**< 状态转换表下一节点 */
} fsa_translist_t;


/**
 * @brief 状态转换. 
 *  
 * @warning 这里浪费3字节. 
 */
typedef struct fsa_trans
{
    uint8_t  Input;        /**< 输入 */
    uint8_t  Pad[3];
    state_t   nextState;    /**< 下一状态 */
} fsa_trans_t;


/**
 * @brief 链表式FSA索引.
 */
typedef struct fsa_list_index
{
    state_t         State;      /**< 当前状态 */
    state_t         FailState;  /**< 失配时应回退到的下一状态，仅对NFA有效 */
    uint32_t        TransCnt;   /**< 状态转换表节点数 */
    uint32_t        MatchCnt;   /**< 匹配表节点数 */
    fsa_trans_t*   trans_addr;
    fsa_pattern_t** match_addr;
} fsa_list_index_t;


/**
 * @brief 全矩阵式DFA索引.
 */
typedef struct fsa_full_index
{
    state_t         State;      /**< 当前状态 */
    uint32_t        MatchCnt;   /**< 匹配表节点数 */
    fsa_pattern_t** match_addr;
    state_t         Trans[FSA_CONT_SIZE];   /**< 当前状态转换表行 */
} fsa_full_index_t;



/**
 * @brief 限带矩阵式DFA索引.
 */
typedef struct fsa_banded_index
{
    state_t         State;      /**< 当前状态 */
    uint32_t        MatchCnt;   /**< 匹配表节点数 */
    uint8_t        Len;        /**< 列数 */
    uint8_t        First;      /**< 起始非0列 */
    uint8_t        Pad[6];
    fsa_pattern_t** match_addr;
    state_t*        Trans;      /**< 当前状态转换表行 */
} fsa_banded_index_t;


/**
 * @brief 用于编译状态机的辅助结构. 
 * @note 仅用于在堆上编译构建状态机；完成后，整个状态机将被转换到Sniper内存块中，此结构将被销毁. 
 */
typedef struct fsa_compile_helper
{
    uint32_t           PtnCnt;         /**< 模式数 */
    uint32_t           MaxStateCnt;    /**< 最大（预测）状态数 */
    uint32_t           ActualStateCnt; /**< 实际状态数 */
    uint32_t           TransCnt;       /**< 状态转换数 */
    uint64_t           HeapMemSize;    /**< 占用堆内存大小 */
    uint8_t*           Mem;            /**< 用户给出的内存块起始地址 */
    uint64_t           MemOffset;      /**< 内存块当前偏移 */
    uint64_t           MemSize;        /**< 内存块总大小 */
    fsa_ptnlist_t*  ptn_list;
    fsa_match_list_t**  MatchList;
    //fsa_ptnlist_t**  match_list;    
    fsa_translist_t**   TransList;      /**< 状态转换表 */
    state_t*            FailTable;      /**< 失配表（仅用于NFA） */
} fsa_compile_helper_t;


/** FSA链表遍历 */
#define FSALIST_FOR_EACH(p, head) \
  for(p = (head); p != NULL; p = p->next)

/** FSA链表安全遍历 */
#define FSALIST_FOR_EACH_SAFE(p, head, tmp) \
   for(p = (head), tmp = (p ? p->next : NULL); p != NULL; \
       p = tmp, tmp = (p ? p->next : NULL))

/** 模式串指针数组类型， 主要是为了作为函数参数时不出现三级指针 */
typedef fsa_pattern_t** fsa_pattern_ptr_array_t;


/** 
 * @brief 0x00-0xff 字符大写转换表.
 */ 
static const uint8_t  CaseTable[FSA_CONT_SIZE] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
    0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

#define CASETAB(x) CaseTable[(x) & 0xff]


////////////////////////////////////////////////////////////////////////////////////

/** 一个简单的队列实现 */

typedef struct _QNODE
{
    state_t state;
    struct _QNODE* next;
} QNODE;

typedef struct _QUEUE
{
    QNODE *head, *tail;
    int count;
} QUEUE;

static inline void Q_Init(QUEUE* q)
{
    q->head = q->tail = NULL;
    q->count = 0;
}

static inline state_t Q_Find(QUEUE* q, state_t state)
{
    QNODE* n = q->head;
    while (n)
    {
        if (state == n->state)
            return 1;
        n = n->next;
    }
    return 0;
}

static inline void Q_Push(QUEUE* q, state_t state)
{
    QNODE* n;

    if (Q_Find(q, state))
        return;

    if (NULL == q->head)
    {
        n = q->tail = q->head = (QNODE*) malloc(sizeof(QNODE));
        n->state = state;
        n->next = NULL;
    }
    else
    {
        n = (QNODE*) malloc(sizeof(QNODE));
        n->state = state;
        n->next = NULL;
        q->tail->next = n;
        q->tail = n;
    }
    q->count++;
}

static inline state_t Q_Pop(QUEUE* q)
{
    state_t state = 0;
    QNODE* n;

    if (q->head != NULL)
    {
        n = q->head;
        state = n->state;
        q->head = q->head->next;
        q->count--;

        if (NULL == q->head)
        {
            q->tail = NULL;
            q->count = 0;
        }
        free(n);
    }
    return state;
}

static inline int Q_Count(QUEUE* q)
{
    return q->count;
}

static void Q_free(QUEUE* q)
{
    while (Q_Count(q))
    {
        Q_Pop(q);
    }
}

////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief 在状态转换表中求得下一状态.
 * @note 仅用于在堆上编译状态机. 
 * 
 * @param helper [IN] 状态机编译辅助结构.
 * @param from [IN] 当前状态.
 * @param in [IN] 输入条件.
 * 
 * @return state_t 下一状态.
 */
static state_t fsa_helper_get_next(fsa_compile_helper_t* helper, state_t from, uint8_t in)
{
    fsa_translist_t* t;

    t = helper->TransList[from];

    while (t)
    {
        if (t->input == in)
            return t->next_state;
        t = t->next;
    }

    if (0 == from)
        return 0;

    return FSA_FAIL_STATE;
}


/**
 * @brief 将新的状态转换条目加入状态转换表. 
 * @note  仅用于在堆上编译状态机；此链表是一个前置链表，新节点做为表头.
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 * @param from [IN] 前一状态.
 * @param in [IN] 输入条件.
 * @param to [IN] 下一状态.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_helper_put_next(fsa_compile_helper_t* helper, 
        state_t from, uint8_t in, state_t to)
{
    fsa_translist_t *t, *t_new;

    t = helper->TransList[from];
    while (t)
    {
        if (t->input == in)
        {
            t->next_state = to;
            return FSA_ERR_OK;
        }
        t = t->next;
    }

    t_new = (fsa_translist_t*) malloc(sizeof(fsa_translist_t));
    if (NULL == t_new)
        return FSA_ERR_BAD_ALLOC;
    helper->HeapMemSize += sizeof(fsa_translist_t);

    t_new->input = in;
    t_new->next_state = to;
    t_new->next = helper->TransList[from];
    helper->TransList[from] = t_new;

    helper->TransCnt++;

    return FSA_ERR_OK;
}


/**
 * @brief 在状态转换表中求得下一状态. 
 * @note 用于在sniper内存中构建好的NFA_LIST和DFA_LIST状态机. 
 * 
 * @param idx [IN] 状态转换表索引.
 * @param from [IN] 当前状态.
 * @param in [IN] 输入条件.
 * 
 * @return state_t 下一状态.
 */
static state_t fsa_list_get_next(fsa_list_index_t* idx, state_t from, uint8_t in)
{
    uint32_t i;

    for (i=0; i<idx->TransCnt; ++i)
    {
        if (idx->trans_addr[i].Input == in)
            return idx->trans_addr[i].nextState;
    }

    if (0 == from)
        return 0;

    return FSA_FAIL_STATE;
}

/**
 * @brief 为发生匹配的状态添加匹配表项.
 * @note 仅用于在堆上编译状态机；匹配表是一个前置链表，新节点做为表头.
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 * @param state [IN] 发生匹配的状态.
 * @param ptn [IN] 匹配到的模式的指针， 不进行深拷贝.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_add_match(fsa_compile_helper_t* helper, 
//        state_t state, fsa_pattern_t* ptn)
            state_t state, uint32_t uid)
{
    //fsa_ptnlist_t* p;

    //p = (fsa_ptnlist_t*) malloc(sizeof(fsa_ptnlist_t));
    //if (NULL == p)
    //{   return FSA_ERR_BAD_ALLOC;   }
    //helper->HeapMemSize += sizeof(fsa_ptnlist_t);

    //p->ptn = ptn;
    //p->next = helper->match_list[state];

    //helper->match_list[state] = p;
    
    fsa_match_list_t* p;
   
    p = (fsa_match_list_t*) malloc(sizeof(fsa_match_list_t));
    helper->HeapMemSize += sizeof(fsa_match_list_t);
    p->uid = uid;
    p->next = helper->MatchList[state];

    helper->MatchList[state] = p;

    return FSA_ERR_OK;
}


/**
 * @brief 为模式串添加状态. 
 * @note 仅用于在堆上编译状态机.
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 * @param ptn [IN] 模式串指针.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t 
fsa_add_states(fsa_compile_helper_t* helper, 
               fsa_pattern_t* ptn,
               int case_sensitive,
               uint32_t uid)
{
    fsa_error_t ret = 0;
    state_t state, next;
    uint8_t  i;

    state = 0;
    //for (i=0; i<ptn->Depth; ++i)
    for (i=0; i<ptn->ptn_len; ++i)
    {
        if(case_sensitive)
            next = fsa_helper_get_next(helper, state, ptn->ptn[i]);
        else
            next = fsa_helper_get_next(helper, state, CASETAB(ptn->ptn[i]));

        if (FSA_FAIL_STATE == next || 0 == next)
            break;
        state = next;
    }

    for (; i<ptn->ptn_len; ++i)
    {
        helper->ActualStateCnt++;
        if(case_sensitive)
            ret = fsa_helper_put_next(helper, state, ptn->ptn[i], 
                                helper->ActualStateCnt);
        else
            ret = fsa_helper_put_next(helper, state, CASETAB(ptn->ptn[i]), 
                                helper->ActualStateCnt);
            if (ret != 0)
            return ret;
        state = helper->ActualStateCnt; 
    }

    ret = fsa_add_match(helper, state, uid);

    return ret;
}


/**
 * @brief 销毁状态机编译辅助结构.
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 */
static void fsa_free_helper(fsa_compile_helper_t* helper)
{
    fsa_ptnlist_t *pp, *ptmp;
    fsa_translist_t *tp, *ttmp;
    fsa_match_list_t *mp, *mtmp;
    uint32_t i;

    if (NULL == helper)
        return;

    if (helper->ptn_list != NULL)
    {
        FSALIST_FOR_EACH_SAFE(pp, helper->ptn_list, ptmp)
        {
            if(pp->ptn != NULL)
            {
                free(pp->ptn);
                pp->ptn = NULL;
            }
            free(pp);
            pp = NULL;
        }
    }


    for (i = 0; i < helper->ActualStateCnt; ++i) 
    {
        if (helper->TransList != NULL && helper->TransList[i] != NULL)
        {
            FSALIST_FOR_EACH_SAFE(tp, helper->TransList[i], ttmp)
            {
                free(tp);
                tp = NULL;
            }
        }

        if(helper->MatchList != NULL && helper->MatchList[i] != NULL)
        {
            FSALIST_FOR_EACH_SAFE(mp, helper->MatchList[i], mtmp)
            {
                free(mp);
                mp = NULL;
            }
        }
        //if (helper->match_list != NULL && helper->match_list[i] != NULL)
        //{
        //    FSALIST_FOR_EACH_SAFE(pp, helper->match_list[i], ptmp)
        //    {
        //        free(pp);
        //        pp = NULL;
        //    }
        //}
    }

    if (helper->TransList != NULL)
    {
        free(helper->TransList);
        helper->TransList = NULL;
    }

    //if (helper->match_list != NULL)
    //{
    //    free(helper->match_list);
    //    helper->match_list = NULL;
    //}
    if(helper->MatchList != NULL)
    {
        free(helper->MatchList);
        helper->MatchList = NULL;
    }

    helper->ActualStateCnt = 0;
    helper->MaxStateCnt = 0;

    if (helper->FailTable != NULL)
    {
        free(helper->FailTable);
        helper->FailTable = NULL;
    }

    free(helper);
    helper = NULL;
}


/**
 * @brief 构建NFA. 
 * @note 仅用于在堆上编译状态机. 
 * @todo 失败时内存如何释放. 
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_build_nfa(fsa_compile_helper_t* helper)
{
    QUEUE queue, *Q;
    state_t state, next, prev;
    fsa_translist_t* tran;
    //fsa_ptnlist_t *mlst, *ptn;
    fsa_match_list_t *mlst, *tmp;
    uint32_t i;

    Q = &queue;
    Q_Init(Q);

    helper->FailTable = (state_t*) malloc(helper->ActualStateCnt * sizeof(state_t));
    if (NULL == helper->FailTable)
        return FSA_ERR_BAD_ALLOC;
    helper->HeapMemSize += helper->ActualStateCnt * sizeof(state_t);

    for (i = 0; i < helper->ActualStateCnt; ++i) 
    {
        helper->FailTable[i] = FSA_FAIL_STATE;
    }

    for (i=0; i<FSA_CONT_SIZE; ++i)
    {
        state = fsa_helper_get_next(helper, 0, i);
        if (state)
        {
            helper->FailTable[state] = 0;
            Q_Push(Q, state);
        }
    }

    while (Q_Count(Q) > 0)
    {
        state = Q_Pop(Q);

        tran = helper->TransList[state];
        while (tran != NULL)
        {
            Q_Push(Q, tran->next_state);

            prev = helper->FailTable[state];
            while ((next = fsa_helper_get_next(helper, prev, tran->input)) == FSA_FAIL_STATE)
            {
                prev = helper->FailTable[prev];
            }
            helper->FailTable[tran->next_state] = next;

            /*
            for (mlst = helper->match_list[next]; 
                 mlst != NULL;
                 mlst = mlst->next)
            {
                ptn = (fsa_ptnlist_t*) malloc(sizeof(fsa_ptnlist_t));
                if (NULL == ptn)
                    return FSA_ERR_BAD_ALLOC;
                helper->HeapMemSize += sizeof(fsa_ptnlist_t);

                ptn->ptn = mlst->ptn; 
                ptn->next = helper->match_list[tran->next_state];
                helper->match_list[tran->next_state] = ptn;
            }
            */

            for(mlst = helper->MatchList[next];
                mlst != NULL;
                mlst = mlst->next)
            {
                tmp = (fsa_match_list_t*) malloc(sizeof(fsa_match_list_t));
                if(NULL == tmp)
                    return FSA_ERR_BAD_ALLOC;
                helper->HeapMemSize += sizeof(fsa_match_list_t);

                tmp->uid = mlst->uid;
                tmp->next = helper->MatchList[tran->next_state];
                helper->MatchList[tran->next_state] = tmp;
            }

            tran = tran->next;
        }
    }

    Q_free(Q);

    return FSA_ERR_OK;
}


/**
 * @brief 构建DFA. 
 * @note 仅用于在堆上编译状态机. 
 * @todo 失败时内存如何释放. 
 * 
 * @param helper [IN/OUT] 状态机编译辅助结构.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_build_dfa(fsa_compile_helper_t* helper)
{
    fsa_error_t ret;
    state_t state, next;
    QUEUE queue, *Q;
    int32_t i;
    uint32_t b;

    Q = &queue;
    Q_Init(Q);

    for (i=0; i<FSA_CONT_SIZE; ++i)
    {
        state = fsa_helper_get_next(helper, 0, i);
        if (state)
            Q_Push(Q, state);
    }

    while (Q_Count(Q) > 0)
    {
        state = Q_Pop(Q);
        for (b = 0; b<FSA_CONT_SIZE; ++b)
        {
            next = fsa_helper_get_next(helper, state, b);
            if (next != FSA_FAIL_STATE && next != 0)
                Q_Push(Q, next);
            else
            {
                next = fsa_helper_get_next(helper, helper->FailTable[state], b);
                if (next != FSA_FAIL_STATE && next != 0)
                {
                    ret = fsa_helper_put_next(helper, state, b, next);
                    if (ret != 0)
                        return ret;
                }
            }
        } // for
    } // while Q
    Q_free(Q);

    return 0;
}


/**
 * @brief 计算NFA_LIST或DFA_LIST状态机所需内存块大小.
 * 
 * @param fsa [IN] 状态机.
 * @param size [OUT] 所需内存大小.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_list_memsize(fsa_t* fsa, uint64_t* size)
{
    fsa_error_t ret = 0;
    fsa_compile_helper_t* helper;
    fsa_translist_t* tran_lst;
    //fsa_ptnlist_t* ptn_lst;
    uint32_t i, cnt;

    helper = (fsa_compile_helper_t*) fsa->Helper;
    *size = 0;

    /// Ptns
    //*size += sizeof(fsa_pattern_t*) * helper->PtnCnt;
    *size += sizeof(fsa_pattern_t) * helper->PtnCnt;

    /// index
    *size += sizeof(fsa_list_index_t) * helper->ActualStateCnt;

    /// trans + match
    for (i=0; i<helper->ActualStateCnt; ++i)
    {
        if (helper->TransList[i] != NULL)
        {
            cnt = 0;
            FSALIST_FOR_EACH(tran_lst, helper->TransList[i])
            { cnt++; }
            *size += sizeof(fsa_trans_t) * cnt;
        }

        //if (helper->match_list[i] != NULL)
        //{
        //    cnt = 0;
        //    FSALIST_FOR_EACH(ptn_lst, helper->match_list[i])
        //    { cnt++; }
        //    *size += sizeof(fsa_pattern_t*) * cnt;
        //}
        
        if(helper->MatchList[i] != NULL)
        {
            fsa_match_list_t* mlst;

            cnt = 0;
            FSALIST_FOR_EACH(mlst, helper->MatchList[i])
                cnt++;
            *size += sizeof(fsa_pattern_t*) * cnt;
        }
    }

    return ret;
}


/**
 * @brief 计算DFA_FULL_MATRIX状态机所需内存块大小.
 * 
 * @param fsa [IN] 状态机.
 * @param size [OUT] 所需内存大小.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_dfa_full_memsize(fsa_t* fsa, uint64_t* size)
{
    fsa_error_t ret = 0;
    fsa_compile_helper_t* helper;
    //fsa_ptnlist_t* ptn_lst;
    uint32_t i, cnt;

    helper = (fsa_compile_helper_t*) fsa->Helper;
    *size = 0;

    /// Ptns
    //*size += sizeof(fsa_pattern_t*) * helper->PtnCnt;
    *size += sizeof(fsa_pattern_t) * helper->PtnCnt;

    /// index + trans
    *size += sizeof(fsa_full_index_t) * helper->ActualStateCnt; // fsa->StateCnt;

    /// match
    for (i=0; i<helper->ActualStateCnt; ++i)
    {
//        if (helper->match_list[i] != NULL)
//        {
//            cnt = 0;
//            FSALIST_FOR_EACH(ptn_lst, helper->match_list[i])
//            { cnt++; }
//            *size += sizeof(fsa_pattern_t*) * cnt;
//        }
//
        if(helper->MatchList[i] != NULL)
        {
            fsa_match_list_t* mlst;

            cnt = 0;
            FSALIST_FOR_EACH(mlst, helper->MatchList[i])
                cnt++;
            *size += sizeof(fsa_pattern_t*) * cnt;
        }
    }

    return ret;
}


/**
 * @brief 将某个状态的状态转换表转换为矩阵行.
 * 
 * @param lst_head [IN] 状态转换表.
 * @param row [OUT] 转换后的状态转换矩阵行.
 */
static void fsa_convert_fullrow(fsa_translist_t* lst_head, state_t* row)
{
    fsa_translist_t* lst;

    FSALIST_FOR_EACH(lst, lst_head)
    {
        row[lst->input] = lst->next_state;
    }
}


/**
 * @brief 计算DFA_BANDED_MATRIX状态机所需内存块大小.
 * 
 * @param fsa [IN] 状态机.
 * @param size [OUT] 所需内存大小.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_dfa_band_memsize(fsa_t* fsa, uint64_t* size)
{
    fsa_error_t ret = FSA_ERR_OK;
    fsa_compile_helper_t* helper;
    //fsa_ptnlist_t* ptn_lst;
    state_t row[FSA_CONT_SIZE];
    int32_t first, last;
    uint32_t i, j, cnt;

    helper = (fsa_compile_helper_t*) fsa->Helper;
    *size = 0;

    /// Ptns
    //*size += sizeof(fsa_pattern_t*) * helper->PtnCnt;
    *size += sizeof(fsa_pattern_t) * helper->PtnCnt;

    /// index
    *size += sizeof(fsa_banded_index_t) * helper->ActualStateCnt; // fsa->StateCnt;

    /// trans + match
    for (i=0; i<helper->ActualStateCnt; ++i)
    {
        memset(row, 0, sizeof(row));    
        first = last = -1;

        if (helper->TransList[i] != NULL)
        {
            fsa_convert_fullrow(helper->TransList[i], row);
        }

        for (j=0; j<FSA_CONT_SIZE; ++j)
        {
            if (row[j] != 0)
            {
                if (-1 == first)
                    first = j;
                last = j;
            }
        }

        cnt = last - first + 1;
        //*size += cnt * sizeof(uint8_t);
        *size += cnt * sizeof(state_t);

        //if (helper->match_list[i] != NULL)
        //{
        //    cnt = 0;
        //    FSALIST_FOR_EACH(ptn_lst, helper->match_list[i])
        //    { cnt++; }
        //    *size += sizeof(fsa_pattern_t*) * cnt;
        //}
        
        if(helper->MatchList[i] != NULL)
        {
            fsa_match_list_t* mlst;

            cnt = 0;
            FSALIST_FOR_EACH(mlst, helper->MatchList[i])
                cnt++;
            *size += sizeof(fsa_pattern_t*) * cnt;
        }
    }

    return ret;
}


/**
 * @brief 将某状态在堆上的状态转换表转换为数组结构.
 * 
 * @param helper [IN/OUT] 状态编译辅助结构.
 * @param state [IN] 状态.
 * @param arr [OUT] 此状态的状态转换数组.
 * @param size [OUT] 转换后的数组大小.
 */
static void fsa_convert_translist(fsa_compile_helper_t* helper, 
                    uint32_t state, 
                    fsa_trans_t** arr, uint32_t* size)
{
    fsa_translist_t* L;
    uint32_t i;

    *size = 0;
    if (helper->TransList[state] != NULL)
    {
        for (L = helper->TransList[state]; L != NULL; L = L->next)
        {    (*size)++;   }
        *arr = (fsa_trans_t*) (helper->Mem + helper->MemOffset);
        helper->MemOffset += sizeof(fsa_trans_t) * (*size);

        for (L = helper->TransList[state], i = 0; 
             L != NULL;
             L = L->next, ++i)
        {
            (*arr)[i].Input = L->input;
            (*arr)[i].nextState = L->next_state;
        }
    }
}


/**
 * @brief 将某状态在堆上的匹配表转换为数组结构.
 * 
 * @param helper [IN/OUT] 状态编译辅助结构.
 * @param state [IN] 状态.
 * @param arr [OUT] 此状态的匹配数组.
 * @param size [OUT] 转换后的数组大小.
 */
static void
fsa_convert_matchlist(fsa_t* fsa, uint32_t state, 
                 fsa_pattern_ptr_array_t* arr, uint32_t* size)
{
    //fsa_ptnlist_t* L;
    uint32_t i;
    fsa_compile_helper_t* helper;

    helper = (fsa_compile_helper_t*) fsa->Helper;


    //if (helper->match_list[state] != NULL)
    //{
    //    for (L = helper->match_list[state]; L != NULL; L = L->next)
    //    {   (*size)++;    }
    //    *arr = (fsa_pattern_t**) (helper->Mem + helper->MemOffset);
    //    helper->MemOffset += sizeof(fsa_pattern_t*) * (*size);

    //    for (L = helper->match_list[state], i = 0;
    //         L != NULL;
    //         L = L->next, ++i)
    //    {   
    //        (*arr)[i] = (fsa_pattern_t*)L->ptn;
    //    }
    //}

    if(helper->MatchList[state] != NULL)
    {
        fsa_match_list_t* L;
        for(L = helper->MatchList[state]; L != NULL; L = L->next)
            (*size)++;
        *arr = (fsa_pattern_t**) (helper->Mem + helper->MemOffset);
        helper->MemOffset += sizeof(fsa_pattern_t*) * (*size);

        for(L = helper->MatchList[state], i = 0;
            L != NULL;
            L = L->next, i++)
        {
            uint32_t uid = L->uid;
            (*arr)[i] = &fsa->PtnList[fsa->PtnCnt-uid-1];
        }
    }
}


/**
 * @brief 将NFA_LIST或DFA_LIST状态机转换到sniper内存块.
 * 
 * @param fsa [IN/OUT] 状态机.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_convert_list(fsa_t* fsa)
{
    fsa_compile_helper_t* helper;
    fsa_list_index_t *p, *IDX;
    uint32_t i;

    if ( !(fsa->Format == NFA_LIST && fsa->Status == FSA_STATUS_NFA) &&
         !(fsa->Format == DFA_LIST && fsa->Status == FSA_STATUS_DFA) )
         return FSA_ERR_ERROR;

    helper = (fsa_compile_helper_t*) fsa->Helper;

    fsa->StateCnt = helper->ActualStateCnt;
    fsa->TransCnt = helper->TransCnt;

    fsa->Mem = (fsa_list_index_t*) (helper->Mem + helper->MemOffset);
    helper->MemOffset += sizeof(fsa_list_index_t) * fsa->StateCnt;

    IDX = (fsa_list_index_t*) fsa->Mem;

    for (i = 0; i<fsa->StateCnt; ++i)
    {
        p = &IDX[i];
        p->State = i;
        p->TransCnt = 0;
        p->trans_addr = NULL;
        p->MatchCnt = 0;
        p->match_addr = NULL;

        if (fsa->Format == NFA_LIST)
            p->FailState = helper->FailTable[i]; 

        fsa_convert_translist(helper, i, &p->trans_addr, &p->TransCnt);
        fsa_convert_matchlist(fsa, i, &p->match_addr, &p->MatchCnt);
    }

    fsa->MemSize = helper->MemOffset;
    fsa_free_helper(helper);

    return FSA_ERR_OK;
}


/**
 * @brief 将DFA_FULL_MATRIX状态机转换到sniper内存块.
 * 
 * @param fsa [IN/OUT] 状态机.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_convert_fullmatrix(fsa_t* fsa)
{
    fsa_compile_helper_t* helper;
    fsa_full_index_t *p, *IDX;
    uint32_t i;

    if (fsa->Status != FSA_STATUS_DFA)
        return FSA_ERR_ERROR;

    helper = (fsa_compile_helper_t*) fsa->Helper;

    fsa->StateCnt = helper->ActualStateCnt;
    fsa->TransCnt = helper->TransCnt;

    fsa->Mem = (fsa_full_index_t*) (helper->Mem + helper->MemOffset);
    helper->MemOffset += sizeof(fsa_full_index_t) * fsa->StateCnt;

    IDX = (fsa_full_index_t*) fsa->Mem;

    for (i=0; i<fsa->StateCnt; ++i)
    {
        p = &IDX[i];
        p->State = i;
        p->MatchCnt = 0;
        p->match_addr = NULL;

        memset(p->Trans, 0, sizeof(state_t) * FSA_CONT_SIZE);   
        if (helper->TransList[i] != NULL)
        {
            fsa_convert_fullrow(helper->TransList[i], p->Trans);
        }

        fsa_convert_matchlist(fsa, i, &p->match_addr, &p->MatchCnt);
    }

    fsa->MemSize = helper->MemOffset;
    fsa_free_helper(helper);

    return 0;
}


/**
 * @brief 将DFA_BANDED_MATRIX状态机转换到sniper内存块.
 * 
 * @param fsa [IN/OUT] 状态机.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_convert_banded_matrix(fsa_t* fsa)
{
    fsa_compile_helper_t* helper;
    fsa_banded_index_t *p, *IDX;
    state_t row[FSA_CONT_SIZE];
    int32_t first, last;
    uint32_t i, j;
    

    if (fsa->Status != FSA_STATUS_DFA)
        return FSA_ERR_BAD_STATUS;

    helper = (fsa_compile_helper_t*) fsa->Helper;

    fsa->StateCnt = helper->ActualStateCnt;
    fsa->TransCnt = helper->TransCnt;

    fsa->Mem = (fsa_banded_index_t*) (helper->Mem + helper->MemOffset);
    helper->MemOffset += sizeof(fsa_banded_index_t) * fsa->StateCnt;

    IDX = (fsa_banded_index_t*) fsa->Mem;

    for (i=0; i<fsa->StateCnt; ++i)
    {
        p = &IDX[i];
        p->State = i;
        p->Len = 0;
        p->First = 0;
        p->Trans = NULL;
        p->MatchCnt = 0;
        p->match_addr = NULL;

        memset(row, 0, sizeof(row));    
        first = last = -1;

        if (helper->TransList[i] != NULL)
        {
            fsa_convert_fullrow(helper->TransList[i], row);
        }

        for (j=0; j<FSA_CONT_SIZE; ++j)
        {
            if (row[j] != 0)
            {
                if (-1 == first)
                    first = j;
                last = j;
            }
        }

        p->First = first;
        p->Len = last - first + 1;

        p->Trans = (state_t*) (helper->Mem + helper->MemOffset);
        helper->MemOffset += p->Len * sizeof(state_t);

        for (j=first; j<=(uint32_t)last; ++j)
            p->Trans[j-first] = row[j];

        fsa_convert_matchlist(fsa, i, &p->match_addr, &p->MatchCnt);
    }

    fsa->MemSize = helper->MemOffset;
    fsa_free_helper(helper);

    return 0;
}


static inline fsa_error_t
fsa_search_list(fsa_t* fsa, 
                  const uint8_t* data,
                  uint32_t datalen,
                  fsa_match_callback cb,
                  void* user_data)
{
    fsa_error_t ret = 0;
    fsa_list_index_t *p, *IDX;
    uint32_t i, j;
    uint8_t x;
    state_t state;

    IDX = (fsa_list_index_t*) fsa->Mem;

    state = 0;
    for (i=0; i<datalen; ++i)
    {
        if(fsa->flags & FSA_FLAG_CASESENSITIVE)
            x = data[i];
        else
            x = CASETAB(data[i]);

        if (fsa->Format == NFA_LIST)
        {
            p = &IDX[state];
            while (FSA_FAIL_STATE == fsa_list_get_next(p, state, x)) 
            {
                state = p->FailState;
                p = &IDX[state];
            }
            state = fsa_list_get_next(p, state, x);
        }
        else if (fsa->Format == DFA_LIST)
        {
            p = &IDX[state];
            state = fsa_list_get_next(p, state, x);
            if (FSA_FAIL_STATE == state)
                state = 0;
        }

        p = &IDX[state];
        for (j=0; j<p->MatchCnt; ++j) 
        { 
            cb(p->match_addr[j]->ptn_id, i, user_data);
#ifdef FSA_DEBUG
            printf("match: %s %u\n", (const char*)p->match_addr[j]->ptn, i);
#endif
        }
    }

    return ret;
}


/**
 * @brief 在DFA_FULL_MATRIX状态机上进行匹配.
 * 
 * @param fsa [IN] 状态机.
 * @param data [IN] 匹配内容.
 * @param datalen [IN] 匹配内容的长度.
 * @param offset [IN] 匹配起始位置（以字节为单位）.
 * @param len [IN] 匹配长度.
 * @param func [IN] 匹配处理函数指针.
 * @param rec [IN/OUT] 匹配簿记信息.  
 * 
 * @return fsa_error_t 错误码.
 */


static inline fsa_error_t
fsa_search_full_matrix(fsa_t* fsa, 
                      const uint8_t* data,
                      uint32_t datalen,
                      fsa_match_callback cb,
                      void* user_data)
{
    fsa_error_t ret = 0;
    fsa_full_index_t *p, *IDX;
    state_t state;
    uint32_t i, j;
    uint8_t x;

    IDX = (fsa_full_index_t*) fsa->Mem;

    state = 0;
    for (i=0; i<datalen; ++i)
    {
        if(fsa->flags & FSA_FLAG_CASESENSITIVE)
            x = data[i];
        else
            x = CASETAB(data[i]);

        p = &IDX[state];
        state = p->Trans[x];

        p = &IDX[state];
        for (j=0; j<p->MatchCnt; ++j) 
        { 
            cb(p->match_addr[j]->ptn_id, i, user_data);
#ifdef FSA_DEBUG
            printf("match: %s %u\n", (const char*)p->match_addr[j]->ptn, i);
#endif 
        }
    }

    return ret;
}


static inline fsa_error_t
fsa_search_banded_matrix(fsa_t* fsa, 
                      const uint8_t* data,
                      uint32_t datalen,
                      fsa_match_callback cb,
                      void* user_data)
{
    fsa_error_t ret = 0;
    fsa_banded_index_t *p, *IDX;
    state_t state;
    uint32_t i, j;
    uint8_t x;

    IDX = (fsa_banded_index_t*) fsa->Mem;

    state = 0;
    for (i=0; i<datalen; ++i)
    {
        if(fsa->flags & FSA_FLAG_CASESENSITIVE)
            x = data[i];
        else
            x = CASETAB(data[i]);

        p = &IDX[state];

        if (x < p->First || x >= (p->First + p->Len))
            state = 0;
        else
            state = p->Trans[x - p->First];

        p = &IDX[state];
        for (j=0; j<p->MatchCnt; ++j) 
        { 
            cb(p->match_addr[j]->ptn_id, i, user_data);
#ifdef FSA_DEBUG
            printf("match: %s %u\n", (const char*)p->match_addr[j]->ptn, i);
#endif
        }
    }

//END:

    return ret;
}


/**
 * @brief 在堆上编译状态机.
 *
 * @param fsa 状态机.
 * 
 * @return fsa_error_t 错误码.
 */
static fsa_error_t fsa_heap_compile(fsa_t* fsa)
{
    fsa_error_t ret = FSA_ERR_OK;
    fsa_compile_helper_t* helper = NULL;
    fsa_ptnlist_t* ptn_lst;
    //uint32_t i;

    helper = (fsa_compile_helper_t*) fsa->Helper;

    /// 估算最大可能状态数
    helper->MaxStateCnt = helper->ActualStateCnt = helper->TransCnt = 0;
    
    FSALIST_FOR_EACH(ptn_lst, helper->ptn_list)
    {
        helper->MaxStateCnt += ptn_lst->ptn->ptn_len;
    }
    helper->MaxStateCnt++;      // +0状态

    /// 预分配转换表
    helper->TransList = (fsa_translist_t**) malloc(helper->MaxStateCnt * sizeof(fsa_translist_t*));
    if (NULL == helper->TransList)
        return FSA_ERR_BAD_ALLOC;
    helper->HeapMemSize += helper->MaxStateCnt * sizeof(fsa_translist_t*);
    memset(helper->TransList, 0, helper->MaxStateCnt * sizeof(fsa_translist_t*));

    /// 预分配Match表
    //helper->match_list = (fsa_ptnlist_t**) malloc(helper->MaxStateCnt * sizeof(fsa_ptnlist_t*));
    //if (NULL == helper->match_list)
    //    return FSA_ERR_BAD_ALLOC;
    //helper->HeapMemSize += helper->MaxStateCnt * sizeof(fsa_ptnlist_t*);
    //memset(helper->match_list, 0, helper->MaxStateCnt * sizeof(fsa_ptnlist_t*));

    helper->MatchList = (fsa_match_list_t**) malloc(helper->MaxStateCnt * sizeof(fsa_match_list_t*));
    if(NULL == helper->MatchList)
        return FSA_ERR_BAD_ALLOC;
    helper->HeapMemSize += helper->MaxStateCnt * sizeof(fsa_match_list_t*);
    memset(helper->MatchList, 0, helper->MaxStateCnt * sizeof(fsa_match_list_t*));

    /// 添加所有状态
    //for(i=0; i<helper->PtnCnt; ++i)
    //{
    //    ret = fsa_add_states(helper, &helper->PtnArray[i], fsa->flags & FSA_FLAG_CASESENSITIVE);
    //    if (ret != 0)
    //        return ret;
    //}
    FSALIST_FOR_EACH(ptn_lst, helper->ptn_list)
    {
        ret = fsa_add_states(helper, ptn_lst->ptn, 
                fsa->flags & FSA_FLAG_CASESENSITIVE, ptn_lst->uid);
        if (ret != 0)
            return ret;
    }
    helper->ActualStateCnt++;   // +0状态
    
    ret = fsa_build_nfa(helper);
    if (ret != 0)
        return ret;
    fsa->Status = FSA_STATUS_NFA;

#ifdef FSA_DEBUG
    fsa_print_info(fsa);
#endif

    if (fsa->Format == DFA_LIST || fsa->Format == DFA_FULL_MATRIX || 
            fsa->Format == DFA_BANDED_MATRIX)
    {
        ret = fsa_build_dfa(helper);
        if (ret != 0)
            return ret;
        fsa->Status = FSA_STATUS_DFA;

#ifdef FSA_DEBUG
    fsa_print_info(fsa);
#endif
    }

    return FSA_ERR_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////

fsa_error_t fsa_init(fsa_t* fsa, fsa_format_e format, uint32_t flags)
{
    if (NULL == fsa)
        return FSA_ERR_ERROR;

    //fsa->Type = type;
    fsa->Format = format;
    fsa->flags = flags;
    fsa->Status = FSA_STATUS_INVALID;
    fsa->StateCnt = 0;
    fsa->PtnCnt = 0;
    fsa->TransCnt = 0;
    fsa->PtnList = NULL;
    fsa->Mem = NULL;
    fsa->MemSize = 0;
    fsa->Status = FSA_STATUS_INIT;
    //fsa->ptn_list = NULL;

    fsa->Helper = (fsa_compile_helper_t*) malloc(sizeof(fsa_compile_helper_t));
    if (NULL == fsa->Helper)
        return FSA_ERR_BAD_ALLOC;
    memset(fsa->Helper, 0, sizeof(fsa_compile_helper_t)); 

    return 0;
}


fsa_error_t fsa_deinit(fsa_t* fsa)
{
    fsa_compile_helper_t* helper;
    
    if (NULL == fsa)
        return FSA_ERR_BAD_ARG;

    /** @note 确保在某些失败（如状态机编译失败）时Helper所用的堆内存能够被释放. */
    if (fsa->Status != FSA_STATUS_COMPILED_FINISH)
    {
        helper = (fsa_compile_helper_t*) fsa->Helper;
        fsa_free_helper(helper);
    }

    fsa->Status = FSA_STATUS_INVALID;

    return FSA_ERR_OK;
}


fsa_error_t  fsa_add_pattern(fsa_t* fsa,
                             const uint8_t* ptn,
                             uint32_t ptn_len,
                             uint32_t ptn_id)
{
    fsa_error_t ret = 0;
    fsa_compile_helper_t* helper;
    fsa_ptnlist_t*  ptn_lst;

    if (NULL == fsa || NULL == ptn || ptn_len < 1)
        return FSA_ERR_BAD_ARG;
    if (fsa->Status != FSA_STATUS_INIT && fsa->Status != FSA_STATUS_PATTERN_ADDED)
        return FSA_ERR_BAD_STATUS;

    helper = (fsa_compile_helper_t*) fsa->Helper;

    ptn_lst = (fsa_ptnlist_t*) malloc(sizeof(fsa_ptnlist_t));
    if(NULL == ptn_lst)
        return FSA_ERR_BAD_ALLOC;
    ptn_lst->ptn = (fsa_pattern_t*) malloc(sizeof(fsa_pattern_t));
    if(NULL == ptn_lst->ptn)
        return FSA_ERR_BAD_ALLOC;

    ptn_lst->ptn->ptn = ptn;
    ptn_lst->ptn->ptn_len = ptn_len;
    ptn_lst->ptn->ptn_id = ptn_id;
    ptn_lst->uid = helper->PtnCnt;

    helper->HeapMemSize += sizeof(fsa_ptnlist_t) + sizeof(fsa_pattern_t);
    ptn_lst->next = helper->ptn_list;
    helper->ptn_list = ptn_lst;
    helper->PtnCnt++;
    
    fsa->Status = FSA_STATUS_PATTERN_ADDED;

    return ret;
}


fsa_error_t  fsa_need_memsize(fsa_t* fsa, uint64_t* size)
{
    fsa_error_t ret = FSA_ERR_OK;

    if (NULL == fsa || NULL == size)
        return FSA_ERR_BAD_ARG;
    if (fsa->Status != FSA_STATUS_PATTERN_ADDED) 
        return FSA_ERR_BAD_STATUS;

    ret = fsa_heap_compile(fsa);
    if (ret != 0)
        goto END;

    switch (fsa->Format)
    {
    case NFA_LIST:
    case DFA_LIST:
        ret = fsa_list_memsize(fsa, size);
        break;
    case DFA_FULL_MATRIX:
        ret = fsa_dfa_full_memsize(fsa, size);
        break;
    case DFA_BANDED_MATRIX:
        ret = fsa_dfa_band_memsize(fsa, size);
        break;
    default:
        return FSA_ERR_BAD_FORMAT;
    }

END:
    return ret;
}


fsa_error_t  fsa_compile(fsa_t* fsa, uint8_t* mem, uint64_t size)
{
    fsa_error_t ret;
    fsa_compile_helper_t* helper;
    fsa_ptnlist_t* ptn_lst;
    uint32_t i;

    if (NULL == fsa || NULL == mem || size < 1)
        return FSA_ERR_BAD_ARG;
    if (fsa->Status != FSA_STATUS_PATTERN_ADDED && 
        fsa->Status != FSA_STATUS_NFA && 
        fsa->Status != FSA_STATUS_DFA) 
        return FSA_ERR_BAD_STATUS;

    helper = (fsa_compile_helper_t*) fsa->Helper;
    if (helper->PtnCnt < 1)
        return FSA_ERR_ERROR;

    helper->Mem = mem;
    helper->MemOffset = 0ULL;
    helper->MemSize = size;

    if (fsa->Status == FSA_STATUS_PATTERN_ADDED)
    {
        /** @note 在此失败时不应调用fsa_free_helper，
         * 也许应给用户一个机会，等待堆内存在下次调用时够用. */
        ret = fsa_heap_compile(fsa); 
        if (ret != 0)
        {
            return ret;
        }
    }

    fsa->PtnList = (fsa_pattern_t*) helper->Mem;
    helper->MemOffset = sizeof(fsa_pattern_t) * helper->PtnCnt;

    for (ptn_lst = helper->ptn_list, i = 0;
         ptn_lst != NULL;
         ptn_lst = ptn_lst->next, ++i)
    {
        //fsa->PtnList[i] = ptn->ptn;
        memcpy(&fsa->PtnList[i], ptn_lst->ptn, sizeof(fsa_pattern_t));
    }
    fsa->PtnCnt = helper->PtnCnt;

    switch (fsa->Format)
    {
    case NFA_LIST:
    case DFA_LIST:
        ret = fsa_convert_list(fsa);  
        //fsa->SearchFunc = FSA_SearchList;
        break;
    case DFA_FULL_MATRIX:
        ret = fsa_convert_fullmatrix(fsa);
        //fsa->SearchFunc = fsa_search_full_matrix;
        break;
    case DFA_BANDED_MATRIX:
        ret = fsa_convert_banded_matrix(fsa);
        //fsa->SearchFunc = FSA_SearchBandedMatrix;
        break;
    default:
        ret = FSA_ERR_BAD_FORMAT;
        break;
    }

    if (ret != 0)
        return ret;

    fsa->Status = FSA_STATUS_COMPILED_FINISH;

#ifdef FSA_DEBUG
    fsa_print_info(fsa);
#endif

    return ret;
}



fsa_error_t fsa_search(fsa_t* fsa, 
                       const uint8_t* data, 
                       uint64_t data_len,
                       fsa_match_callback cb,
                       void* user_data)
{
    fsa_error_t ret = 0;
    
    if (NULL == fsa || NULL == data || data_len < 1 || NULL == cb)
        return FSA_ERR_BAD_ARG;

    if (fsa->Status != FSA_STATUS_COMPILED_FINISH)
        return FSA_ERR_BAD_STATUS;

    switch (fsa->Format)
    {
    case NFA_LIST:
    case DFA_LIST:
        ret = fsa_search_list(fsa, data, data_len, cb, user_data);
        break;
    case DFA_FULL_MATRIX:
        ret = fsa_search_full_matrix(fsa, data, data_len, cb, user_data);
        break;
    case DFA_BANDED_MATRIX:
        ret = fsa_search_banded_matrix(fsa, data, data_len, cb, user_data);
        break;
    default:
        return FSA_ERR_BAD_FORMAT; 
    }

    //ret = fsa_search_full_matrix(fsa, data, data_len, cb, user_data);

    return ret;
}



uint32_t  fsa_get_pattern_count(fsa_t* fsa)
{
    fsa_compile_helper_t*  helper;
        
    if (NULL == fsa || fsa->Status == FSA_STATUS_INVALID)
        return 0;
    else if (fsa->Status == FSA_STATUS_COMPILED_FINISH)
        return fsa->PtnCnt;
    else
    {
        helper = (fsa_compile_helper_t*) fsa->Helper;
        return helper->PtnCnt;
    }
}


uint32_t  fsa_get_state_count(fsa_t* fsa)
{
    fsa_compile_helper_t*  helper;

    if (NULL == fsa || fsa->Status == FSA_STATUS_INVALID)
        return 0;
    else if (fsa->Status == FSA_STATUS_COMPILED_FINISH)
        return fsa->StateCnt;
    else
    {
        helper = (fsa_compile_helper_t*) fsa->Helper;
        return helper->ActualStateCnt;
    }
}



/*
#define OFFSET_MATCH(ptn, pos, dlen) \
    ( ptn->OffSet == -1 || \
    ((ptn->Dir == RM_CONTOPT_DIR_HEAD && ptn->OffSet == (pos+1-ptn->Depth) ) || \
     (ptn->Dir == RM_CONTOPT_DIR_TAIL && (dlen-ptn->OffSet-1) == (pos+1-ptn->Depth))))
 */



fsa_error_t  fsa_print_info(fsa_t* fsa)
{
    fsa_compile_helper_t* helper;
    fsa_translist_t* tran;
    fsa_match_list_t* ptn;
    uint32_t i;

    if (NULL == fsa)
    {
        fprintf(stderr, "FSA is NULL\n");
        return FSA_ERR_BAD_ARG;
    }

    helper = (fsa_compile_helper_t*) fsa->Helper;

    fprintf(stdout, "==================================================\n");

    switch (fsa->Format)
    {
    case NFA_LIST:
        fprintf(stdout, "Format: NFA_LIST\n");
        break;
    case DFA_LIST:
        fprintf(stdout, "Format: DFA_LIST\n");
        break;
    case DFA_FULL_MATRIX:
        fprintf(stdout, "Format: DFA_FULL_MATRIX\n");
        break;
    case DFA_BANDED_MATRIX:
        fprintf(stdout, "Format: DFA_BANDED_MATRIX\n");
        break;
    default:
        return FSA_ERR_BAD_FORMAT;
    }
    

    switch (fsa->Status)
    {
    case FSA_STATUS_INIT:
        fprintf(stdout, "Status: FSA_STATUS_INIT\n");
        break;
    case FSA_STATUS_PATTERN_ADDED:
        fprintf(stdout, "Status: FSA_STATUS_PATTERN_ADDED\n");
        fprintf(stdout, "PtnCnt: %u\n", helper->PtnCnt);
#ifdef __GNUC__
        fprintf(stdout, "HeapMemSize: %lu (at Heap)\n", helper->HeapMemSize);
#else
        fprintf(stdout, "HeapMemSize: %llu (at Heap)\n", helper->HeapMemSize);
#endif
        break;
    case FSA_STATUS_NFA:
    case FSA_STATUS_DFA:
        if (FSA_STATUS_NFA == fsa->Status)
            fprintf(stdout, "Status: FSA_STATUS_NFA\n"); 
        else
            fprintf(stdout, "Status: FSA_STATUS_DFA\n");

        fprintf(stdout, "PtnCnt: %u\n", helper->PtnCnt);
        fprintf(stdout, "MaxStateCnt: %u\n", helper->MaxStateCnt);
        fprintf(stdout, "ActualStateCnt: %u\n", helper->ActualStateCnt);
        fprintf(stdout, "TransCnt: %u\n", helper->TransCnt);
#ifdef __GNUC__
        fprintf(stdout, "HeapMemSize: %lu (at Heap)\n", helper->HeapMemSize);
#else
        fprintf(stdout, "HeapMemSize: %llu (at Heap)\n", helper->HeapMemSize);
#endif
        fprintf(stdout, "Details:\n");
        for (i=0; i<helper->ActualStateCnt; ++i)
        {
            if (FSA_STATUS_NFA == fsa->Status)
                fprintf(stdout, "\t%d, %d, ", i, helper->FailTable[i]);
            else
                fprintf(stdout, "\t%d ", i);

            FSALIST_FOR_EACH(tran, helper->TransList[i])
            {
                char ch[8] = { (char)tran->input };
                if (ch[0] == '\r')
                { ch[0] = '\\'; ch[1] = 'r'; }
                else if (ch[0] == '\n')
                { ch[0] = '\\'; ch[1] = 'n'; }
                if (isprint(ch[0]))
                    fprintf(stdout, "%s->%d ", ch, tran->next_state);
                else
                    fprintf(stdout, "%x->%d ", CASETAB(ch[0]), tran->next_state);
            }

            FSALIST_FOR_EACH(ptn, helper->MatchList[i])
            {
                fprintf(stdout, "[%u] ", ptn->uid);
            }

            fprintf(stdout, "\n");
        }
        break;
    case FSA_STATUS_COMPILED_FINISH:
        fprintf(stdout, "Status: FSA_STATUS_COMPILED_FINISH\n");
        fprintf(stdout, "PtnCnt: %u\n", fsa->PtnCnt);
        fprintf(stdout, "StateCnt: %u\n", fsa->StateCnt);
        fprintf(stdout, "TransCnt: %u\n", fsa->TransCnt);
#ifdef __GUNC__
        fprintf(stdout, "MemSize: %llu (at RM)\n", fsa->MemSize);
#else
        fprintf(stdout, "MemSize: %lu (at RM)\n", fsa->MemSize);
#endif
        break;
    case FSA_STATUS_INVALID:
    default:
        fprintf(stderr, "invalid FSA status\n");
        break;
    }

    fprintf(stdout, "==================================================\n\n");

    return 0; 
}


