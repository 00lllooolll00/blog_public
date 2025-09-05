---
title: 滤波器
description: 嵌入式系统滤波器
date: 2025-08-21
lastmod: 2025-09-05
slug: 滤波器
image: pic5.png
categories:
    - 嵌入式
tags:
    - 滤波
---
# MyFilter - 嵌入式信号处理滤波器库

一个轻量级、高性能的嵌入式信号处理滤波器库，提供四种常用的数字滤波器实现，适用于各种嵌入式系统的信号处理需求。

## 👇🏻源码下载
- [MyFilter.c](/code/MyFilter/MyFilter.c)
- [MyFilter.h](/code/MyFilter/MyFilter.h)

## 📦 功能特性

- **低通滤波器 (Low Pass Filter)** - 一阶 IIR 滤波器，计算简单，实时性好
- **中值滤波器 (Median Filter)** - 有效抑制脉冲噪声，保持信号边缘
- **均值滤波器 (Moving Average)** - 平滑随机噪声，计算效率高
- **卡尔曼滤波器 (Kalman Filter)** - 最优估计算法，自适应调整

## 🏗️ 文件结构

```
MyFilter/
├── MyFilter.h          # 头文件 - 结构体定义和函数声明
├── MyFilter.c          # 实现文件 - 滤波器算法实现
└── README.md           # 说明文档
```

## 🔧 快速开始

### 1. 包含头文件

```c
#include "MyFilter.h"
```

### 2. 初始化滤波器

```c
// 低通滤波器初始化
lowpass_filter_t lpf;
LowPass_Filter_Init(&lpf, 0.8f);  // 系数0.8，强滤波

// 中值滤波器初始化  
midvalue_filter_t mvf;
MidValue_Filter_Init(&mvf, 5);    // 窗口大小5

// 均值滤波器初始化
average_filter_t avgf;
Avg_Filter_Init(&avgf, 10);       // 10个样本

// 卡尔曼滤波器初始化
kalman_filter_t kf;
Kalman_Filter_Init(&kf, 0.1f, 1.0f, 0.0f);  // Q=0.1, R=1.0, 初始值=0
```

### 3. 实时滤波处理

```c
float sensor_value = read_sensor();  // 读取传感器原始数据

// 应用各种滤波器
LowPass_Filter(&lpf, sensor_value);
MidValue_Filter(&mvf, sensor_value); 
Avg_Filter(&avgf, sensor_value);
Kalman_Filter(&kf, sensor_value);

// 获取滤波结果
float filtered_value = kf.KF_Res;  // 使用卡尔曼滤波结果
```

## 📊 滤波器对比指南

| 滤波器类型     | 适用场景     | 优点                 | 缺点         | 计算复杂度 |
| -------------- | ------------ | -------------------- | ------------ | ---------- |
| **低通滤波**   | 一般噪声抑制 | 计算简单，内存占用少 | 相位延迟     | O(1)       |
| **中值滤波**   | 脉冲噪声抑制 | 有效去除异常值       | 需要缓冲区   | O(n log n) |
| **均值滤波**   | 随机噪声平滑 | 简单有效             | 响应较慢     | O(n)       |
| **卡尔曼滤波** | 最优估计     | 自适应，精度高       | 参数调优复杂 | O(1)       |

## ⚙️ 参数配置

### 低通滤波器参数
```c
// 滤波系数选择指南：
LowPass_Filter_Init(&filter, 0.1f);  // 轻滤波 - 实时控制
LowPass_Filter_Init(&filter, 0.5f);  // 中等滤波 - 一般应用  
LowPass_Filter_Init(&filter, 0.8f);  // 强滤波 - 传感器去噪
```

### 中值滤波器参数
```c
// 窗口大小选择：
MidValue_Filter_Init(&filter, 3);   // 快速响应 - 图像处理
MidValue_Filter_Init(&filter, 7);   // 平衡效果 - 一般应用
MidValue_Filter_Init(&filter, 11);  // 强滤波 - 严重噪声
```

