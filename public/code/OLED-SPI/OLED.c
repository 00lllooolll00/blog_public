/**
 * @file Oled.c
 * @brief OLED显示驱动模块实现文件
 * @details 基于I2C1+DMA实现高速OLED显示驱动
 *          支持硬件I2C和DMA非阻塞传输，优化显示性能
 * @author N1ntyNine99
 * @note 基于博客：https://blog.csdn.net/weixin_45065888/article/details/118225993
 * @date 2025-07-28
 */

#include "OLED.h"
#include "OLED_Font.h"

// OLED显示缓冲区，每页128列，共8页
static uint8_t OLED_GRAM[8][128];

// DMA传输相关缓冲区
static uint8_t OLED_GRAMbuf[2][8][128]; // DMA传输缓冲区 双缓冲区
static uint8_t currentBuffer = 0; // 当前使用的缓冲区
static uint8_t OLED_CMDbuf[8][3]; // 命令缓冲区
static volatile uint8_t CountFlag = 0; // 当前传输页计数
static volatile uint8_t BufFinshFlag = 0; // 传输完成标志
static volatile uint8_t DMA_Ready_Flag = 1; // 确保DMA传输完成后才开始下一个传输
static volatile OLED_DMA_STAGE_t CMD_Data_Flag = OLED_DMA_STAGE_CMD; //传输命令还是数据的标志位

// SPI命令/数据区分
#define OLED_DC_CMD()  HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET)
#define OLED_DC_DATA() HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET)

//重启
#define OLED_RESET()   HAL_GPIO_WritePin(OLED_REST_PORT, OLED_REST_PIN, GPIO_PIN_RESET) //低电平重启
#define OLED_DEFUALT() HAL_GPIO_WritePin(OLED_REST_PORT, OLED_REST_PIN, GPIO_PIN_SET)

//片选
#define OLED_CS_LOW()  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET)
#define OLED_CS_HIGH() HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET)

// OLED初始化命令序列
static const uint8_t OLED_Init_CMD[] = {0xAE, 0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8,
                                        0x3F, 0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1,
                                        0xDA, 0x12, 0xDB, 0x30, 0x8D, 0x14, 0xAF, 0x20, 0x00};

/**
 * @brief SPI DMA传输完成回调函数
 * @note 用于发送数据到OLED显存，放到HAL_SPI_TxCpltCallback中
 */
void OLED_SPI_TxCplt_Handler(void)
{
    if (!BufFinshFlag) return;

    if (CMD_Data_Flag == OLED_DMA_STAGE_CMD)
    {
        OLED_DC_DATA(); // 切换到数据阶段

        // 发送当前缓冲区的数据
        if (HAL_SPI_Transmit_DMA(OLED_SPI, OLED_GRAMbuf[currentBuffer][CountFlag], 128) != HAL_OK)
        {
            BufFinshFlag = 0;
            CountFlag = 0;
            CMD_Data_Flag = OLED_DMA_STAGE_CMD;
            OLED_CS_HIGH();
            return;
        }
        CMD_Data_Flag = OLED_DMA_STAGE_DATA;
    }
    else
    {
        if (CountFlag < 7)
        {
            CountFlag++;
            OLED_DC_CMD(); // 切换到命令阶段
            if (HAL_SPI_Transmit_DMA(OLED_SPI, OLED_CMDbuf[CountFlag], 3) != HAL_OK)
            {
                BufFinshFlag = 0;
                CountFlag = 0;
                CMD_Data_Flag = OLED_DMA_STAGE_CMD;
                OLED_CS_HIGH();
                return;
            }
            CMD_Data_Flag = OLED_DMA_STAGE_CMD;
        }
        else
        {
            BufFinshFlag = 0;
            CountFlag = 0;
            CMD_Data_Flag = OLED_DMA_STAGE_CMD;
            OLED_CS_HIGH(); // 最后一帧数据传输完成后拉高片选
        }
    }

    DMA_Ready_Flag = 1;
}
/**
 * @brief SPI错误回调函数
 * @note 处理SPI传输错误，重置DMA状态 放入HAL_SPI_ErrorCallback中
 */
void OLED_SPI_Error_Handler(void)
{
    HAL_SPI_Abort(OLED_SPI);
    OLED_CS_HIGH();

    BufFinshFlag = 0;
    CountFlag = 0;
    CMD_Data_Flag = OLED_DMA_STAGE_CMD;
    DMA_Ready_Flag = 1;
}

