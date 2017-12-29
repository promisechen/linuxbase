/**
 * @file 
 * @brief fsa declaration.
 * @author zzq
 * @version 1.0
 * @date 2015-09-28
 */


#ifndef __HS_FSA_H__
#define __HS_FSA_H__


#include <stdint.h>


#ifdef __cplusplus
 extern "C" {
#endif



/** 失配状态 */
#define FSA_FAIL_STATE  -1

/** 可能的输入数 */
#define FSA_CONT_SIZE   256

/** 状态 */
typedef int32_t state_t;

/** 错误码 */
typedef int fsa_error_t;

/**
 * @name 状态机错误码.
 * @{
 */
#define FSA_ERR_OK      0
#define FSA_ERR_ERROR   -1
#define FSA_ERR_FAIL    -1

#define FSA_ERR_BAD_ARG     -6
#define FSA_ERR_BAD_ALLOC   -7
#define FSA_ERR_BAD_FORMAT  -8
#define FSA_ERR_BAD_STATUS  -9
/** @} */

typedef struct fsa_pattern
{
    const uint8_t* ptn;
    uint32_t ptn_len;
    uint32_t ptn_id;
} fsa_pattern_t;


/**
 * @brief 状态机数据结构类型（存储）.
 */
typedef enum fsa_format
{
    NFA_LIST = 0,       /**< 链表式NFA */
    DFA_LIST,           /**< 链表式DFA */
    DFA_FULL_MATRIX,    /**< 全矩阵式DFA */
    DFA_BANDED_MATRIX   /**< 限带矩阵式DFA */
} fsa_format_e;


/**
 * @name 状态机标记.
 * @{
 */
/** 大小写敏感 */
#define FSA_FLAG_CASESENSITIVE  1
/** 全字匹配(英文时有意义) */
#define FSA_FLAG_WHOLEWORD      2
/** @} */


/**
 * @brief 状态机状态.
 */
typedef enum fsa_status
{
    FSA_STATUS_INVALID = 0,     /**< 无效 */
    FSA_STATUS_INIT,            /**< 已初始化 */
    FSA_STATUS_PATTERN_ADDED,   /**< 已添加模式 */
    FSA_STATUS_NFA,             /**< 已在堆上编译NFA */
    FSA_STATUS_DFA,             /**< 已在堆上编译DFA */
    FSA_STATUS_COMPILED_FINISH  /**< 已转换到sniper内存 */
} fsa_status_e;


/**
 * @brief 多模匹配有限状态机. 
 * @note 为对调用者达成数据封装和信息隐藏目的，Helper和Mem成员声明为void*. 
 */
typedef struct fsa
{
    //fsa_type_e    Type;             /**< 状态机用途 */
    fsa_format_e  Format;           /**< 状态机存储结构 */
    uint32_t     flags;
    fsa_status_e  Status;           /**< 状态 */
    uint32_t     pad;
    uint32_t     StateCnt;         /**< 状态总数 */
    uint32_t     PtnCnt;           /**< 模式数 */
    uint32_t     TransCnt;         /**< 状态转换数 */
    uint64_t     MemSize;          /**< 占用内存字节数 */
    //RM_BDOPT_STRING_S**  PtnList;   /**< 模式表 */
    fsa_pattern_t*     PtnList;
    void*      Helper;           /**< 不透明编译辅助数据 */
    void*      Mem;              /**< 不透明内部内存结构 */
    void*      SearchFunc;       /**< 搜索函数指针，实际类型为FSASearchFunction */
} fsa_t;



/////////////////////////////////////////////////////////////////////////////////////////


/**
 * @brief 搜索函数原型. 
 * @note 不同Format的FSA使用基于此原型的不同搜索函数，这些函数在fsa.c中. 
 *  
 * @param data [IN] 匹配内容.
 * @param datalen [IN] 匹配内容的长度.
 *  
 * @return int32_t 错误码，具体含义见sp_error.h. 
 */
//typedef int32_t (*FSASearchFunction) (SP_FSA_S* fsa, 
//                                       const uint8_t* data, uint32_t datalen);

typedef int (*fsa_match_callback) (uint32_t ptn_id,
                                   uint64_t offset,
                                   void* user_data);

/**
 * @brief 初始化状态机.
 *
 * @param fsa [IN/OUT] 状态机.
 * @param format [IN] 状态机存储格式.
 * 
 * @return int32_t 错误码，具体含义见sp_error.h. 
 */
fsa_error_t fsa_init(fsa_t* fsa, fsa_format_e format, uint32_t flags);

/**
 * @brief 反初始化状态机.
 * 
 * @param fsa 状态机.
 * @return int32_t 错误码，具体含义见sp_error.h. 
 */
fsa_error_t fsa_deinit(fsa_t* fsa);


/**
 * @brief 返回状态机当前的模式串数.
 * 
 * @param fsa [IN] 状态机.
 * 
 * @return uint32_t 添加到状态机的模式串总数，fsa状态无效时返回0.
 */
uint32_t  fsa_get_pattern_count(fsa_t* fsa);


/**
 * @brief 返回状态机当前状态数.
 * 
 * @param fsa [IN] 状态机.
 * 
 * @return uint32_t 状态机状态总数, fsa无效时返回0.
 */
uint32_t  fsa_get_state_count(fsa_t* fsa);


/**
 * @brief 返回状态当前的构建阶段.
 * 
 * @param fsa [IN] 状态机.
 * 
 * @return FSA_STATUS_E 状态机的构建阶段枚举值, fsa无效时返回FSA_STATUS_INVALID.
 */
static inline fsa_status_e  fsa_get_status(fsa_t* fsa)
{
    return (fsa ? fsa->Status : FSA_STATUS_INVALID);
}


/**
 * @brief 为状态机添加模式.
 * 
 * @param fsa [IN/OUT] 状态机.
 * @param ptn [IN] 模式.
 * 
 * @return int32_t 错误码，具体含义见sp_error.h.
 */


fsa_error_t  fsa_add_pattern(fsa_t* fsa,
                             const uint8_t* ptn,
                             uint32_t ptn_len,
                             uint32_t ptn_id);

/**
 * @brief 计算状态机所需内存大小.
 * 
 * @param fsa [IN/OUT] 状态机.
 * @param size [OUT] 所需内存大小（字节数）. 
 *  
 * @return int32_t 错误码，具体含义见sp_error.h.
 */
fsa_error_t fsa_need_memsize(fsa_t* fsa, uint64_t* size);



/**
 * @brief 编译状态机. 
 * @note 
 *    1. FSA_Compile内部多次调用RM_Calloc来分配FSA所需的内存；而FSA_Compile内部 
 *       没有内存分配操作，调用者给出内存起始地址mem和大小size，FSA直接在mem上进行
 *       内存布局. 
 *    2. FSA_Compile内部不检查内存是否足够，请调用FSA_NeedMemSize来确定所需内存大小.
 *  
 * @param fsa [IN/OUT] 状态机.
 * @param mem [IN/OUT] 内存块起始地址.
 * @param size [IN] 内存块大小. 
 *  
 * @return int32_t 错误码，具体含义见sp_error.h.
 */
fsa_error_t fsa_compile(fsa_t* fsa, uint8_t* mem, uint64_t size);


/**
 * @brief 使用状态机进行模式匹配.
 * 
 * @param fsa [IN] 状态机.
 * @param data [IN] 匹配内容.
 * @param datalen [IN] 匹配内容的长度.
 * 
 * @return int32_t 错误码，具体含义见sp_error.h.
 */
fsa_error_t fsa_search(fsa_t* fsa, 
                       const uint8_t* data, 
                       uint64_t data_len,
                       fsa_match_callback cb,
                       void* user_data);



/**
 * @brief 输出状态机信息.
 * 
 * @param fsa [IN] 状态机.
 * @return int32_t 错误码，具体含义见sp_error.h.
 */
fsa_error_t  fsa_print_info(fsa_t* fsa);



#ifdef __cplusplus
 }
#endif /* __cplusplus */


#endif /* __HS_FSA_H__ */

