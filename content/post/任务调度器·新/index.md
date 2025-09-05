---
title: 任务调度器新版
description: 基于定时器、链表的任务调度器
date: 2025-09-05
lastmod: 2025-09-05
slug: 新任务调度器
image: unnamed.png
categories:
    - 嵌入式
tags:
    - 任务调度器
---
# 任务调度器系统 v3.0

## 📖 项目简介

本项目是一个轻量级的实时任务调度器系统，采用协作式多任务调度机制，支持优先级调度、动态内存管理和完善的错误处理。系统由任务调度器和内存池管理器两大核心模块组成。

## 👇🏻源码下载

- [Task.c](/code/Task_New/Task.c)
- [Task.h](/code/Task_New/Task.h)
- [Pool.c](/code/Task_New/Pool.c)
- [Pool.h](/code/Task_New/Pool.h)

### 🌟 核心特性

- **协作式调度**：任务主动让出CPU，避免抢占式调度的复杂性
- **优先级调度**：支持256级优先级（0-255，数值越小优先级越高）
- **双链表架构**：等待链表 + 运行链表，状态清晰分离
- **智能延时管理**：32位高低16位存储设计，节省内存空间
- **动态内存管理**：基于FreeRTOS heap4算法的内存池系统
- **完善错误处理**：分类错误代码，便于调试和定位问题
- **静态/动态任务**：支持两种任务创建方式

---

## 🎯 函数命名规范

### 命名约定说明

本系统采用统一的函数命名规范，**函数名的第一个字母代表其返回值类型**：

| 首字母 | 返回值类型             | 示例                                   |
| ------ | ---------------------- | -------------------------------------- |
| **r**  | `TaskRes_t` (枚举类型) | `rTaskInit()`, `rTaskCreate_Dynamic()` |
| **p**  | 指针类型               | `pTaskCreate_Static()`                 |
| **v**  | `void`                 | `vTaskStart()`                         |
| **u**  | `size_t` (unsigned)    | `uTaskGetFreeMemory()`                 |

> [!WARNING]
>
>任务调度器函数严格遵循返回值类型首字母规范。内存池模块(`MemPool_`)采用模块前缀命名，不遵循首字母规范

---

## 📚 API 参考文档

### 🔧 系统初始化

#### `TaskRes_t rTaskInit(void)`
**功能**：初始化任务调度器系统  
**返回值**：
- `TASK_OK`：初始化成功
- `TASK_ERROR_POOL_NOT_INIT`：内存池初始化失败
- `TASK_ERROR_INIT_FAILED`：任务系统初始化失败

**初始化流程**：
1. 初始化内存池系统
2. 初始化双链表结构（等待链表、运行链表）
3. 调用用户重定义的`TaskCreation()`函数添加任务

**使用示例**：
```c
// 用户需要重定义TaskCreation函数来添加任务
bool TaskCreation(void) {
    pTaskHandler_t task1, task2;
    
    // 添加LED任务
    if (rTaskCreate_Dynamic(led_blink_task, 1, &task1) != TASK_OK) {
        return false;
    }
    
    // 添加传感器任务
    if (rTaskCreate_Dynamic(sensor_task, 2, &task2) != TASK_OK) {
        return false;
    }
    
    return true;  // 所有任务添加成功
}

int main(void) {
    TaskRes_t result = rTaskInit();
    if (result != TASK_OK) {
        // 处理初始化错误
        printf("系统初始化失败，错误码：%d\n", result);
    }
}
```

**重要说明**：
- 系统采用**弱定义机制**，用户必须在自己的代码中重新定义`TaskCreation()`函数
- 所有任务的添加都应该放在`TaskCreation()`函数中完成
- 如果不重定义此函数，系统初始化会返回`TASK_ERROR_INIT_FAILED`

---

### 📝 任务管理

#### `TaskRes_t rTaskCreate_Dynamic(void (*pfunc)(void), uint8_t priority, pTaskHandler_t *task_handler)`
**功能**：动态添加一个任务（使用内存池分配节点）  
**参数**：
- `pfunc`：任务回调函数指针
- `priority`：任务优先级（0-255，越小优先级越高）
- `task_handler`：返回的任务句柄指针，可为NULL