/**
 * @brief 向OLED写入一个字节（SPI）
 * @param dat 要写入的数据
 * @param cmd 命令/数据标志 (0:命令, 1:数据)
 */
void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    OLED_CS_LOW();
    if (cmd)
    {
        OLED_DC_DATA();
    }
    else
    {
        OLED_DC_CMD();
    }
    HAL_SPI_Transmit(OLED_SPI, &dat, 1, HAL_MAX_DELAY);
    OLED_CS_HIGH();
}

/**
 * @brief 开启OLED显示
 */
void OLED_Display_On(void)
{
    OLED_WR_Byte(OLED_CMD_DISPLAY_ON, 0);
}

/**
 * @brief 关闭OLED显示
 */
void OLED_Display_Off(void)
{
    OLED_WR_Byte(OLED_CMD_DISPLAY_OFF, 0);
}

/**
 * @brief 设置OLED显示位置
 * @param x 列坐标 (0-127)
 * @param y 页坐标 (0-7)
 */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
#if OLED_SIZE == OLED_1_3_INCH
    x += 2;
#endif
    OLED_CMDbuf[y][0] = 0xb0 + y; // 设置页地址
    OLED_CMDbuf[y][1] = ((x & 0xf0) >> 4) | 0x10; // 设置列地址高4位
    OLED_CMDbuf[y][2] = (x & 0x0f) | 0x00; // 设置列地址低4位
}

/**
 * @brief 清除OLED显示
 */
void OLED_Clear(void)
{
    memset(OLED_GRAM, 0x00, sizeof(OLED_GRAM));
}

/**
 * @brief 开启OLED（兼容性函数）
 */
void OLED_On(void)
{
    OLED_Display_On();
}

/**
 * @brief 填充OLED显示缓冲区
 * @param fill_Data 填充数据
 */
void OLED_fill_picture(uint8_t fill_Data)
{
    memset(OLED_GRAM, fill_Data, sizeof(OLED_GRAM));
}

/**
 * @brief 更新OLED显示缓冲区到屏幕（SPI+DMA）
 * @note 使用DMA高速传输，增强错误处理
 */
void OLED_Update(void)
{
    // 防止DMA重入，检查两个标志确保传输已完全停止
    if (!DMA_Ready_Flag || BufFinshFlag) return;
    DMA_Ready_Flag = 0; // Lock the DMA for this new transfer

    // 切换到下一个缓冲区
    uint8_t nextBuffer = (currentBuffer == 0) ? 1 : 0;

    // 准备下一个缓冲区的数据
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_CMDbuf[i][0] = 0xB0 + i;
        OLED_CMDbuf[i][1] = 0x10;
        OLED_CMDbuf[i][2] = 0x00;
        // 使用 memcpy 优化数据复制
        memcpy(OLED_GRAMbuf[nextBuffer][i], OLED_GRAM[i], 128);
    }

    // 切换到新的缓冲区以供下一次DMA使用
    currentBuffer = nextBuffer;

    // 设置状态标志以开始DMA序列
    BufFinshFlag = 1;
    CountFlag = 0;
    CMD_Data_Flag = OLED_DMA_STAGE_CMD;

    OLED_CS_LOW();
    OLED_DC_CMD(); // 发送命令部分

    // 启动DMA序列，数据发送将在DMA回调中链式进行
    if (HAL_SPI_Transmit_DMA(OLED_SPI, OLED_CMDbuf[0], 3) != HAL_OK)
    {
        // 如果启动失败，重置所有状态
        OLED_CS_HIGH();
        BufFinshFlag = 0;
        CountFlag = 0;
        CMD_Data_Flag = OLED_DMA_STAGE_CMD;
        DMA_Ready_Flag = 1; // Release the lock
    }
    // CS引脚将在DMA回调序列的最后一帧完成后拉高
}

/**
 * @brief 初始化OLED
 */
void OLED_Init(void)
{
    HAL_Delay(100); // 等待OLED稳定
    OLED_RESET(); //重启OLED
    HAL_Delay(300);
    OLED_DEFUALT();

    OLED_CS_LOW();
    // 发送初始化命令（SPI逐字节发送，D/C拉低）
    OLED_DC_CMD();
    HAL_SPI_Transmit(OLED_SPI, (uint8_t *)OLED_Init_CMD, sizeof(OLED_Init_CMD), HAL_MAX_DELAY);
    OLED_CS_HIGH();

    // 清除显示缓冲区
    OLED_Clear();
    OLED_Update();

    HAL_Delay(50);
}

