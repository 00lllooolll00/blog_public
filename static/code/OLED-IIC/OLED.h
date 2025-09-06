/**
 * @file Oled.h
 * @brief OLED显示驱动模块头文件
 * @details 定义了OLED显示相关的函数接口和参数
 *          支持硬件I2C和软件I2C两种通信方式
 * @author N1ntyNine99
 * @note 在原作者的基础之上修改 原帖：https://blog.csdn.net/hwytree/article/details/ 
 * 123559144
 * @date 2025-07-28
 */

#ifndef __OLED_h
#define __OLED_h

#include "SysConfig.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* OLED屏幕尺寸配置 */
#define OLED_0_96_INCH 1 // 0.96英寸OLED
#define OLED_1_3_INCH  0 // 1.3英寸OLED

#ifndef OLED_SIZE

#define OLED_SIZE OLED_0_96_INCH // 默认使用0.96英寸OLED

#endif



#define OLED_I2C &hi2c1
/**
 * 配置模式选择
 */
#define OLED_ADDR 0x78 // OLED的I2C设备地址

/**
 * OLED命令定义
 */
#define OLED_CMD_DISPLAY_ON      0xAF // 开启显示
#define OLED_CMD_DISPLAY_OFF     0xAE // 关闭显示
#define OLED_CMD_SET_CONTRAST    0x81 // 设置对比度
#define OLED_CMD_NORMAL_DISPLAY  0xA6 // 正常显示模式
#define OLED_CMD_INVERSE_DISPLAY 0xA7 // 反显模式

/**
 * 中断处理
 */
void OLED_I2C_MasterTxCplt_Handler(void);
void OLED_I2C_MemTxCplt_Handler(void);
void OLED_I2C_Error_Handler(void);

/**
 * 基础函数声明 
 */
void OLED_WR_Byte(uint8_t dat, uint8_t cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_On(void);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_Update(void); // 新增的缓冲区更新函数
void OLED_fill_picture(uint8_t fill_Data);

/**
 * 显示函数声明
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t chr);
void OLED_ShowNum(uint8_t x, uint8_t y, uint8_t Char_Size, uint32_t num, uint8_t len);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t *p);
void OLED_Printf(uint8_t x, uint8_t y, uint8_t Char_Size, const char *format, ...); // 新增的Printf风格函数

/**
 * 画图函数声明
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void OLED_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color);
void OLED_FillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color);
void OLED_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color);
void OLED_FillTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color);
void OLED_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height, uint8_t color);

/**
 * 扩展画图函数声明
 */
void OLED_DrawGrid(uint8_t spacing, uint8_t color);
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t color);
void OLED_DrawFrame(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t thickness, uint8_t color);
void OLED_DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);
void OLED_DrawEllipse(uint8_t x0, uint8_t y0, uint8_t rx, uint8_t ry, uint8_t color);
void OLED_DrawArrow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t size, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_h */