**返回值**：
- `TASK_OK`：添加成功
- `TASK_ERROR_NULL_POINTER`：函数指针为空
- `TASK_ERROR_MEMORY_ALLOC`：内存分配失败

**使用示例**：
```c
pTaskHandler_t my_task;

void my_task_function(void) {
    printf("任务执行中...\n");
    rTaskDelay(1000);  // 延时1秒
}

TaskRes_t result = rTaskCreate_Dynamic(my_task_function, 1, &my_task);
```

#### `pTaskHandler_t pTaskCreate_Static(TaskNode_t *node, pTaskHandler_t *task_handler)`
**功能**：静态添加一个任务（用户提供节点内存）  
**参数**：
- `node`：用户提供的任务节点内存
- `task_handler`：任务句柄

**适用场景**：内存受限的嵌入式系统，避免动态内存分配

#### `TaskRes_t rTaskDelete(pTaskHandler_t task_handler)`
**功能**：删除一个任务并释放其内存  
**参数**：
- `task_handler`：要删除的任务句柄

**返回值**：
- `TASK_OK`：移除成功
- `TASK_ERROR_TASK_NOT_FOUND`：任务未找到

---

### ⏸️ 任务控制

#### `TaskRes_t rTaskSuspend(pTaskHandler_t task_handler)`
**功能**：挂起任务  
**参数**：
- `task_handler`：任务句柄，**如果为NULL则挂起当前任务**

**特色功能**：支持任务自我挂起
```c
void my_task(void) {
    // 执行一些工作
    if (error_condition) {
        rTaskSuspend(NULL);  // 挂起自己
        return;
    }
}
```

#### `TaskRes_t rTaskResume(pTaskHandler_t task_handler)`
**功能**：恢复任务  
**参数**：
- `task_handler`：任务句柄，**如果为NULL则恢复当前任务**

#### `TaskRes_t rTaskDelay(uint16_t delay_ms)`
**功能**：设置当前任务的延时时间  
**参数**：
- `delay_ms`：延时时间（毫秒，最大65535ms）

**设计特点**：
- 仅对当前正在执行的任务有效
- 类似操作系统的sleep()函数
- 任务主动让出CPU，实现协作式调度

#### `TaskRes_t rTaskSetPriority(pTaskHandler_t task_handler, uint8_t priority)`
**功能**：设置任务优先级  
**参数**：
- `task_handler`：任务句柄，**如果为NULL则设置当前任务**
- `priority`：新的优先级值

#### `TaskRes_t rTaskGetInfo(pTaskHandler_t task_handler, TaskInfo_t *task_info)`
**功能**：获取任务详细信息  
**参数**：
- `task_handler`：任务句柄，**如果为NULL则获取当前任务信息**
- `task_info`：任务信息结构体指针

**返回值**：
- `TASK_OK`：获取成功
- `TASK_ERROR_NULL_POINTER`：参数为空

#### `size_t uTaskGetFreeMemory(void)`
**功能**：获取内存池剩余可用内存大小  
**返回值**：剩余内存字节数

---

### 🚀 调度器启动

#### `void vTaskStart(uint32_t (*tick_get)(void))`
**功能**：启动任务调度器主循环  
**参数**：
- `tick_get`：获取系统时钟tick的函数指针

**调度流程**：
1. 遍历等待链表，递减倒计时
2. 将倒计时为0的任务移至运行链表
3. 按优先级执行运行链表中的任务
4. 记录任务执行时间统计
5. 执行完成后重置倒计时并移回等待链表

**使用示例**：
```c
// STM32 HAL示例
vTaskStart(HAL_GetTick);

// 裸机示例
uint32_t get_system_tick(void) {
    return system_tick_counter;
}
vTaskStart(get_system_tick);
```

---

## 🧠 内存池管理系统

### 🏗️ 架构设计

内存池管理器采用**FreeRTOS heap4算法**的设计思路，具备以下特性：

- **首次适应算法**：快速查找合适大小的空闲块
- **块分割与合并**：自动处理内存碎片
- **地址排序链表**：空闲块按地址顺序排列，便于合并
- **内存对齐**：支持4/8字节对齐，提高访问效率
- **安全保护**：完整的边界检查和重复释放检测

### 📊 数据结构