/**
 * @brief 在指定位置显示一个字符
 * @param x 列坐标 (0-127)
 * @param y 像素行坐标 (0-63)
 * @param Char_Size 字符大小 (8:6x8, 16:8x16)
 * @param chr 要显示的字符
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t chr)
{
    unsigned char c = 0, i = 0;
    uint8_t page_y = y / 8; // 将像素坐标转换为页坐标

    c = chr - ' '; // 得到偏移后的值

    if (x > 127 - 1)
    {
        x = 0;
        page_y = page_y + 2;
    }

    if (Char_Size == 16)
    {
        // 检查边界
        if (page_y + 1 >= 8 || x + 8 > 128) return;

        for (i = 0; i < 8; i++)
        {
            if (x + i < 128) OLED_GRAM[page_y][x + i] = F8X16[c * 16 + i];
        }

        for (i = 0; i < 8; i++)
        {
            if (x + i < 128) OLED_GRAM[page_y + 1][x + i] = F8X16[c * 16 + i + 8];
        }
    }
    else
    {
        // 检查边界
        if (page_y >= 8 || x + 6 > 128) return;

        for (i = 0; i < 6; i++)
        {
            if (x + i < 128) OLED_GRAM[page_y][x + i] = F6x8[c][i];
        }
    }
}

/**
 * @brief 计算幂运算（内部使用）
 * @param m 底数
 * @param n 指数
 * @return 幂运算结果
 */
static uint32_t pow_internal(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

/**
 * @brief 显示数字
 * @param x 起始列坐标
 * @param y 起始页坐标
 * @param num 要显示的数字
 * @param Char_Size 字符大小
 * @param len 数字长度
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t Char_Size, uint8_t len)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t char_width = (Char_Size == 16) ? 8 : 6;

    for (t = 0; t < len; t++)
    {
        temp = (num / pow_internal(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + char_width * t, y, Char_Size, ' ');
                continue;
            }
            else enshow = 1;
        }
        OLED_ShowChar(x + char_width * t, y, Char_Size, temp + '0');
    }
}

/**
 * @brief 显示字符串
 * @param x 起始列坐标
 * @param y 起始像素行坐标
 * @param Char_Size 字符大小 (8:6x8, 16:8x16)
 * @param p 字符串指针

 */
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t Char_Size, uint8_t *p)
{
    uint8_t char_width = (Char_Size == 16) ? 8 : 6;
    uint8_t char_height_pixels = (Char_Size == 16) ? 16 : 8; // 像素高度

    while ((*p <= '~') && (*p >= ' ')) // 判断是不是非法字符!
    {
        if (x > (128 - char_width))
        {
            x = 0;
            y += char_height_pixels;
        }
        if (y >= 64 || (Char_Size == 16 && y >= 48)) // 检查像素边界
        {
            y = x = 0;
            OLED_Clear();
        }
        OLED_ShowChar(x, y, Char_Size, *p);
        x += char_width;
        p++;
    }
}

