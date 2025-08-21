/**
 * @file    Task.c
 * @brief   任务调度管理模块实现
 * @details 实现了一个简单的任务调度器
 *          支持多个任务的定时调度执行
 * @author  N1ntyNine99
 * @date    2025-08-17
 * @version v2.0
 */

#include "Task.h"

/* ========================= 私有变量 ========================= */
static uint8_t Task_Num = 0; //当前任务数量
static task_t Task_Schedule[MAXTASKS]; //任务调度表

#if DYNAMIC_MODIFY
static uint16_t Task_Min_Interval = 0; //调度表中最小的任务时间间隔
#endif
/* ========================= 空闲函数 ========================= */

/**
 * @brief 空闲函数
 * 
 * @note 在调度器空闲的时候会调用的函数
 * 
 * @warning 不要尝试在该函数中进行一些耗费时间的任务!!!
 */
__weak void Task_IdleFunction(void)
{
}

/* ========================= 私有函数 ========================= */
/**
 * @brief   任务比较函数(用于排序)
 * @param   task1 第一个任务指针
 * @param   task2 第二个任务指针
 * @retval  比较结果
 * @note    用于按照优先级对任务进行排序
 */
static int Task_CMP(const void *task1, const void *task2)
{
    task_t *p1 = (task_t *)task1;
    task_t *p2 = (task_t *)task2;

    if (p1->Priority != p2->Priority)
    {
        return p1->Priority - p2->Priority;
    }
    else
    {
        return p1->CreationIndex - p2->CreationIndex;
    }
}

/* ========================= 任务管理函数 ========================= */
/**
 * @brief  添加一个任务到调度器
 * @param  taskName 任务名称
 * @param  pfunc 任务函数指针
 * @param  time_ms 任务执行间隔时间(毫秒)
 * @param  priority 任务优先级
 * @retval 返回函数是否执行成功
 * @note   优先级数值越小，优先级越高
 */
bool Task_Add(const char *taskName, taskFunction_t pfunc, uint16_t time_ms, void *para, uint8_t priority)
{
    if (Task_Num >= MAXTASKS || pfunc == NULL) return false;

    Task_Schedule[Task_Num].Name = taskName;
    Task_Schedule[Task_Num].LastWakeUp = 0;
    Task_Schedule[Task_Num].Interval = time_ms;
    Task_Schedule[Task_Num].State = ACTIVE;
    Task_Schedule[Task_Num].Priority = priority;
    Task_Schedule[Task_Num].CreationIndex = Task_Num;
    Task_Schedule[Task_Num].Function = pfunc;
    Task_Schedule[Task_Num].Para = para;
    Task_Schedule[Task_Num].MaxUsed = 0;

#if DYNAMIC_MODIFY
    Task_Min_Interval = (Task_Min_Interval > time_ms) ? time_ms : Task_Min_Interval;
#endif

    Task_Num++;

    qsort((void *)Task_Schedule, Task_Num, sizeof(task_t), Task_CMP); //按照优先级排序
    return true;
}

/**
 * @brief  启动任务调度
 * @param  func 传入获得时钟心跳的函数的指针
 * @note   此函数应在主循环中周期性调用
 */
