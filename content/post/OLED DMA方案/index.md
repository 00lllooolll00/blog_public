---
title: OLED DMA高刷驱动
description: 使用HAL库 DMA实现的OLED驱动
date: 2025-08-21
slug: OLED
image: OLED.jpg
categories:
    - 嵌入式
tags:
    - OLED
---
# OLED 0.96寸 DMA驱动库使用说明 (IIC & SPI 版本)

## 源码下载

### SPI版本
**注意:SPI版本目前还有问题，在显示大型图片会不稳定**
- [OLED.c](/code/OLED-SPI/OLED.c)
- [OLED.h](/code/OLED-SPI/OLED.h)
- [OLED_Font.h](/code/OLED-SPI/OLED_Font.h)

### IIC版本
- [OLED.c](/code/OLED-IIC/OLED.c)
- [OLED.h](/code/OLED-IIC/OLED.h)
- [OLED_Font.h](/code/OLED-IIC/OLED_Font.h)

## 📖 概述

本驱动库基于STM32 HAL库开发，专为0.96寸OLED显示屏设计，支持I2C和SPI两种通信方式，均采用DMA方案实现高性能显示。支持SSD1306控制器，分辨率128x64像素。两个版本的API接口完全相同，便于切换使用。

## ✨ 特性

- 🚀 **高性能**: 基于DMA非阻塞传输，显示效率高
- 🎨 **丰富的图形功能**: 支持点、线、矩形、圆形、三角形等几何图形
- 📝 **灵活的文本显示**: 支持6x8和8x16两种字体，支持Printf格式化输出
- 🔧 **易于集成**: 简单的API接口，易于移植
- 💾 **双缓冲机制**: 使用显示缓冲区，避免显示闪烁
- 🔄 **双模式支持**: IIC和SPI版本无缝切换，共享相同API

## 🛠️ 硬件配置要求

### IIC配置
- **I2C外设**: I2C1 (可在OLED.h中修改OLED_I2C宏定义)
- **I2C地址**: 0x78 (可在OLED.h中修改OLED_ADDR宏定义)
- **时钟频率**: 建议100kHz-400kHz

#### CubeMX配置步骤 (IIC)
1. **启用I2C1**
   - Mode: I2C
   - Configuration: Fast Mode (400kHz)

2. **启用DMA**
   - 在DMA Settings中添加I2C1_TX通道
   - Direction: Memory To Peripheral
   - Priority: Medium或High

3. **启用中断**
   - I2C1 event interrupt: ✅ 启用
   - I2C1 error interrupt: ✅ 启用
   - DMA1 Channel X global interrupt: ✅ 启用

4. **引脚配置**
   ```
   PB6  -> I2C1_SCL
   PB7  -> I2C1_SDA
   ```
   (具体引脚根据你的硬件设计调整)

### SPI配置
- **SPI外设**: SPI1 (可在OLED.h中修改OLED_SPI宏定义)
- **DC/RES/CS引脚**: 用于命令/数据切换和重置还有片选 (可在OLED.h中修改相关宏定义)

#### CubeMX配置步骤 (SPI)
1. **启用SPI1**
   
   - Mode: Full-Duplex Master 或者 Transmit Master Only(推荐)
   - 软件片选
   - Configuration:默认即可
   - SPI最稳定在 10M bits/s 最高在20M bits/s 左右
   
2. **启用DMA**
   - 在DMA Settings中添加SPI1_TX通道
   - Direction: Memory To Peripheral
   - Priority: Medium或High

3. **启用中断**
   - SPI1 global interrupt: ✅ 启用
   - DMA1 Channel X global interrupt: ✅ 启用

4. **引脚配置**
   ```
   PA5  -> SPI1_SCK
   PA7  -> SPI1_MOSI
   PB0  -> OLED_DC (命令/数据切换，高电平=数据，低电平=命令)
   PB1  -> OLED_RES (重置，低电平=重置)
   ```
   (具体引脚根据你的硬件设计调整)

## 📁 文件结构

```
OLED/
├── OLED.h          # 头文件，包含所有函数声明和宏定义 (IIC/SPI通用，根据宏配置)
├── OLED.c          # 主实现文件 (根据通信方式选择对应版本的实现)
├── OLED_Font.h     # 字体数据文件 (通用)
└── readme.md       # 本说明文档
```

**注意**: 根据选择的通信方式，使用对应的OLED.c实现文件。头文件OLED.h中通过宏定义（如OLED_SPI或OLED_I2C）区分配置。

