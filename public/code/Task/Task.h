/**
 * @file    Task.h
 * @brief   任务调度管理模块头文件
 * @details 定义了任务调度相关的数据结构和函数接口
 *          包括任务添加、任务启动、任务暂停等基本功能
 * @author  N1ntyNine99
 * @date    2025-04-17
 * @version v1.0
 */

#ifndef __TASK_h
#define __TASK_h

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================= 宏定义 ========================= */
#ifndef __weak
#if defined(__GNUC__) || defined(__clang__)
#define __weak __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define __weak __weak
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define __weak __weak
#else
#define __weak
#endif
#endif
/**
 * @brief   是否开启动态调整
 */
#define DYNAMIC_MODIFY 0

/**
 * @brief   是否开启低功耗
 */
#define LOW_POWER 0

/**
 * @brief   最大任务数量
 * @note    可根据系统资源调整
 */
#define MAXTASKS 20

/**
 * @brief   最小休眠时间(ms)
 * @note    低于此时间不进入休眠模式
 */
#define MIN_SLEEP_TICK 10

/**
 * @brief   任务调整力度(ms)
 */
#define TASK_ADJUST 5

/* ========================= 类型定义 ========================= */
/**
 * @brief   任务函数指针类型
 */
typedef void (*taskFunction_t)(void *para);

/**
 * @brief   获取系统心跳的函数指针类型
 */
typedef uint32_t (*systick_get)(void);

/**
 * @brief   任务状态枚举
 */
typedef enum
{
    ACTIVE = 0, /**< 活动状态，任务正常执行 */
    SUSPEND, /**< 暂停状态，任务暂停执行 */
    INACTIVE, /**< 非活动状态，任务已删除 */
} task_ea_t;

/**
 * @brief 任务调度器状态枚举
 */
typedef enum
{
    SCHEDULE_IDLE = 0,
    SCHEDULE_BUSY
} task_schedule_t;

/**
 * @brief   任务结构体定义
 */
typedef struct
{
    const char *Name; /**< 任务名称 */
    uint32_t LastWakeUp; /**< 上一次唤醒时间 */
    uint16_t MaxUsed; /**< 最大用时 */
    uint16_t Interval; /**< 任务执行间隔(ms) */
    uint8_t Priority; /**< 任务优先级，数值越小优先级越高 */
    task_ea_t State; /**< 任务状态 */
    uint8_t CreationIndex; /**< 任务创建顺序 */
    void *Para; /**< 任务执行函数入参 */
    taskFunction_t Function; /**< 任务执行函数 */
} task_t;

/* ========================= 函数声明 ========================= */
bool Task_Add(const char *taskName, taskFunction_t pfunc, uint16_t time_ms, void *para, uint8_t priority);
void Task_Start(systick_get func);
uint8_t Task_CheckNum(void);
bool Task_Suspend(const char *taskName);
bool Task_Resume(const char *taskName);
bool Task_Delete(const char *taskName);
bool Task_GetMaxUsed(const char *taskName, uint16_t *time_ms);

#ifdef __cplusplus
}
#endif

#endif /* __TASK_h */
