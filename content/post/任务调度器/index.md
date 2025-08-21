---
title: 任务调度器
description: 基于定时器的任务调度器
date: 2025-08-21
slug: 任务调度器
image: pic8.jpg
categories:
    - 嵌入式
tags:
    - 任务调度器
---

# Task Scheduler 任务调度器

一个轻量级、可配置的任务调度管理系统，专为嵌入式系统设计。支持多任务定时调度、优先级管理、动态调整和低功耗模式。

## 📋 功能特性

- **多任务管理**：支持最多 20 个并发任务
- **优先级调度**：数值越小优先级越高（0 为最高优先级）
- **动态时间调整**：可根据任务执行时间自动调整调度间隔（可选）
- **低功耗支持**：在空闲时进入低功耗模式（可选）
- **任务状态管理**：支持激活、暂停、删除等状态控制
- **性能监控**：实时监控每个任务的最大执行时间

## 🏗️ 系统架构


## 源码下载

- [下载 Task.h](/code/Task/Task.h)
- [下载 Task.c](/code/Task/Task.c)


### 文件结构

```
project/
├── Task.h          # 头文件 - 类型定义和函数声明
├── Task.c          # 实现文件 - 核心调度逻辑
└── main.c          # 用户应用代码
```

### 核心数据结构

```c
typedef struct {
    const char *Name;           // 任务名称
    uint32_t LastWakeUp;        // 上一次唤醒时间
    uint16_t MaxUsed;           // 最大执行时间(ms)
    uint16_t Interval;          // 执行间隔(ms)
    uint8_t Priority;           // 优先级
    task_ea_t State;            // 任务状态
    uint8_t CreationIndex;      // 创建顺序
    void *Para;                 // 函数参数
    taskFunction_t Function;    // 任务函数指针
} task_t;
```

## 🔧 快速开始

### 1. 包含头文件

```c
#include "Task.h"
```

### 2. 定义任务函数

```c
void LED_Task(void *para) {
    // LED 控制逻辑
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

void Sensor_Read_Task(void *para) {
    // 传感器读取逻辑
    float *temperature = (float *)para;
    *temperature = read_temperature();
}
```

### 3. 添加任务到调度器

```c
float temp_value = 0.0f;

// 添加LED闪烁任务，每500ms执行一次，优先级为1
Task_Add("LED", LED_Task, 500, NULL, 1);

// 添加温度读取任务，每1000ms执行一次，优先级为2
Task_Add("Temperature", Sensor_Read_Task, 1000, &temp_value, 2);
```

### 4. 在主循环中启动调度器

```c
int main(void) {
    // 系统初始化...
    
    while (1) {
        Task_Start(HAL_GetTick);  // 传入系统时钟获取函数
    }
}
```

## ⚙️ 配置选项

在 `Task.h` 中修改以下宏定义：

```c
#define DYNAMIC_MODIFY 0     // 动态调整功能 (0:关闭, 1:开启)
#define LOW_POWER 0          // 低功耗模式 (0:关闭, 1:开启)
#define MAXTASKS 20          // 最大任务数量
#define MIN_SLEEP_TICK 10    // 最小休眠时间(ms)
#define TASK_ADJUST 5        // 任务调整力度(ms)
```

## 📖 API 参考

### 任务管理

- `Task_Add()` - 添加新任务
- `Task_Delete()` - 删除任务
- `Task_Suspend()` - 暂停任务
- `Task_Resume()` - 恢复任务

### 状态查询

- `Task_CheckNum()` - 获取当前任务数量
- `Task_GetMaxUsed()` - 获取任务最大执行时间

### 调度控制

- `Task_Start()` - 启动任务调度（在主循环中调用）

## 🎯 使用示例

### 基本使用

```c
// 定义任务函数
void MyTask(void *para) {
    printf("Task executed!\n");
}

// 添加任务
Task_Add("PrintTask", MyTask, 1000, NULL, 1);

// 主循环
while (1) {
    Task_Start(HAL_GetTick);
}
```

### 带参数的任务

```c
typedef struct {
    uint8_t id;
    uint16_t data;
} sensor_data_t;

void SensorTask(void *para) {
    sensor_data_t *data = (sensor_data_t *)para;
    data->data = read_sensor_value(data->id);
}

sensor_data_t sensor1 = {1, 0};
Task_Add("Sensor1", SensorTask, 200, &sensor1, 2);
```

## ⚠️ 注意事项

1. **时间精度**：依赖准确的系统时钟，建议使用硬件定时器
2. **任务执行时间**：单个任务执行时间不应超过最小任务间隔
3. **优先级冲突**：同优先级任务按创建顺序执行
4. **中断安全**：非线程安全，不要在中断中调用API函数
5. **内存占用**：每个任务占用约 32 字节内存

## 🔍 性能优化建议

1. **启用动态调整**：设置 `DYNAMIC_MODIFY 1` 自动优化任务间隔
2. **合理设置优先级**：关键任务设置更高优先级
3. **使用低功耗模式**：在电池供电设备中设置 `LOW_POWER 1`
4. **监控任务耗时**：定期检查 `MaxUsed` 避免任务超时

## 📊 版本信息

- **v2.0** (2025-08-17): 增强时间溢出处理，优化性能监控
- **v1.0** (2025-04-17): 初始版本，基础调度功能

## 👥 作者信息

**N1ntyNine99** - 电子爱好者

---

💡 **提示**: 在实际使用前，请根据您的硬件平台调整系统时钟获取函数（如 `HAL_GetTick()`）。