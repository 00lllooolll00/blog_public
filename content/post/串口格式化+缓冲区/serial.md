---
title: 串口格式化发送+环形缓冲区
description: 基于HAL库实现
date: 2025-08-21
slug: 串口
categories:
    - 嵌入式
tags:
    - 串口
---
# 串口通信模块使用说明

## 概述

本模块实现了一个功能完整的串口通信系统，支持环形缓冲区数据管理、DMA数据传输、格式化输出以及数据帧解析功能。适用于STM32等嵌入式系统的串口通信需求。

## 源码下载地址
- [Serial.c](/code/Serial/Serial.c)
- [Serial.h](/code/Serial/Serial.h)

## 功能特性

- ✅ **环形缓冲区管理**：高效的数据缓存与处理机制
- ✅ **DMA数据传输**：支持DMA发送和接收，提高传输效率
- ✅ **格式化输出**：提供类似printf的格式化输出功能
- ✅ **数据帧解析**：自动解析数据帧，支持校验功能
- ✅ **指令提取**：从数据帧中提取有效指令数据
- ✅ **可配置参数**：支持多种配置选项

## 文件结构

```
串口格式化发送+环形缓冲区/
├── Serial.h        # 头文件，包含宏定义和函数声明
├── Serial.c        # 实现文件，包含所有功能实现
└── README.md       # 本使用说明文档
```

## 配置参数

### 基本配置 (Serial.h)

```c
#define USER_UART              &huart1        // 指定使用的串口
#define RX_TX_BUFFERS          255           // 发送/接收缓存大小
#define CIRCULAR_BUFFER_ENABLE 0             // 启用环形缓冲区 (0=禁用, 1=启用)
```

### 环形缓冲区配置

```c
#define DATAPACK_HEAD   0xAA    // 数据包头标识
#define CHECKSUM_ENABLE 1       // 校验功能 (1=启用, 0=禁用)
#define CMD_MIN_LENGTH  4       // 指令最小长度 (自动计算)
#define CMD_MAX_LENGTH  255     // 指令最大长度
#define CIRCLBUFFERS_SIZE 255   // 环形缓冲区大小
```

## 数据帧格式

### 启用校验位时
```
| 包头(1byte) | 长度(1byte) | 数据(n bytes) | 校验位(1byte) |
|    0xAA     |   数据长度   |   有效数据    |    校验和     |
```

### 不启用校验位时
```
| 包头(1byte) | 长度(1byte) | 数据(n bytes) |
|    0xAA     |   数据长度   |   有效数据    |
```

**注意**：长度字段表示整个数据帧的长度，包括包头、长度字段本身、数据和校验位。

## API接口

### 1. 格式化输出函数

#### MyPrintf()
```c
uint16_t MyPrintf(UART_HandleTypeDef *huart, const char *format, ...);
```
- **功能**：格式化串口输出，类似printf函数
- **参数**：
  - huart - 使用的串口句柄
  - format - 格式化字符串
  - ... - 可变参数
- **返回值**：实际发送的字节数
- **示例**：
```c
MyPrintf(&huart1, "温度: %d°C, 湿度: %d%%\r\n", temperature, humidity);
```

#### MyPrintf_DMA()
```c
uint16_t MyPrintf_DMA(UART_HandleTypeDef *huart, const char *format, ...);
```
- **功能**：通过DMA格式化串口输出
- **参数**：
  - huart - 使用的串口句柄
  - format - 格式化字符串
  - ... - 可变参数
- **返回值**：实际发送的字节数
- **示例**：
```c
MyPrintf_DMA(&huart1, "系统状态: %s\r\n", status_str);
```

### 2. 环形缓冲区函数