/**
 * @brief Printf风格的格式化显示
 * @param x 起始列坐标
 * @param y 起始像素行坐标 (0-63)
 * @param Char_Size 字符大小 (8:6x8, 16:8x16)
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void OLED_Printf(uint8_t x, uint8_t y, uint8_t Char_Size, const char *format, ...)
{
    char buffer[128];
    va_list args;

    // 确保坐标在有效范围内
    if (y >= 64) y = 0;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    OLED_ShowString(x, y, Char_Size, (uint8_t *)buffer);
}

// ==================== 画图函数实现 ====================

/**
 * @brief 画单个像素点
 * @param x 列坐标 (0-127)
 * @param y 行坐标 (0-63)
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t page, bit_pos;

    // 边界检查
    if (x >= 128 || y >= 64) return;

    page = y / 8; // 计算页地址
    bit_pos = y % 8; // 计算在页内的位位置

    if (color)
    {
        OLED_GRAM[page][x] |= (1 << bit_pos); // 置1
    }
    else
    {
        OLED_GRAM[page][x] &= ~(1 << bit_pos); // 清0
    }
}

/**
 * @brief 画直线 (Bresenham算法)
 * @param x1 起点列坐标
 * @param y1 起点行坐标
 * @param x2 终点列坐标
 * @param y2 终点行坐标
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;
    int16_t e2;

    int16_t x = x1, y = y1;

    while (1)
    {
        OLED_DrawPixel(x, y, color);

        if (x == x2 && y == y2) break;

        e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y += sy;
        }
    }
}

/**
 * @brief 画矩形边框
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param width 宽度
 * @param height 高度
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    // 画四条边
    OLED_DrawLine(x, y, x + width - 1, y, color); // 上边
    OLED_DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color); // 下边
    OLED_DrawLine(x, y, x, y + height - 1, color); // 左边
    OLED_DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color); // 右边
}

/**
 * @brief 填充矩形
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param width 宽度
 * @param height 高度
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    for (uint8_t i = 0; i < height; i++)
    {
        OLED_DrawLine(x, y + i, x + width - 1, y + i, color);
    }
}

/**
 * @brief 画圆形边框 (中点圆算法)
 * @param x0 圆心列坐标
 * @param y0 圆心行坐标
 * @param radius 半径
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color)
{
    int16_t x = 0;
    int16_t y = radius;
    int16_t d = 1 - radius;

    // 画8个对称点
    while (x <= y)
    {
        OLED_DrawPixel(x0 + x, y0 + y, color);
        OLED_DrawPixel(x0 - x, y0 + y, color);
        OLED_DrawPixel(x0 + x, y0 - y, color);
        OLED_DrawPixel(x0 - x, y0 - y, color);
        OLED_DrawPixel(x0 + y, y0 + x, color);
        OLED_DrawPixel(x0 - y, y0 + x, color);
        OLED_DrawPixel(x0 + y, y0 - x, color);
        OLED_DrawPixel(x0 - y, y0 - x, color);

        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

/**
 * @brief 填充圆形
 * @param x0 圆心列坐标
 * @param y0 圆心行坐标
 * @param radius 半径
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_FillCircle(uint8_t x0, uint8_t y0, uint8_t radius, uint8_t color)
{
    for (int16_t y = -radius; y <= radius; y++)
    {
        for (int16_t x = -radius; x <= radius; x++)
        {
            if (x * x + y * y <= radius * radius)
            {
                OLED_DrawPixel(x0 + x, y0 + y, color);
            }
        }
    }
}

/**
 * @brief 画三角形边框
 * @param x1 顶点1列坐标
 * @param y1 顶点1行坐标
 * @param x2 顶点2列坐标
 * @param y2 顶点2行坐标
 * @param x3 顶点3列坐标
 * @param y3 顶点3行坐标
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color)
{
    OLED_DrawLine(x1, y1, x2, y2, color);
    OLED_DrawLine(x2, y2, x3, y3, color);
    OLED_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief 填充三角形 (扫描线填充算法)
 * @param x1 顶点1列坐标
 * @param y1 顶点1行坐标
 * @param x2 顶点2列坐标
 * @param y2 顶点2行坐标
 * @param x3 顶点3列坐标
 * @param y3 顶点3行坐标
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_FillTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, uint8_t color)
{
    // 简化版本：使用扫描线填充
    int16_t min_y = (y1 < y2) ? ((y1 < y3) ? y1 : y3) : ((y2 < y3) ? y2 : y3);
    int16_t max_y = (y1 > y2) ? ((y1 > y3) ? y1 : y3) : ((y2 > y3) ? y2 : y3);

    for (int16_t y = min_y; y <= max_y; y++)
    {
        int16_t x_intersects[3];
        int16_t count = 0;

        // 计算与当前扫描线的交点
        if (((y1 <= y && y < y2) || (y2 <= y && y < y1)) && y1 != y2)
        {
            x_intersects[count++] = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
        }
        if (((y2 <= y && y < y3) || (y3 <= y && y < y2)) && y2 != y3)
        {
            x_intersects[count++] = x2 + (y - y2) * (x3 - x2) / (y3 - y2);
        }
        if (((y3 <= y && y < y1) || (y1 <= y && y < y3)) && y3 != y1)
        {
            x_intersects[count++] = x3 + (y - y3) * (x1 - x3) / (y1 - y3);
        }

        // 如果有两个交点，填充之间的像素
        if (count >= 2)
        {
            int16_t x_start = (x_intersects[0] < x_intersects[1]) ? x_intersects[0] : x_intersects[1];
            int16_t x_end = (x_intersects[0] > x_intersects[1]) ? x_intersects[0] : x_intersects[1];
            OLED_DrawLine(x_start, y, x_end, y, color);
        }
    }
}

/**
 * @brief 显示位图（纵向8点格式）
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param bitmap 位图数据指针
 * @param width 位图宽度
 * @param height 位图高度
 * @param color 颜色 (0:黑色, 1:白色)
 * 
 * @note 
 * 数据存储格式：
 * 纵向8点，低位在上，先从左到右，再从上到下
 * 每一个Bit对应一个像素点
 * 
 *      B0 B0                  B0 B0
 *      B1 B1                  B1 B1
 *      B2 B2                  B2 B2
 *      B3 B3  ------------->  B3 B3 --
 *      B4 B4                  B4 B4  |
 *      B5 B5                  B5 B5  |
 *      B6 B6                  B6 B6  |
 *      B7 B7                  B7 B7  |
 *                                    |
 *  -----------------------------------
 *  |   
 *  |   B0 B0                  B0 B0
 *  |   B1 B1                  B1 B1
 *  |   B2 B2                  B2 B2
 *  --> B3 B3  ------------->  B3 B3
 *      B4 B4                  B4 B4
 *      B5 B5                  B5 B5
 *      B6 B6                  B6 B6
 *      B7 B7                  B7 B7
 * 
 */