```c
typedef struct mem_block {
    struct mem_block *next_free;  // 指向下一个空闲块
    size_t block_size;           // 块大小，最高位用作分配标记
} mem_block_t;

typedef struct {
    size_t total_size;      // 内存池总大小
    size_t free_bytes;      // 当前可用字节数
    size_t min_free_bytes;  // 历史最小可用字节数
    size_t alloc_count;     // 分配次数统计
    size_t free_count;      // 释放次数统计
} pool_stats_t;
```

### 🔧 核心算法

#### 内存分配流程
1. **参数验证**：检查size是否有效
2. **大小计算**：添加头部信息并进行内存对齐
3. **空闲块查找**：使用首次适应算法遍历空闲链表
4. **块分割**：如果找到的块过大，分割成所需大小和剩余部分
5. **标记分配**：设置最高位标记块已分配
6. **统计更新**：更新内存使用统计信息

#### 内存释放流程
1. **指针验证**：检查指针是否在有效范围内
2. **分配检查**：验证块确实已分配，防止重复释放
3. **邻块合并**：检查前后相邻块并自动合并
4. **插入链表**：将合并后的空闲块按地址顺序插入链表

### 📋 内存池API

#### `bool MemPool_Init(void)`
**功能**：初始化内存池系统  
**返回值**：true表示成功，false表示失败  
**说明**：内存池函数采用模块前缀命名，不遵循首字母返回值类型规范

#### `void *MemPool_Malloc(size_t size)`
**功能**：分配指定大小的内存块  
**参数**：size - 需要分配的字节数  
**返回值**：成功返回内存指针，失败返回NULL

#### `bool MemPool_Free(void *ptr)`
**功能**：释放内存块  
**参数**：ptr - 要释放的内存指针  
**返回值**：true表示成功，false表示失败

#### `size_t MemPool_GetFreeSize(void)`
**功能**：获取剩余可用内存大小  
**返回值**：可用字节数

#### `void MemPool_GetStats(pool_stats_t *stats)`
**功能**：获取内存池统计信息  
**参数**：stats - 统计信息结构体指针

#### `bool MemPool_CheckIntegrity(void)`
**功能**：检查内存池完整性  
**返回值**：true表示完整，false表示检测到损坏

---

## 🎨 核心设计特性

### 💡 智能时间管理

采用**32位高低16位分离存储**设计：
- **高16位**：存储设定的延时时间值
- **低16位**：存储当前倒计时值

```c
// 时间操作宏
#define TASK_SET_TRIG_TIME(set_val, cur_val)  (((uint32_t)(set_val) << 16) | ((uint32_t)(cur_val) & 0xFFFF))
#define TASK_GET_SET_TIME(trig_time)          ((uint16_t)((trig_time) >> 16))
#define TASK_GET_CUR_TIME(trig_time)          ((uint16_t)((trig_time) & 0xFFFF))
#define TASK_RESET_TIME(trig_time)            (((trig_time) & 0xFFFF0000) | (((trig_time) >> 16) & 0xFFFF))
```

**优势**：
- 节省内存空间（1个32位变量 vs 2个16位变量）
- 原子操作，避免数据不一致
- 支持最大65535ms的延时设置

### 🔄 双链表调度架构

- **等待链表（Wait_Schedule）**：存放等待执行的任务
- **运行链表（Run_Schedule）**：存放就绪待执行的任务

**调度流程**：
```
[等待链表] --倒计时到0--> [运行链表] --执行完成--> [等待链表]
```

### 🚨 分类错误处理系统

```c
typedef enum {
    TASK_ERR_TICK_NULL = 1,     // tick获取函数为空
    TASK_ERR_WAIT_TO_RUN,       // 等待链表到运行链表移动失败
    TASK_ERR_RUN_TO_WAIT,       // 运行链表到等待链表移动失败
    TASK_ERR_MEMORY_CORRUPT,    // 内存损坏
    TASK_ERR_LIST_CORRUPT       // 链表结构损坏
} TaskErrorCode_t;
```

每种错误都有独特的死循环，便于调试时通过断点快速定位问题类型。

---

## 📊 使用示例

### 🚀 完整示例程序

