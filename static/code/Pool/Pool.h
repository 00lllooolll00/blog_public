/**
 * @file Pool.h
 * @brief 内存池管理模块
 * @details 实现固定大小块的内存池管理，支持动态分配和释放内存
 *          内存池使用双向链表管理空闲块，支持内存碎片合并
 * @author N1netyNine99
 * @date 2025-05-07
 */

#ifndef __Pool_h
#define __Pool_h

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================= 宏定义区 ========================= */
/**
 * @brief 使用默认参数初始化内存池，块大小64字节，块数量1024
 */
#define MemPool_Init_Default() MemPool_Init(64, 1024)

/* ========================= 类型定义区 ========================= */
/**
 * @brief 内存块状态枚举
 */
typedef enum
{
    BLOCK_FREE = 0, ///< 空闲状态
    BLOCK_BUSY = 1 ///< 占用状态
} block_state_t;

/**
 * @brief 内存块结构体，包含链表指针和状态信息
 */
typedef struct pool_block_t
{
    struct pool_block_t *Block_Prev; ///< 前一个块指针
    struct pool_block_t *Block_Next; ///< 后一个块指针

    struct
    {
        block_state_t Block_State:1; ///< 块状态，0表示空闲，1表示占用
        uint16_t Blocks_Num:15; ///< 连续块数量
    } Block_Info;
} pool_block_t;

/**
 * @brief 内存池管理结构体
 */
typedef struct
{
    pool_block_t *Mempool_FreeBlockHead; ///< 空闲块链表头指针
    uint32_t Mempool_Busy; ///< 已占用块数量
    uint32_t Mempool_Free; ///< 空闲块数量
    uint8_t *Mempool_Total; ///< 内存池总地址指针
    pool_block_t *Mempool_Block_Info; ///< 块信息数组指针
} mempool_t;

/* ========================= 函数声明区 ========================= */
bool MemPool_Init(uint32_t block_size, uint32_t block_num);
void MemPool_Deinit(void);
void *MemPool_Malloc(size_t size);
bool MemPool_Free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