void OLED_DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height, uint8_t color)
{
    if (!bitmap || width == 0 || height == 0) return;
    if (x >= 128 || y >= 64) return;

    // 计算位图占用的页数
    uint8_t pages = (height + 7) / 8; // 向上取整
    uint8_t start_page = y / 8; // 起始页
    uint8_t offset_in_page = y % 8; // 页内偏移行数

    // 边界裁剪
    uint8_t actual_width = (x + width > 128) ? (128 - x) : width;
    uint8_t actual_pages = (start_page + pages > 8) ? (8 - start_page) : pages;

    for (uint8_t p = 0; p < actual_pages; p++)
    {
        uint8_t dest_page = start_page + p; // 目标页
        uint16_t src_offset = p * width; // 源数据偏移
        uint8_t *dest_addr = &OLED_GRAM[dest_page][x]; // 目标地址

        if (offset_in_page == 0)
        {
            // 无偏移，直接整行复制
            memcpy(dest_addr, &bitmap[src_offset], actual_width);
        }
        else
        {
            // 有页内偏移，需要进行位运算合并
            for (uint8_t col = 0; col < actual_width; col++)
            {
                uint8_t src_data = bitmap[src_offset + col];
                // 上移偏移行数并与目标页现有数据合并
                dest_addr[col] |= (src_data << offset_in_page);

                // 如果有数据溢出到下一页
                if (p + 1 < actual_pages && dest_page + 1 < 8)
                {
                    OLED_GRAM[dest_page + 1][x + col] |= (src_data >> (8 - offset_in_page));
                }
            }
        }

        // 颜色反转处理
        if (color == 0)
        {
            for (uint8_t col = 0; col < actual_width; col++)
            {
                dest_addr[col] = ~dest_addr[col];
            }
        }
    }
}
// ==================== 扩展画图函数实现 ====================

