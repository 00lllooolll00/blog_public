/**
 * @file Pool.c
 * @brief 内存池管理模块实现
 * @details 实现固定大小块的内存池分配与回收功能
 *          支持多块连续分配和释放时的合并优化
 * @author N1ntyNine99
 * @date 2025-05-07
 */

/*
//                          _ooOoo_                               //
//                         o8888888o                              //
//                         88" . "88                              //
//                         (| -_- |)                              //
//                         O\  =  /O                              //
//                      ____/`---'\____                           //
//                    .'  \\|     |//  `.                         //
//                   /  \\|||  :  |||//  \                        //
//                  /  _||||| -:- |||||-  \                       //
//                  |   | \\\  -  /// |   |                       //
//                  | \_|  ''\---/''  |   |                       //
//                  \  .-\__  `-`  ___/-. /                       //
//                ___`. .'  /--.--\  `. . ___                     //
//            \  \ `-.   \_ __\ /__ _/   .-` /  /                 //
//      ========`-.____`-.___\_____/___.-`____.-'========         //
//                           `=---='                              //
//      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        //
//      佛祖保佑                永无BUG               永不修改       //
*/

/* ========================= 头文件包含区 ========================= */
#include "Pool.h"

/* ========================= 宏定义区 ========================= */
/** @brief 内存池总大小 */
#define MEMPOOL_TOTAL_SIZE (MEMPOOL_BLOCK_NUM * MEMPOOL_BLOCK_SIZE)

/* ========================= 全局变量区 ========================= */
/** @brief 主内存池结构，放置在SRAM区域 */
__attribute__((section(".sram"))) static mempool_t MainPool;

/** @brief 内存池块大小 */
static uint32_t MEMPOOL_BLOCK_SIZE = 0;
/** @brief 内存池块数量 */
static uint32_t MEMPOOL_BLOCK_NUM = 0;

/* ========================= 内部函数前置声明区 ========================= */
static bool MemPool_Check(void);
static __always_inline void *__MemPool_MallocOnce(void);
static __always_inline void *__MemPool_MallocMulti(uint32_t blocks_num);
static __always_inline bool __MemPool_Free(void *p, uint32_t blocks_num, uint32_t block_start_index);

/* ========================= 内存池初始化与销毁区 ========================= */

/**
 * @brief 初始化内存池
 * @param block_size 每个内存块的大小(字节)
 * @param block_num 内存块的数量
 * @return 初始化是否成功
 * @retval true 初始化成功
 * @retval false 初始化失败
 */
bool MemPool_Init(uint32_t block_size, uint32_t block_num)
{
    /* 参数检查 */
    if (block_size == 0 || block_num == 0) return false;

    MEMPOOL_BLOCK_SIZE = block_size;
    MEMPOOL_BLOCK_NUM = block_num;

    /* 分配内存池空间和块信息数组 */
    MainPool.Mempool_Total = (uint8_t *)malloc(MEMPOOL_TOTAL_SIZE);
    MainPool.Mempool_Block_Info = (pool_block_t *)malloc(MEMPOOL_BLOCK_NUM * sizeof(pool_block_t));

    /* 检查内存分配是否成功 */
    if (MainPool.Mempool_Total == NULL || MainPool.Mempool_Block_Info == NULL)
    {
        if (MainPool.Mempool_Total)
        {
            free(MainPool.Mempool_Total);
            MainPool.Mempool_Total = NULL;
        }
        if (MainPool.Mempool_Block_Info)
        {
            free(MainPool.Mempool_Block_Info);
            MainPool.Mempool_Block_Info = NULL;
        }
        return false;
    }

    /* 初始化内存池数据和块信息 */
    memset(MainPool.Mempool_Total, 0, MEMPOOL_TOTAL_SIZE);
    for (uint32_t i = 0; i < MEMPOOL_BLOCK_NUM; i++)
    {
        MainPool.Mempool_Block_Info[i].Block_Next = NULL;
        MainPool.Mempool_Block_Info[i].Block_Prev = NULL;
        MainPool.Mempool_Block_Info[i].Block_Info.Block_State = BLOCK_FREE;
        MainPool.Mempool_Block_Info[i].Block_Info.Blocks_Num = 0;
    }

    /* 构建空闲块链表 */
    MainPool.Mempool_FreeBlockHead = &MainPool.Mempool_Block_Info[0];
    pool_block_t *CurrentNode = MainPool.Mempool_FreeBlockHead;

    for (uint32_t i = 0; i < MEMPOOL_BLOCK_NUM - 1; i++)
    {
        CurrentNode->Block_Next = &MainPool.Mempool_Block_Info[i + 1];
        CurrentNode = CurrentNode->Block_Next;
        CurrentNode->Block_Prev = &MainPool.Mempool_Block_Info[i];
    }

    MainPool.Mempool_Block_Info[MEMPOOL_BLOCK_NUM - 1].Block_Next = NULL;
    MainPool.Mempool_Busy = 0;
    MainPool.Mempool_Free = MEMPOOL_BLOCK_NUM - MainPool.Mempool_Busy;

    return true;
}