#### Buffer_Write()
```c
uint8_t Buffer_Write(uint8_t *Data, uint8_t Length);
```
- **功能**：向环形缓冲区写入数据
- **参数**：Data - 数据指针，Length - 数据长度
- **返回值**：实际写入的字节数，0表示缓冲区空间不足
- **示例**：
```c
uint8_t data[] = {0x01, 0x02, 0x03};
uint8_t result = Buffer_Write(data, sizeof(data));
```

#### Buffer_Read()
```c
uint8_t Buffer_Read(BufferType_t *Command);
```
- **功能**：从环形缓冲区读取一个完整的指令
- **参数**：Command - 未使用（可传入NULL）
- **返回值**：指令数据长度，0表示没有找到有效指令
- **示例**：
```c
uint8_t cmd_length = Buffer_Read(NULL);
if (cmd_length > 0) {
    // 处理Serial_Command数组中的指令数据
    for (int i = 0; i < cmd_length - 2; i++) {
        // 处理Serial_Command[i]
    }
}
```

### 3. 外部变量

#### Serial_Command[]
```c
extern uint8_t Serial_Command[CMD_MAX_LENGTH - (CHECKSUM_ENABLE ? 3 : 2)];
```
- **功能**：存储解析出的指令数据（不包含包头、长度和校验位）
- **使用**：当Buffer_Read()返回值大于0时，该数组包含有效的指令数据

#### RxCplt_Flag
```c
extern FlagStatus RxCplt_Flag;
```
- **功能**：数据接收完成标志位（仅在CIRCULAR_BUFFER_ENABLE=0时使用）
- **使用**：检查是否有新数据接收完成

#### RxSerial_Buffer[]
```c
extern uint8_t RxSerial_Buffer[RX_TX_BUFFERS];
```
- **功能**：串口接收缓冲区

#### Serial_Idle_Handler()
```c
void Serial_Idle_Handler(void);
```
- **功能**：串口空闲中断处理函数（仅在CIRCULAR_BUFFER_ENABLE=0时使用）
- **使用**：在串口空闲中断中调用此函数来处理接收到的数据

## 使用步骤

### 1. 硬件配置
- 配置STM32的UART外设
- 启用DMA（推荐）
- 配置串口参数（波特率、数据位等）
- 启用串口空闲中断（普通缓冲区模式需要）

### 2. 中断配置
在CubeMX中或代码中配置串口空闲中断：

```c
// 启用串口空闲中断
__HAL_UART_ENABLE_IT(USER_UART, UART_IT_IDLE);

// 在中断服务函数中处理
void USART1_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(USER_UART, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(USER_UART);
        Serial_Idle_Handler();
    }
    HAL_UART_IRQHandler(USER_UART);
}
```

### 3. 软件集成

1. **包含头文件**
```c
#include "Serial.h"
```

2. **配置参数**
根据需求修改Serial.h中的配置宏定义

3. **初始化串口接收**
```c
// 启动DMA接收
HAL_UARTEx_ReceiveToIdle_DMA(USER_UART, RxSerial_Buffer, sizeof(RxSerial_Buffer));

// 在串口空闲中断中调用处理函数（普通缓冲区模式）
void USART1_IRQHandler(void)
{
    // 检测空闲中断
    if(__HAL_UART_GET_FLAG(USER_UART, UART_FLAG_IDLE))
    {
        __HAL_UART_CLEAR_IDLEFLAG(USER_UART);
        Serial_Idle_Handler();
    }
    HAL_UART_IRQHandler(USER_UART);
}
```

### 4. 使用模式

#### 模式1：普通缓冲区模式 (默认)
设置 `CIRCULAR_BUFFER_ENABLE = 0`

```c
void main_loop() {
    if (RxCplt_Flag == SET) {
        RxCplt_Flag = RESET;
        // 处理RxSerial_Buffer中的数据
        process_raw_data(RxSerial_Buffer);
    }
    
    // 发送数据
    MyPrintf(&huart1, "状态更新: %d\r\n", system_status);
}
```

#### 模式2：环形缓冲区模式 (推荐用于复杂协议)
设置 `CIRCULAR_BUFFER_ENABLE = 1`