```c
#include "Task.h"
#include <stdio.h>

// 任务句柄
pTaskHandler_t led_task, sensor_task, display_task;

// 用户重定义TaskCreation函数 - 在这里添加所有任务
bool TaskCreation(void) {
    TaskRes_t result;
    
    // 添加LED任务
    result = rTaskCreate_Dynamic(led_blink_task, 1, &led_task);
    if (result != TASK_OK) {
        printf("✗ LED任务添加失败\n");
        return false;
    }
    printf("✓ LED任务添加成功\n");
    
    // 添加传感器任务
    result = rTaskCreate_Dynamic(sensor_read_task, 2, &sensor_task);
    if (result != TASK_OK) {
        printf("✗ 传感器任务添加失败\n");
        return false;
    }
    printf("✓ 传感器任务添加成功\n");
    
    // 添加显示任务
    result = rTaskCreate_Dynamic(display_task, 3, &display_task);
    if (result != TASK_OK) {
        printf("✗ 显示任务添加失败\n");
        return false;
    }
    printf("✓ 显示任务添加成功\n");
    
    return true;  // 所有任务添加成功
}

// LED闪烁任务
void led_blink_task(void) {
    static bool led_state = false;
    
    led_state = !led_state;
    printf("LED: %s\n", led_state ? "ON" : "OFF");
    
    // 根据状态设置不同延时
    if (led_state) {
        rTaskDelay(100);   // 亮100ms
    } else {
        rTaskDelay(900);   // 暗900ms
    }
}

// 传感器读取任务
void sensor_read_task(void) {
    static int count = 0;
    
    int sensor_value = rand() % 100;  // 模拟传感器读取
    printf("传感器读取: %d (第%d次)\n", sensor_value, ++count);
    
    // 根据读取值动态调整采样频率
    if (sensor_value > 80) {
        rTaskDelay(50);    // 高值时高频采样
    } else if (sensor_value > 40) {
        rTaskDelay(200);   // 中等值时中频采样
    } else {
        rTaskDelay(500);   // 低值时低频采样
    }
}

// 显示任务
void display_task(void) {
    static uint32_t update_count = 0;
    
    printf("===== 系统状态更新 #%lu =====\n", ++update_count);
    
    // 获取内存使用情况
    pool_stats_t stats;
    MemPool_GetStats(&stats);
    printf("内存使用: %zu/%zu 字节\n", 
           stats.total_size - stats.free_bytes, stats.total_size);
    
    // 每10次更新后降低自己的优先级
    if (update_count % 10 == 0) {
        printf("降低显示任务优先级\n");
        rTaskSetPriority(NULL, 10);  // 对自己降低优先级
    }
    
    rTaskDelay(2000);  // 2秒更新一次
}

// 系统时钟函数（模拟）
static uint32_t system_tick = 0;
uint32_t get_system_tick(void) {
    return system_tick++;
}

int main(void) {
    printf("=== 任务调度器系统 v3.0 示例 ===\n");
    
    // 1. 初始化系统（会自动调用TaskCreation函数添加任务）
    TaskRes_t result = rTaskInit();
    if (result != TASK_OK) {
        printf("系统初始化失败！错误码: %d\n", result);
        return -1;
    }
    printf("✓ 系统初始化成功，所有任务已添加\n");
    
    // 2. 启动调度器
    printf("启动任务调度器...\n");
    vTaskStart(get_system_tick);
    
    return 0;  // 永远不会执行到这里
}
```

### 🔧 高级使用技巧

#### 任务自管理
```c
void smart_task(void) {
    static int error_count = 0;
    
    if (do_some_work() == ERROR) {
        error_count++;
        
        if (error_count > 5) {
            printf("错误过多，挂起自己\n");
            rTaskSuspend(NULL);  // 挂起自己
            return;
        }
        
        // 出错时降低执行频率
        rTaskDelay(1000);
    } else {
        error_count = 0;  // 重置错误计数
        rTaskDelay(100);   // 正常执行频率
    }
}
```

#### 动态优先级调整
```c
void adaptive_task(void) {
    static int workload = 0;
    
    workload = calculate_workload();
    
    if (workload > 80) {
        rTaskSetPriority(NULL, 1);  // 提升优先级
        rTaskDelay(50);              // 高频执行
    } else {
        rTaskSetPriority(NULL, 5);  // 降低优先级
        rTaskDelay(200);             // 低频执行
    }
}
```

---

## 📈 新旧版本对比

### 📋 老版本 v2.0 特性回顾

老版本任务调度器采用了以下设计特点：