/**
 * @brief 释放内存池资源
 */
void MemPool_Deinit(void)
{
    /* 释放内存池空间 */
    if (MainPool.Mempool_Total)
    {
        free(MainPool.Mempool_Total);
        MainPool.Mempool_Total = NULL;
    }
    if (MainPool.Mempool_Block_Info)
    {
        free(MainPool.Mempool_Block_Info);
        MainPool.Mempool_Block_Info = NULL;
    }

    /* 重置内存池状态 */
    MainPool.Mempool_FreeBlockHead = NULL;
    MainPool.Mempool_Busy = 0;
    MainPool.Mempool_Free = 0;

    MEMPOOL_BLOCK_SIZE = 0;
    MEMPOOL_BLOCK_NUM = 0;
}

/* ========================= 内存池完整性检查区 ========================= */

/**
 * @brief 检查内存池的完整性
 * @return 内存池是否完整
 * @retval true 内存池完整
 * @retval false 内存池有异常
 */
static bool MemPool_Check(void)
{
    uint32_t free_count = 0;

    /* 统计空闲块数量 */
    for (uint32_t i = 0; i < MEMPOOL_BLOCK_NUM; i++)
    {
        if (MainPool.Mempool_Block_Info[i].Block_Info.Block_State == BLOCK_FREE) free_count++;
    }

    /* 检查空闲块数量与记录是否一致 */
    if (free_count != MainPool.Mempool_Free) return false;

    /* 检查忙块数量与记录是否一致 */
    if (MEMPOOL_BLOCK_NUM - free_count != MainPool.Mempool_Busy) return false;

    uint32_t list_count = 0;
    pool_block_t *curr = MainPool.Mempool_FreeBlockHead;

    bool res = false;

    /* 快慢指针检测链表是否有环 */
    pool_block_t *p_slow = MainPool.Mempool_FreeBlockHead;
    pool_block_t *p_fast = MainPool.Mempool_FreeBlockHead;
    bool is_cycle = false;

    while (p_fast && p_fast->Block_Next)
    {
        p_slow = p_slow->Block_Next;
        p_fast = p_fast->Block_Next->Block_Next;

        if (p_fast == p_slow)
        {
            is_cycle = true;
            break;
        }
    }

    if (is_cycle == true) return false; /* 说明链表中有环路，有问题return false */

    /* 遍历链表检查每个节点 */
    while (curr)
    {
        /* 检查节点指针是否在有效范围内 */
        if ((curr < MainPool.Mempool_Block_Info) || (curr >= MainPool.Mempool_Block_Info + MEMPOOL_BLOCK_NUM))
        {
            return false;
        }

        /* 检查节点状态是否为空闲 */
        if (curr->Block_Info.Block_State != BLOCK_FREE) return false;
        curr = curr->Block_Next;
        list_count++;

        /* 防止链表过长（可能有环但没被快慢指针检测到） */
        if (list_count > MEMPOOL_BLOCK_NUM) return false;
    }
    res = (list_count == free_count) ? true : false;
    return res;
}

/* ========================= 内存分配功能区 ========================= */

/**
 * @brief 分配单个内存块
 * @return 分配的内存块指针
 * @retval NULL 分配失败
 */
static __always_inline void *__MemPool_MallocOnce(void)
{
    /* 获取空闲块链表头 */
    pool_block_t *p = MainPool.Mempool_FreeBlockHead;
    if (!p) return NULL;

    /* 更新空闲块链表头 */
    MainPool.Mempool_FreeBlockHead = p->Block_Next;

    /* 更新链表连接 */
    if (MainPool.Mempool_FreeBlockHead) MainPool.Mempool_FreeBlockHead->Block_Prev = NULL;
    p->Block_Next = NULL;
    p->Block_Info.Block_State = BLOCK_BUSY;
    p->Block_Info.Blocks_Num = 1;

    /* 计算实际内存地址 */
    void *blocks = (void *)(MainPool.Mempool_Total + (p - MainPool.Mempool_Block_Info) * MEMPOOL_BLOCK_SIZE);

    /* 清除内存块数据 */
    memset(blocks, 0, MEMPOOL_BLOCK_SIZE);

    /* 更新内存池状态 */
    MainPool.Mempool_Busy++;
    MainPool.Mempool_Free--;
    return blocks;
}

/**
 * @brief 分配多个连续内存块
 * @param blocks_num 需要分配的内存块数量
 * @return 分配的内存块起始指针
 * @retval NULL 分配失败
 */