### 卡尔曼滤波器参数
```c
// 噪声协方差调优：
Kalman_Filter_Init(&filter, 0.01f, 0.1f, 0.0f);  // 高精度传感器
Kalman_Filter_Init(&filter, 0.1f, 1.0f, 0.0f);   // 中等精度传感器  
Kalman_Filter_Init(&filter, 1.0f, 10.0f, 0.0f);  // 低精度传感器
```

## 🎯 应用示例

### 传感器数据滤波
```c
// 温度传感器滤波
lowpass_filter_t temp_filter;
LowPass_Filter_Init(&temp_filter, 0.7f);

float read_temperature(void) {
    float raw_temp = read_adc() * 0.1f;  // 原始数据
    LowPass_Filter(&temp_filter, raw_temp);
    return temp_filter.LF_Res;  // 滤波后的温度
}
```

### 电机控制信号平滑
```c
// 电机控制信号处理
average_filter_t motor_filter;
Avg_Filter_Init(&motor_filter, 5);

void set_motor_speed(float speed) {
    Avg_Filter(&motor_filter, speed);
    analog_write(MOTOR_PIN, motor_filter.AF_Res);  // 平滑后的速度
}
```

### IMU 数据融合
```c
// 陀螺仪数据卡尔曼滤波
kalman_filter_t gyro_filter;
Kalman_Filter_Init(&gyro_filter, 0.001f, 0.1f, 0.0f);

float filter_gyro(float raw_gyro) {
    Kalman_Filter(&gyro_filter, raw_gyro);
    return gyro_filter.KF_Res;  // 优化后的角速度
}
```

## ⚠️ 使用注意事项

1. **内存分配**：所有滤波器使用静态内存，无需动态分配
2. **实时性**：考虑最坏情况下的执行时间（中值滤波耗时最长）
3. **初始化**：使用前必须调用初始化函数
4. **参数范围**：注意各参数的有效范围，避免异常值
5. **数据类型**：使用 float 类型，确保平台支持浮点运算

## 🔧 性能优化建议

1. **根据需求选择滤波器**：不要过度设计，选择最简单的有效滤波器
2. **合理设置参数**：根据信号特性调整滤波器参数
3. **缓冲区大小**：在效果和内存之间找到平衡点
4. **采样率匹配**：滤波器参数应与系统采样率相匹配

## 📝 API 参考

### 低通滤波器
- `LowPass_Filter_Init()` - 初始化低通滤波器
- `LowPass_Filter()` - 执行低通滤波

### 中值滤波器  
- `MidValue_Filter_Init()` - 初始化中值滤波器
- `MidValue_Filter()` - 执行中值滤波

### 均值滤波器
- `Avg_Filter_Init()` - 初始化均值滤波器
- `Avg_Filter()` - 执行均值滤波

### 卡尔曼滤波器
- `Kalman_Filter_Init()` - 初始化卡尔曼滤波器
- `Kalman_Filter()` - 执行卡尔曼滤波

## 📊 资源占用

- **代码大小**: ~3KB (取决于启用哪些滤波器)
- **RAM 占用**: 约 100-200 字节（取决于配置）
- **计算时间**: 低通/卡尔曼: O(1), 均值: O(n), 中值: O(n log n)

## 🐛 故障排除

**问题**: 滤波效果不明显
- **解决**: 调整滤波器参数，增大滤波强度

**问题**: 响应速度太慢
- **解决**: 减小窗口大小或滤波系数

**问题**: 内存占用过大
- **解决**: 减小缓冲区大小，使用更简单的滤波器

## 📄 许可证

MIT License - 可自由用于个人和商业项目。

## 👥 作者信息

**N1ntyNine99** - 嵌入式系统开发者

---

💡 **提示**: 在实际使用前，建议根据具体的应用场景和硬件平台进行参数调优和性能测试。