#### 🗂️ **数据结构**
- **固定数组**：`task_t Task_Schedule[MAXTASKS]` 静态分配
- **任务结构**：包含任务名称、函数指针、参数、优先级、时间间隔等
- **时间管理**：`LastWakeUp` + `Interval` 分离存储方式

#### 🔄 **调度机制**
- **时间轮询**：每次调度遍历整个任务数组
- **qsort排序**：每次添加任务后重新排序
- **优先级+创建顺序**：相同优先级按创建顺序执行

#### 🎛️ **任务控制**
- **字符串查找**：通过任务名称字符串查找和操作任务
- **状态管理**：ACTIVE/SUSPEND/INACTIVE 三种状态
- **时间统计**：记录每个任务的最大执行时间

#### ⚡ **性能特性**
- **动态调整**：支持根据任务执行时间动态调整调度间隔
- **低功耗**：支持系统空闲时进入低功耗模式
- **空闲回调**：提供空闲时执行的回调函数

---

### 🆚 架构对比

| 特性         | 老版本 v2.0                        | 新版本 v3.0                 |
| ------------ | ---------------------------------- | --------------------------- |
| **调度方式** | 时间间隔轮询 + qsort排序           | 双链表优先级调度            |
| **内存管理** | 静态数组 `Task_Schedule[MAXTASKS]` | 动态内存池 + 静态选项       |
| **时间管理** | `LastWakeUp` + `Interval` 分离存储 | 32位高低位智能存储          |
| **数据结构** | 固定数组结构                       | 双向链表结构                |
| **任务限制** | 最大任务数限制 `MAXTASKS`          | 仅受内存限制                |
| **错误处理** | 简单bool返回值                     | 分类错误代码 + 调试支持     |
| **任务控制** | 暂停/恢复/删除                     | 挂起/恢复/删除 + 优先级调整 |
| **API设计**  | 字符串任务名查找                   | 句柄直接操作                |

### 🚀 性能提升

#### 内存使用优化
- **老版本**：固定数组 `task_t Task_Schedule[MAXTASKS]`，每个任务包含完整结构体
- **新版本**：链表节点动态分配，仅在需要时占用内存

#### 时间管理优化
- **老版本**：`LastWakeUp` + `Interval` 两个32位变量（8字节）
- **新版本**：一个32位变量高低16位存储（4字节），节省50%空间

#### 调度效率提升
- **老版本**：每次调度都要遍历完整数组 + qsort排序，O(n log n)复杂度
- **新版本**：双链表分离就绪/等待任务，避免重复排序，平均复杂度更低

#### 任务查找优化
- **老版本**：字符串比较查找任务，O(n)时间复杂度
- **新版本**：句柄直接访问，O(1)时间复杂度

#### API易用性对比
```c
// 老版本 - 添加任务需要多个参数
Task_Add("LED_Task", led_function, 1000, NULL, 1);

// 新版本 - 更简洁的任务创建
rTaskCreate_Dynamic(led_function, 1, &led_task);
```

```c
// 老版本 - 字符串查找任务
Task_Suspend("LED_Task");

// 新版本 - 句柄直接操作
rTaskSuspend(led_task);
```

```c
// 老版本 - 无内置延时功能，需要在任务内部实现
void task_function(void* param) {
    // 需要自己管理时间间隔
    static uint32_t last_time = 0;
    uint32_t now = HAL_GetTick();
    if (now - last_time < 1000) return;
    last_time = now;
    // 执行任务逻辑...
}

// 新版本 - 简洁的延时设置
void task_function(void) {
    // 执行任务逻辑...
    rTaskDelay(1000);  // 直接设置延时
}
```

### 🛡️ 安全性增强

#### 内存安全
- **老版本**：固定数组边界，可能出现数组越界
- **新版本**：动态内存管理，完整的内存边界检查，防止重复释放和野指针访问

#### 错误诊断
- **老版本**：简单的bool返回值，难以定位具体错误原因
- **新版本**：详细的错误枚举类型，不同错误进入不同死循环，便于调试

#### 任务管理安全性
- **老版本**：任务删除仅标记为INACTIVE，内存无法回收
- **新版本**：真正的任务删除，释放占用的内存资源

### 📊 功能对比表