static __always_inline void *__MemPool_MallocMulti(uint32_t blocks_num)
{
    pool_block_t *start = NULL;
    uint32_t cnt = 0;
    uint32_t start_idx = 0;

    /* 查找连续的空闲块 */
    for (uint32_t i = 0; i < MEMPOOL_BLOCK_NUM; i++)
    {
        if (MainPool.Mempool_Block_Info[i].Block_Info.Block_State == BLOCK_FREE)
        {
            if (!cnt)
            {
                start = &MainPool.Mempool_Block_Info[i];
                start_idx = i;
            }
            cnt++;
            if (cnt == blocks_num) break;
        }
        else
        {
            start = NULL;
            cnt = 0;
        }
    }

    /* 没找到足够的连续空闲块 */
    if (cnt < blocks_num) return NULL;
    uint32_t blocks_remove_index[blocks_num];

    /* 记录需要从空闲链表中移除的块索引 */
    for (uint32_t i = 0; i < blocks_num; i++)
    {
        blocks_remove_index[i] = start_idx + i;
    }

    /* 从空闲链表中移除这些块 */
    for (uint32_t i = 0; i < blocks_num; i++)
    {
        pool_block_t *p = &MainPool.Mempool_Block_Info[blocks_remove_index[i]];

        if (p->Block_Prev) p->Block_Prev->Block_Next = p->Block_Next;
        else if (MainPool.Mempool_FreeBlockHead == p)
        {
            MainPool.Mempool_FreeBlockHead = p->Block_Next;
        }

        if (p->Block_Next) p->Block_Next->Block_Prev = p->Block_Prev;

        p->Block_Next = NULL;
        p->Block_Prev = NULL;
        p->Block_Info.Block_State = BLOCK_BUSY;
    }

    /* 更新起始块的信息 */
    start->Block_Info.Blocks_Num = blocks_num;
    void *blocks = (void *)(MainPool.Mempool_Total + (start - MainPool.Mempool_Block_Info) * MEMPOOL_BLOCK_SIZE);

    /* 清除内存数据 */
    memset(blocks, 0, MEMPOOL_BLOCK_SIZE * blocks_num);

    /* 更新内存池状态 */
    MainPool.Mempool_Busy += blocks_num;
    MainPool.Mempool_Free -= blocks_num;

    return blocks;
}

/**
 * @brief 从内存池分配指定大小的内存
 * @param size 需要分配的内存大小(字节)
 * @return 分配的内存块指针
 * @retval NULL 分配失败
 */
void *MemPool_Malloc(size_t size)
{
    /* 检查是否有可用块 */
    if (!MainPool.Mempool_Free) return NULL;

    /* 计算需要的块数 */
    uint32_t block_need = (size + MEMPOOL_BLOCK_SIZE - 1) / MEMPOOL_BLOCK_SIZE;

    /* 检查空闲块数量是否足够 */
    if (block_need > MainPool.Mempool_Free) return NULL;

#ifdef DEBUG
    /* 调试模式下检查内存池完整性 */
    if (MemPool_Check() != true) return NULL;
#endif

    void *p = NULL;

    /* 根据需要的块数选择分配方式 */
    if (block_need == 1)
    {
        p = __MemPool_MallocOnce();
        return p;
    }
    else
    {
        p = __MemPool_MallocMulti(block_need);
        return p;
    }
}

/* ========================= 内存释放功能区 ========================= */

/**
 * @brief 释放内存块的内部实现
 * @param p 要释放的内存块指针
 * @param blocks_num 内存块数量
 * @param block_start_index 起始内存块索引
 * @return 是否成功释放
 * @retval true 释放成功
 * @retval false 释放失败
 */