```c
void main_loop() {
    uint8_t cmd_length = Buffer_Read(NULL);
    if (cmd_length > 0) {
        // 处理接收到的指令
        process_command(Serial_Command, cmd_length - (CHECKSUM_ENABLE ? 3 : 2));
    }
    
    // 发送数据
    MyPrintf_DMA(&huart1, "状态更新: %d\r\n", system_status);
}
```

## 数据发送示例

### 发送标准数据帧
```c
void send_command(uint8_t cmd, uint8_t *data, uint8_t data_len) {
    uint8_t frame[256];
    uint8_t frame_len = 0;
    
    // 构建数据帧
    frame[frame_len++] = DATAPACK_HEAD;                    // 包头
    frame[frame_len++] = data_len + (CHECKSUM_ENABLE ? 4 : 3); // 总长度
    frame[frame_len++] = cmd;                              // 命令
    
    // 添加数据
    for (uint8_t i = 0; i < data_len; i++) {
        frame[frame_len++] = data[i];
    }
    
    // 添加校验位（如果启用）
    if (CHECKSUM_ENABLE) {
        uint8_t checksum = 0;
        for (uint8_t i = 1; i < frame_len; i++) {
            checksum += frame[i];
        }
        frame[frame_len++] = checksum;
    }
    
    // 发送数据帧
    HAL_UART_Transmit_DMA(&huart1, frame, frame_len);
}
```

## 应用示例

### 温度传感器数据发送
```c
void send_temperature_data() {
    float temperature = 25.6;
    uint8_t temp_data[4];
    
    // 将浮点数转换为字节数组
    memcpy(temp_data, &temperature, sizeof(float));
    
    // 发送温度数据
    send_command(0x01, temp_data, sizeof(temp_data));
}
```

### 指令处理示例
```c
void process_received_command() {
    uint8_t cmd_length = Buffer_Read(NULL);
    if (cmd_length > 0) {
        uint8_t command = Serial_Command[0];
        
        switch (command) {
            case 0x01: // 查询温度
                send_temperature_data();
                break;
                
            case 0x02: // 设置参数
                if (cmd_length >= 3) {
                    uint8_t param = Serial_Command[1];
                    uint8_t value = Serial_Command[2];
                    set_parameter(param, value);
                }
                break;
                
            default:
                MyPrintf(&huart1, "未知命令: 0x%02X\r\n", command);
                break;
        }
    }
}
```

## 注意事项

1. **内存管理**：确保CIRCLBUFFERS_SIZE设置合理，避免溢出
2. **DMA配置**：正确配置DMA中断和回调函数
3. **校验功能**：根据通信可靠性需求选择是否启用校验
4. **线程安全**：在多线程环境中注意数据访问的同步
5. **错误处理**：建议添加适当的错误处理机制
6. **串口句柄**：使用格式化输出函数时需要传入正确的串口句柄
7. **空闲中断**：在普通缓冲区模式下需要正确配置和处理串口空闲中断

## 故障排除

### 常见问题

1. **数据接收不完整**
   - 检查DMA配置是否正确
   - 确认串口参数匹配
   - 检查缓冲区大小是否足够

2. **校验失败**
   - 确认发送端和接收端校验算法一致
   - 检查数据传输过程中是否有丢失或错误

3. **缓冲区溢出**
   - 增大CIRCLBUFFERS_SIZE
   - 提高数据处理频率
   - 优化数据处理算法

4. **空闲中断不触发**
   - 检查NVIC中断配置是否正确
   - 确认空闲中断标志位清除方法正确
   - 验证Serial_Idle_Handler()是否被正确调用

## 版本信息

- **作者**：N1ntyNine99
- **版本**：v1.0
- **日期**：2025-04-17
- **更新日期**：2025-07-27

## 许可证

本模块代码仅供学习和参考使用。