| 功能模块     | 老版本 v2.0             | 新版本 v3.0         | 提升说明             |
| ------------ | ----------------------- | ------------------- | -------------------- |
| **任务创建** | ✅ `Task_Add()`          | ✅ 静态+动态双模式   | 支持两种内存分配方式 |
| **任务删除** | ⚠️ 标记删除(INACTIVE)    | ✅ 真正删除+内存释放 | 可释放内存资源       |
| **任务暂停** | ✅ `Task_Suspend()`      | ✅ `rTaskSuspend()`  | 支持NULL参数(自操作) |
| **任务恢复** | ✅ `Task_Resume()`       | ✅ `rTaskResume()`   | 支持NULL参数(自操作) |
| **优先级**   | ✅ 创建时设定            | ✅ 动态调整优先级    | 运行时可修改         |
| **任务查找** | 🔤 字符串名称查找        | 🎯 句柄直接访问      | 性能提升显著         |
| **内存限制** | ❌ 固定数组大小限制      | ✅ 仅受系统内存限制  | 灵活性大幅提升       |
| **时间统计** | ✅ `MaxUsed` 时间统计    | ✅ 增强的时间统计    | 更精确的性能监控     |
| **空闲处理** | ✅ `Task_IdleFunction()` | ❌ 移除空闲函数      | 简化调度逻辑         |
| **低功耗**   | ✅ 支持低功耗模式        | ❌ 暂未实现          | 后续版本考虑         |
| **动态调整** | ✅ 任务间隔动态调整      | ❌ 采用固定调度      | 简化复杂度           |
| **内存池**   | ❌ 无                    | ✅ 完整内存池实现    | 动态内存管理         |
| **错误处理** | ⚠️ bool返回值            | ✅ 完善的错误分类    | 调试支持大幅增强     |

---

## ⚙️ 系统配置与弱定义函数

### 🔧 弱定义机制说明

新版本调度器采用**弱定义（Weak Definition）**机制，提供了两个用户可重定义的函数：

#### `bool TaskCreation(void)` - 任务创建函数
**功能**：用户必须重定义此函数来添加系统所需的所有任务  
**调用时机**：在`rTaskInit()`初始化过程中自动调用  
**返回值**：
- `true`：所有任务创建成功
- `false`：任务创建失败，导致系统初始化失败

**默认实现**：
```c
__weak bool TaskCreation(void) {
    return false;  // 默认返回失败，用户必须重定义
}
```

**用户实现示例**：
```c
// 用户在自己的代码中重新定义TaskCreation函数
bool TaskCreation(void) {
    pTaskHandler_t task1, task2, task3;
    
    // 创建LED控制任务
    if (rTaskCreate_Dynamic(led_control_task, 1, &task1) != TASK_OK) {
        return false;
    }
    
    // 创建数据采集任务
    if (rTaskCreate_Dynamic(data_collect_task, 2, &task2) != TASK_OK) {
        return false;
    }
    
    // 创建通信任务
    if (rTaskCreate_Dynamic(communication_task, 3, &task3) != TASK_OK) {
        return false;
    }
    
    return true;  // 所有任务创建成功
}
```

#### `void TaskIdle(void)` - 空闲函数
**功能**：系统空闲时执行的函数，用户可选择性重定义  
**调用时机**：当没有就绪任务时调用  
**注意事项**：不要在此函数中执行耗时操作！

**默认实现**：
```c
__weak void TaskIdle(void) {
    // 默认为空函数
}
```

**用户实现示例**：
```c
// 用户可选择重新定义空闲函数
void TaskIdle(void) {
    // 进入低功耗模式
    __WFI();  // ARM Cortex-M系列的等待中断指令
    
    // 或者执行一些轻量级的后台任务
    // watchdog_feed();  // 喂狗操作
}
```

### 📝 使用流程

1. **编写任务函数**：定义各个任务的具体实现
2. **重定义TaskCreation**：在此函数中添加所有需要的任务
3. **调用rTaskInit**：系统会自动调用您的TaskCreation函数
4. **启动调度器**：调用vTaskStart开始任务调度