## 🚀 快速开始

### 1. 添加文件到工程
将所有源文件添加到你的STM32项目中，并在主文件中包含头文件。根据通信方式在OLED.h中配置宏定义（如定义OLED_SPI或OLED_I2C），并把对应中断回调函数（如OLED_I2C_MasterTxCplt_Handler或OLED_SPI_TxCplt_Handler）放到HAL回调中。

```c
#include "OLED.h"
```

### 2. 初始化OLED
```c
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    // 根据模式初始化外设
    // IIC模式:
    MX_I2C1_Init();
    // SPI模式:
    // MX_SPI1_Init();
    MX_DMA_Init();
    
    // 初始化OLED
    OLED_Init();
    
    while(1)
    {
        // 你的主循环代码
    }
}
```

### 3. 基本显示示例
```c
// 清屏
OLED_Clear();

// 显示字符串
OLED_ShowString(0, 0, "Hello OLED!", 16);

// 显示数字
OLED_ShowNum(0, 2, 12345, 5, 16);

// 使用Printf格式化显示
OLED_Printf(0, 4, 16, "Temp: %d°C", 25);

// 更新显示（必须调用此函数才能在屏幕上看到内容）
OLED_Update();
```

## 📚 API参考

### 基础控制函数

#### `void OLED_Init(void)`
初始化OLED显示屏，必须在使用其他函数前调用。

#### `void OLED_Clear(void)`
清空显示缓冲区，将所有像素设为黑色。

#### `void OLED_Update(void)`
将显示缓冲区内容更新到OLED屏幕，使用DMA高速传输。

#### `void OLED_Display_On(void)` / `void OLED_Display_Off(void)`
开启/关闭OLED显示。

#### `void OLED_fill_picture(uint8_t fill_Data)`
用指定数据填充整个显示缓冲区。
- `fill_Data`: 填充数据 (0x00=全黑, 0xFF=全白)

### 文本显示函数

#### `void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t chr)`
在指定位置显示单个字符。
- `x`: 列坐标 (0-127)
- `y`: 页坐标 (0-7)
- `chr`: 要显示的字符
- `Char_Size`: 字符大小 (12: 6x8字体, 16: 8x16字体)

#### `void OLED_ShowString(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t *p)`
显示字符串。
- `p`: 字符串指针

#### `void OLED_ShowNum(uint8_t x, uint8_t y, uint8_t Char_Size, uint32_t num, uint8_t len)`
显示数字。
- `num`: 要显示的数字
- `len`: 数字长度
- `Char_Size`: 字符大小

#### `void OLED_Printf(uint8_t x, uint8_t y, uint8_t Char_Size, const char *format, ...)`
Printf风格的格式化显示。
- `format`: 格式化字符串
- `...`: 可变参数

### 图形绘制函数

#### 基础图形
```c
// 画像素点
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);

// 画直线
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);

// 画矩形边框
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

// 填充矩形
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);

// 画圆形边框
void OLED_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color);

// 填充圆形
void OLED_FillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color);
```

#### 高级图形
```c
// 画三角形
void OLED_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color);

// 填充三角形
void OLED_FillTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color);

// 画椭圆
void OLED_DrawEllipse(uint8_t x0, uint8_t y0, uint8_t rx, uint8_t ry, uint8_t color);

// 画圆角矩形
void OLED_DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);

// 画箭头
void OLED_DrawArrow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t size, uint8_t color);
```

#### 实用功能
```c
// 画网格
void OLED_DrawGrid(uint8_t spacing, uint8_t color);

// 画进度条
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t color);

// 画带厚度的边框
void OLED_DrawFrame(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t thickness, uint8_t color);

// 显示位图
void OLED_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height, uint8_t color);
```

### 坐标系统说明

- **列坐标 (x)**: 0-127，从左到右
- **行坐标 (y)**: 0-63，从上到下 (像素坐标)
- **页坐标 (y)**: 0-7，从上到下 (字符显示坐标，每页8个像素)
- **颜色参数**: 0=黑色(像素熄灭), 1=白色(像素点亮)

## 🎯 使用示例

### 示例1: 基本文本显示
```c
void DisplayBasicText(void)
{
    OLED_Clear();
    
    // 显示标题 (8x16字体)
    OLED_ShowString(0, 0, "STM32 OLED", 16);
    
    // 显示系统信息 (6x8字体)
    OLED_ShowString(0, 2, "System: Running", 12);
    OLED_ShowString(0, 3, "Voltage: 3.3V", 12);
    
    // 显示实时数据
    static uint16_t counter = 0;
    OLED_Printf(0, 5, 12, "Count: %d", counter++);
    
    OLED_Update();
}
```