void Task_Start(systick_get func)
{
    uint32_t NowTick = func();
    uint32_t Tick_Next_Task = UINT32_MAX; //下一个要执行的任务的定时器值
    task_schedule_t All_Scheduler_IDLE = SCHEDULE_IDLE;

    for (uint8_t i = 0; i < Task_Num; i++)
    {
        //任务应该被调用
        if (NowTick - Task_Schedule[i].LastWakeUp >= Task_Schedule[i].Interval)
        {
            Task_Schedule[i].LastWakeUp = NowTick;
            All_Scheduler_IDLE = SCHEDULE_BUSY;
            //计算任务最大用时
            uint32_t StartTick = func();
            Task_Schedule[i].Function(Task_Schedule[i].Para);
            uint32_t EndTick = func();
            uint16_t TickDiff; //时间差值
            //记录该任务的最大用时
            if (EndTick >= StartTick) //时钟心跳没有溢出
            {
                TickDiff = EndTick - StartTick;
            }
            else //时钟心跳溢出
            {
                TickDiff = (UINT32_MAX - StartTick) + EndTick + 1;
            }
            TickDiff = TickDiff > UINT16_MAX ? UINT16_MAX : TickDiff; //限幅
            if (TickDiff > Task_Schedule[i].MaxUsed) Task_Schedule[i].MaxUsed = TickDiff;
        }
        else //任务不该被调用
        {
            // 计算下次执行本任务前的剩余时间
            uint32_t TimeToNext = 0;

            // 安全计算剩余时间，避免溢出
            if (NowTick >= Task_Schedule[i].LastWakeUp)
            {
                uint32_t Diff_Time = NowTick - Task_Schedule[i].LastWakeUp; //计算当前的时间到上一次调用时间的差值
                if (Diff_Time < Task_Schedule[i].Interval)
                {
                    //如果差值小于任务间隔说明还没调用，则下一个任务的时间就是间隔减去差值
                    TimeToNext = Task_Schedule[i].Interval - Diff_Time;
                }
                else // 已经到期，但可能因为状态不是ACTIVE而未执行
                {
                    TimeToNext = 0;
                }
            }
            else
            {
                // 处理HAL_GetTick()溢出的情况
                // (UINT32_MAX - Task_Schedule[i].LastWakeUp) + NowTick + 1:
                // 先计算32位无符号整型的最大值距离上一次唤醒的时间
                // 加上当前NowTick，由于已经溢出，这就代表当前时间理论和上一次任务的时间之差
                // +1是因为计数是从0开始的
                // 然后用任务间隔减去这个值就算出来距离下一次任务的时间
                TimeToNext = Task_Schedule[i].Interval - ((UINT32_MAX - Task_Schedule[i].LastWakeUp) + NowTick + 1);
            }
            // 更新下一个执行任务的时间
            if (TimeToNext < Tick_Next_Task)
            {
                Tick_Next_Task = TimeToNext;
            }
            //执行空闲任务
            Task_IdleFunction();
        }
#if DYNAMIC_MODIFY
        //如果有任务的最大耗时超过了调度器列表中最小的任务时间间隔
        if (Task_Schedule[i].MaxUsed > Task_Min_Interval)
        {
            //动态调整时间间隔
            Task_Schedule[i].Interval += TASK_ADJUST;
        }
#endif
    }

#if LOW_POWER
    if (Tick_Next_Task > MIN_SLEEP_TICK && All_Scheduler_IDLE)
    {
        //低功耗实现
    }
#endif
}

/**
 * @brief  获取当前任务数量
 * @param  无
 * @retval 当前任务数量
 */
uint8_t Task_CheckNum(void)
{
    return Task_Num;
}

/**
 * @brief  暂停一个任务
 * @param  taskName 任务名称
 * @retval 返回函数是否执行成功
 * @note   被暂停的任务不会执行，但仍然在任务列表中
 */
bool Task_Suspend(const char *taskName)
{
    for (uint8_t i = 0; i < Task_Num; i++)
    {
        if (Task_Schedule[i].Name == taskName)
        {
            Task_Schedule[i].State = SUSPEND;
            return true;
        }
    }
    return false;
}

/**
 * @brief  恢复一个任务
 * @param  taskName 任务名称
 * @retval 返回函数是否执行成功
 * @note   恢复被暂停的任务
 */
bool Task_Resume(const char *taskName)
{
    for (uint8_t i = 0; i < Task_Num; i++)
    {
        if (Task_Schedule[i].Name == taskName && Task_Schedule[i].State == SUSPEND)
        {
            Task_Schedule[i].State = ACTIVE;
            return true;
        }
    }
    return false;
}

/**
 * @brief  删除一个任务
 * @param  taskName 任务名称
 * @retval 返回函数是否执行成功
 * @note   将任务标记为非活动状态
 */
bool Task_Delete(const char *taskName)
{
    for (uint8_t i = 0; i < Task_Num; i++)
    {
        if (Task_Schedule[i].Name == taskName)
        {
            Task_Schedule[i].State = INACTIVE;
            return true;
        }
    }
    return false;
}

/**
 * @brief 获取指定任务的最大消耗时间(ms)
 * @param taskName 任务名称
 * @param time_ms 存储最大用时的变量
 * @retval 返回函数是否执行成功
 */
bool Task_GetMaxUsed(const char *taskName, uint16_t *time_ms)
{
    for (uint8_t i = 0; i < Task_Num; i++)
    {
        if (Task_Schedule[i].Name == taskName)
        {
            *time_ms = Task_Schedule[i].MaxUsed;
            return true;
        }
    }
    return false;
}