```c
// 步骤1: 编写任务函数
void my_task1(void) {
    // 任务逻辑
    rTaskDelay(1000);
}

void my_task2(void) {
    // 任务逻辑
    rTaskDelay(500);
}

// 步骤2: 重定义TaskCreation
bool TaskCreation(void) {
    pTaskHandler_t h1, h2;
    
    if (rTaskCreate_Dynamic(my_task1, 1, &h1) != TASK_OK) return false;
    if (rTaskCreate_Dynamic(my_task2, 2, &h2) != TASK_OK) return false;
    
    return true;
}

// 步骤3&4: 主函数
int main(void) {
    if (rTaskInit() != TASK_OK) {  // 自动调用TaskCreation
        printf("初始化失败\n");
        return -1;
    }
    
    vTaskStart(HAL_GetTick);  // 启动调度器
    return 0;
}
```

---

## ⚙️ 配置选项

### 内存池配置
```c
// Pool.h 中的配置宏
#define MEMPOOL_SIZE (4096)        // 内存池大小
#define MEMPOOL_ALIGNMENT (8)      // 内存对齐字节数
```

### 编译器支持
- **GCC/Clang**：完全支持
- **MSVC**：支持
- **IAR**：支持
- **Keil ARM**：支持

### 平台兼容性
- **STM32系列**：完全兼容
- **Arduino**：兼容
- **ESP32**：兼容
- **X86仿真**：兼容

---

## 🐛 调试指南

### 错误代码速查

| 错误代码                  | 含义         | 可能原因       | 解决方案             |
| ------------------------- | ------------ | -------------- | -------------------- |
| `TASK_ERR_TICK_NULL`      | tick函数为空 | 未传入时钟函数 | 检查vTaskStart()参数 |
| `TASK_ERR_WAIT_TO_RUN`    | 链表移动失败 | 链表结构损坏   | 检查内存完整性       |
| `TASK_ERR_RUN_TO_WAIT`    | 链表移动失败 | 链表结构损坏   | 检查内存完整性       |
| `TASK_ERR_MEMORY_CORRUPT` | 内存损坏     | 越界访问       | 使用内存检测工具     |
| `TASK_ERR_LIST_CORRUPT`   | 链表损坏     | 指针错误       | 检查任务节点操作     |

### 调试技巧

1. **断点调试**：在不同错误的死循环处设置断点
2. **统计监控**：定期调用`MemPool_GetStats()`监控内存使用
3. **完整性检查**：定期调用`MemPool_CheckIntegrity()`检查内存池
4. **日志输出**：在任务中添加状态输出

### 常见问题

**Q: 系统初始化失败，返回TASK_ERROR_INIT_FAILED？**
A: 检查是否重定义了`TaskCreation()`函数，且该函数返回true。如果没有重定义此函数，系统会调用默认的弱定义版本（返回false）。

**Q: 任务不执行怎么办？**
A: 检查是否调用了`rTaskDelay()`设置延时，新添加的任务默认延时为0。

**Q: 可以在TaskCreation之外添加任务吗？**
A: 不建议。虽然技术上可行，但为了保持代码结构清晰，建议所有任务都在`TaskCreation()`中添加。

**Q: 内存分配失败？**
A: 检查内存池大小设置，或存在内存泄漏。

**Q: 系统卡死？**
A: 可能进入了错误处理死循环，检查错误代码定位问题。

---

## 📝 版本历史

### v3.0 (2025-09-05)
- ✨ 全新双链表调度架构，替代数组+排序方案
- 🚀 FreeRTOS heap4风格内存池系统
- 💡 32位高低16位智能时间存储
- 🛡️ 完善的错误处理系统，支持调试定位
- 📊 详细的统计和诊断功能
- 🎯 任务自操作支持（NULL参数）
- 🔧 句柄式API设计，替代字符串查找
- ⚡ 性能优化：O(1)任务访问，避免排序开销

### v2.0 (2025-08-17) - 老版本
- ✅ 基于固定数组的任务调度表
- ✅ qsort优先级排序机制
- ✅ 字符串任务名称系统
- ✅ 任务暂停/恢复/删除功能
- ✅ 任务最大用时统计
- ✅ 低功耗模式支持
- ✅ 动态时间间隔调整
- ✅ 空闲函数回调机制
- ⚠️ 固定最大任务数限制
- ⚠️ 简单bool返回值错误处理

### v1.x (历史版本)
- 基础的任务轮询机制

---

## 📄 许可证

本项目采用MIT许可证，详情请查看LICENSE文件。

---

**享受使用任务调度器系统 v3.0！** 🎉