### 示例2: 图形界面
```c
void DisplayGraphicsDemo(void)
{
    OLED_Clear();
    
    // 画边框
    OLED_DrawRect(0, 0, 128, 64, 1);
    
    // 画进度条
    static uint8_t progress = 0;
    OLED_DrawProgressBar(10, 10, 100, 8, progress, 1);
    OLED_Printf(10, 20, 12, "Progress: %d%%", progress);
    
    // 画一些装饰图形
    OLED_DrawCircle(100, 40, 10, 1);
    OLED_FillCircle(30, 40, 5, 1);
    OLED_DrawTriangle(50, 35, 60, 35, 55, 45, 1);
    
    progress = (progress + 1) % 101;
    OLED_Update();
}
```

### 示例3: 数据监控界面
```c
typedef struct {
    float temperature;
    float humidity;
    uint16_t battery_mv;
} SensorData_t;

void DisplaySensorData(SensorData_t* data)
{
    OLED_Clear();
    
    // 标题
    OLED_ShowString(35, 0, "Sensor Data", 12);
    OLED_DrawLine(0, 10, 127, 10, 1);
    
    // 温度
    OLED_Printf(5, 15, 12, "Temp: %.1f C", data->temperature);
    
    // 湿度
    OLED_Printf(5, 25, 12, "Humi: %.1f%%", data->humidity);
    
    // 电池电压
    OLED_Printf(5, 35, 12, "Batt: %dmV", data->battery_mv);
    
    // 电池电量条
    uint8_t battery_percent = (data->battery_mv - 3000) * 100 / 1200; // 3.0V-4.2V
    OLED_DrawProgressBar(5, 45, 80, 6, battery_percent, 1);
    
    // 状态指示器
    OLED_FillCircle(100, 48, 3, 1);
    OLED_ShowString(105, 47, "OK", 12);
    
    OLED_Update();
}
```

## ⚠️ 注意事项

1. **必须调用OLED_Update()**
   - 所有绘制操作都是在显示缓冲区中进行
   - 必须调用`OLED_Update()`才能将内容显示到屏幕上

2. **DMA传输状态**
   - `OLED_Update()`使用DMA非阻塞传输
   - 如果上一次传输未完成，会跳过本次更新
   - 正常情况下刷新频率可达到数十Hz

3. **中断回调函数**
   - 确保在stm32xxxx_it.c中正确实现I2C/SPI和DMA中断回调
   - 本库已提供回调函数实现，会自动处理DMA传输 (IIC: OLED_I2C_xxx_Handler; SPI: OLED_SPI_xxx_Handler)

4. **坐标边界**
   - 绘制时注意坐标边界，超出范围的绘制会被忽略
   - 文本显示会自动换行处理

5. **性能优化建议**
   - 避免频繁调用`OLED_Clear()`
   - 批量绘制后再调用`OLED_Update()`
   - 对于动画效果，建议控制刷新频率

6. **模式切换**
   - 在OLED.h中通过宏定义选择IIC或SPI模式
   - SPI版本额外需要DC和RES引脚控制

## 🔧 故障排除

### 显示屏无反应
1. 检查通信连接 (IIC: SCL/SDA; SPI: SCK/MOSI/DC/RES)
2. 确认CubeMX配置中的DMA和中断设置
3. 验证电源供电 (3.3V/5V)

### 显示内容不更新
1. 确保调用了`OLED_Update()`
2. 检查DMA传输是否正常工作
3. 验证中断是否正确响应 (IIC/SPI特定)

### 显示异常/乱码
1. 检查字体文件是否正确包含
2. 确认坐标参数是否在有效范围内
3. 验证字符编码是否匹配

## 📝 版本历史

- **v1.0** (2025-07-28)
  - 初始版本发布，支持IIC和SPI双模式
  - 支持基本文本和图形显示
  - 实现DMA高速传输

## 👨‍💻 作者信息

- **作者**: N1ntyNine99
- **基于**: [CSDN博客文章](https://blog.csdn.net/hwytree/article/details/123559144) 和 [另一个CSDN文章](https://blog.csdn.net/weixin_45065888/article/details/118225993)
- **日期**: 2025-07-28

## 📄 许可证

本项目采用开源许可证，请根据需要使用和修改。

---

💡 **提示**: 如有任何问题或建议，欢迎通过Issues反馈！