static __always_inline bool __MemPool_Free(void *p, uint32_t blocks_num, uint32_t block_start_index)
{
    /* 验证所有块是否都处于忙状态 */
    for (uint32_t i = 0; i < blocks_num; i++)
    {
        if (MainPool.Mempool_Block_Info[block_start_index + i].Block_Info.Block_State != BLOCK_BUSY) return false;
    }

    /* 检查左侧相邻块是否空闲 */
    uint32_t left_freeblock_num = 0;
    int32_t left_lastfree_idx = (int32_t)block_start_index - 1;

    while (left_lastfree_idx >= 0 &&
           MainPool.Mempool_Block_Info[left_lastfree_idx].Block_Info.Block_State == BLOCK_FREE)
    {
        left_lastfree_idx--;
        left_freeblock_num++;
    }

    /* 检查右侧相邻块是否空闲 */
    uint32_t right_freeblock_num = 0;
    uint32_t right_lastfree_idx = block_start_index + blocks_num;

    while (right_lastfree_idx < MEMPOOL_BLOCK_NUM &&
           MainPool.Mempool_Block_Info[right_lastfree_idx].Block_Info.Block_State == BLOCK_FREE)
    {
        right_lastfree_idx++;
        right_freeblock_num++;
    }

    if (left_freeblock_num > 0 || right_freeblock_num > 0) /* 有可合并的块 */
    {
        /* 从空闲链表中移除左侧相邻的空闲块 */
        for (uint32_t i = 0; i < left_freeblock_num; i++)
        {
            pool_block_t *curr = &MainPool.Mempool_Block_Info[block_start_index - i - 1];

            if (curr->Block_Next) curr->Block_Next->Block_Prev = curr->Block_Prev;

            if (curr->Block_Prev) curr->Block_Prev->Block_Next = curr->Block_Next;
            else if (MainPool.Mempool_FreeBlockHead == curr) MainPool.Mempool_FreeBlockHead = curr->Block_Next;

            curr->Block_Next = NULL;
            curr->Block_Prev = NULL;
        }

        /* 从空闲链表中移除右侧相邻的空闲块 */
        for (uint32_t i = 0; i < right_freeblock_num; i++)
        {
            pool_block_t *curr = &MainPool.Mempool_Block_Info[block_start_index + blocks_num + i];

            if (curr->Block_Next) curr->Block_Next->Block_Prev = curr->Block_Prev;

            if (curr->Block_Prev) curr->Block_Prev->Block_Next = curr->Block_Next;
            else if (MainPool.Mempool_FreeBlockHead == curr) MainPool.Mempool_FreeBlockHead = curr->Block_Next;

            curr->Block_Next = NULL;
            curr->Block_Prev = NULL;
        }

        /* 将待释放的块标记为空闲 */
        for (uint32_t i = 0; i < blocks_num; i++)
        {
            MainPool.Mempool_Block_Info[block_start_index + i].Block_Info.Block_State = BLOCK_FREE;
            MainPool.Mempool_Block_Info[block_start_index + i].Block_Next = NULL;
            MainPool.Mempool_Block_Info[block_start_index + i].Block_Prev = NULL;
        }

        /* 合并所有相邻的空闲块 */
        uint32_t merge_start = block_start_index - left_freeblock_num;
        uint32_t merge_count = left_freeblock_num + blocks_num + right_freeblock_num;

        pool_block_t *merge_block = &MainPool.Mempool_Block_Info[merge_start];
        merge_block->Block_Info.Blocks_Num = merge_count;

        /* 将合并后的大块添加到空闲链表头部 */
        merge_block->Block_Next = MainPool.Mempool_FreeBlockHead;
        merge_block->Block_Prev = NULL;

        if (MainPool.Mempool_FreeBlockHead) MainPool.Mempool_FreeBlockHead->Block_Prev = merge_block;

        MainPool.Mempool_FreeBlockHead = merge_block;
    }
    else /* 没有可以合并的块 */
    {
        /* 将每个块单独添加到空闲链表 */
        for (uint32_t i = 0; i < blocks_num; i++)
        {
            pool_block_t *curr = &MainPool.Mempool_Block_Info[block_start_index + i];
            curr->Block_Info.Block_State = BLOCK_FREE;
            curr->Block_Next = MainPool.Mempool_FreeBlockHead;
            curr->Block_Prev = NULL;
            if (MainPool.Mempool_FreeBlockHead) MainPool.Mempool_FreeBlockHead->Block_Prev = curr;
            MainPool.Mempool_FreeBlockHead = curr;
        }
    }

    /* 更新内存池状态 */
    MainPool.Mempool_Busy -= blocks_num;
    MainPool.Mempool_Free += blocks_num;
    return true;
}

/**
 * @brief 释放内存块
 * @param ptr 要释放的内存块指针
 * @return 是否成功释放
 * @retval true 释放成功
 * @retval false 释放失败
 */
bool MemPool_Free(void *ptr)
{
    /* 参数检查 */
    if (!ptr) return false;
    uintptr_t block_offset = ((uintptr_t)ptr - (uintptr_t)MainPool.Mempool_Total);

    /* 检查指针是否在内存池范围内且对齐到块大小 */
    if (block_offset >= MEMPOOL_TOTAL_SIZE || block_offset % MEMPOOL_BLOCK_SIZE != 0) return false;

    /* 计算块索引和块数 */
    uint32_t block_index = block_offset / MEMPOOL_BLOCK_SIZE;
    uint32_t blocks_num = MainPool.Mempool_Block_Info[block_index].Block_Info.Blocks_Num;

    /* 检查块范围是否有效 */
    if (block_index + blocks_num > MEMPOOL_BLOCK_NUM) return false;

    /* 执行释放操作 */
    bool res = __MemPool_Free(ptr, blocks_num, block_index);

    return res;
}

/* ========================= END OF FILE ========================= */