/**
 * @brief 画网格
 * @param spacing 网格间距
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawGrid(uint8_t spacing, uint8_t color)
{
    // 画垂直线
    for (uint8_t x = 0; x < 128; x += spacing)
    {
        OLED_DrawLine(x, 0, x, 63, color);
    }

    // 画水平线
    for (uint8_t y = 0; y < 64; y += spacing)
    {
        OLED_DrawLine(0, y, 127, y, color);
    }
}

/**
 * @brief 画进度条
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param width 进度条宽度
 * @param height 进度条高度
 * @param progress 进度 (0-100)
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawProgressBar(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t progress, uint8_t color)
{
    // 限制进度值
    if (progress > 100) progress = 100;

    // 计算填充宽度
    uint8_t fill_width = (width - 2) * progress / 100;

    // 绘制边框（每次都绘制，确保边框完整）
    OLED_DrawRect(x, y, width, height, color);

    // 绘制内部：分两部分，已填充部分和未填充部分
    if (fill_width > 0)
    {
        // 绘制已填充部分
        OLED_FillRect(x + 1, y + 1, fill_width, height - 2, color);
    }

    // 绘制未填充部分（如果有的话）
    if (fill_width < width - 2)
    {
        OLED_FillRect(x + 1 + fill_width, y + 1, (width - 2) - fill_width, height - 2, !color);
    }
}

/**
 * @brief 画带厚度的边框
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param width 宽度
 * @param height 高度
 * @param thickness 边框厚度
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawFrame(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t thickness, uint8_t color)
{
    for (uint8_t i = 0; i < thickness; i++)
    {
        OLED_DrawRect(x + i, y + i, width - 2 * i, height - 2 * i, color);
    }
}

/**
 * @brief 画圆角矩形
 * @param x 起始列坐标
 * @param y 起始行坐标
 * @param width 宽度
 * @param height 高度
 * @param radius 圆角半径
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color)
{
    // 限制圆角半径
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;

    // 画四条直线边
    OLED_DrawLine(x + radius, y, x + width - radius, y, color); // 上边
    OLED_DrawLine(x + radius, y + height - 1, x + width - radius, y + height - 1, color); // 下边
    OLED_DrawLine(x, y + radius, x, y + height - radius, color); // 左边
    OLED_DrawLine(x + width - 1, y + radius, x + width - 1, y + height - radius, color); // 右边

    // 画四个圆角
    for (int16_t i = 0; i < radius; i++)
    {
        for (int16_t j = 0; j < radius; j++)
        {
            if (i * i + j * j <= radius * radius)
            {
                // 左上角
                OLED_DrawPixel(x + radius - i, y + radius - j, color);
                // 右上角
                OLED_DrawPixel(x + width - radius + i - 1, y + radius - j, color);
                // 左下角
                OLED_DrawPixel(x + radius - i, y + height - radius + j - 1, color);
                // 右下角
                OLED_DrawPixel(x + width - radius + i - 1, y + height - radius + j - 1, color);
            }
        }
    }
}

/**
 * @brief 画椭圆
 * @param x0 椭圆中心列坐标
 * @param y0 椭圆中心行坐标
 * @param rx 水平半径
 * @param ry 垂直半径
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawEllipse(uint8_t x0, uint8_t y0, uint8_t rx, uint8_t ry, uint8_t color)
{
    int16_t x, y;
    int32_t rx2 = rx * rx;
    int32_t ry2 = ry * ry;
    int32_t fx2 = 4 * rx2;
    int32_t fy2 = 4 * ry2;
    int32_t s;

    // 第一段：从顶部到中间
    for (x = 0, y = ry, s = 2 * ry2 + rx2 * (1 - 2 * ry); ry2 * x <= rx2 * y; x++)
    {
        OLED_DrawPixel(x0 + x, y0 + y, color);
        OLED_DrawPixel(x0 - x, y0 + y, color);
        OLED_DrawPixel(x0 + x, y0 - y, color);
        OLED_DrawPixel(x0 - x, y0 - y, color);

        if (s >= 0)
        {
            s += fx2 * (1 - y);
            y--;
        }
        s += ry2 * ((4 * x) + 6);
    }

    // 第二段：从中间到右侧
    for (x = rx, y = 0, s = 2 * rx2 + ry2 * (1 - 2 * rx); rx2 * y <= ry2 * x; y++)
    {
        OLED_DrawPixel(x0 + x, y0 + y, color);
        OLED_DrawPixel(x0 - x, y0 + y, color);
        OLED_DrawPixel(x0 + x, y0 - y, color);
        OLED_DrawPixel(x0 - x, y0 - y, color);

        if (s >= 0)
        {
            s += fy2 * (1 - x);
            x--;
        }
        s += rx2 * ((4 * y) + 6);
    }
}

/**
 * @brief 画箭头
 * @param x1 起点列坐标
 * @param y1 起点行坐标
 * @param x2 终点列坐标
 * @param y2 终点行坐标
 * @param size 箭头大小
 * @param color 颜色 (0:黑色, 1:白色)
 */
void OLED_DrawArrow(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t size, uint8_t color)
{
    // 画主线
    OLED_DrawLine(x1, y1, x2, y2, color);

    // 计算箭头方向
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = sqrtf(dx * dx + dy * dy);

    if (length > 0)
    {
        // 单位向量
        float ux = dx / length;
        float uy = dy / length;

        // 垂直向量
        float vx = -uy;
        float vy = ux;

        // 箭头两个端点
        int16_t ax1 = x2 - size * ux + size * 0.5f * vx;
        int16_t ay1 = y2 - size * uy + size * 0.5f * vy;
        int16_t ax2 = x2 - size * ux - size * 0.5f * vx;
        int16_t ay2 = y2 - size * uy - size * 0.5f * vy;

        // 画箭头
        OLED_DrawLine(x2, y2, ax1, ay1, color);
        OLED_DrawLine(x2, y2, ax2, ay2, color);
    }
}