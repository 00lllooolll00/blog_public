#include "HWT101.h"

#define HWT_BUFFER_SIZE 11 //HWT101的数据缓冲区大小
#define HWT_FRAME_SIZE  11 //HWT101一帧数据长度

#define HWT_SEND_CMD(X) HAL_UART_Transmit(HWT101_UART, (X), sizeof(X), HAL_MAX_DELAY) //命令发送宏定义
#define HWT_DELAY(X)    HAL_Delay(X) //延时函数

static uint8_t HWT101_DataPack[HWT_BUFFER_SIZE];
static const uint8_t HWT101_UnlockReg[5] = {0xFF, 0xAA, 0x69, 0x88, 0xB5}; //解锁寄存器
static const uint8_t HWT101_ZtoZero[5] = {0xFF, 0xAA, 0x76, 0x00, 0x00}; //z轴角度归零
static const uint8_t HWT101_200HzOutput[5] = {0xFF, 0xAA, 0x0B, 0x00}; //200hz输出
static const uint8_t HWT101_500HzOutput[5] = {0xFF, 0xAA, 0x0C, 0x00}; //200hz输出
static const uint8_t HWT101_BuadRate115200[5] = {0xFF, 0xAA, 0x04, 0x06, 0x00}; //波特率115200
static const uint8_t HWT101_Save[5] = {0xFF, 0xAA, 0x00, 0x00, 0x00}; //保存数据
static const uint8_t HWT101_Reset[5] = {0xFF, 0xAA, 0x00, 0xFF, 0x00}; //重启
static const uint8_t HWT101_LED_OFF[5] = {0xFF, 0xAA, 0x1B, 0x01, 0x00}; //关闭LED
static const uint8_t HWT101_LED_ON[5] = {0xFF, 0xAA, 0x1B, 0x00, 0x00}; //开启LED

static uint16_t HWT101_Size = 0; //接收到的数据大小

bool HWT_Ready_Flag = false; //数据准备完成
float HWT_Yaw_Angle = 0; //偏航角
float HWT_Y_Speed = 0; //偏航角速度
float HWT_Z_Speed = 0; //偏航角速度

/**
 * @brief 初始化HWT101
 * 
 */
void HWT101_Init(void)
{
    // 发送配置命令，每个命令之间增加延时确保设备有时间处理
    HWT_SEND_CMD(HWT101_LED_OFF);
    HWT_DELAY(10);
    HWT_SEND_CMD(HWT101_UnlockReg);
    HWT_DELAY(10);
    HWT_SEND_CMD(HWT101_ZtoZero);
    HWT_DELAY(10);
    HWT_SEND_CMD(HWT101_500HzOutput);
    HWT_DELAY(10);
    HWT_SEND_CMD(HWT101_BuadRate115200);
    HWT_DELAY(10);
    HWT_SEND_CMD(HWT101_Save);
    HWT_DELAY(100); // 保存需要更长时间
    HWT_SEND_CMD(HWT101_Reset);
    HWT_DELAY(500); // 重启需要更长时间

    HAL_UARTEx_ReceiveToIdle_DMA(HWT101_UART, HWT101_DataPack, HWT_BUFFER_SIZE); //使用dma
    HWT_Ready_Flag = false;
    memset(HWT101_DataPack, 0, HWT_BUFFER_SIZE);
}

/**
 * @brief 计算校验和
 * 
 * @param data 计算的数组
 * @param length 数组长度(不包括校验字节)
 * @return uint8_t 校验和
 */
static uint8_t HWT_CheckSum(uint8_t *data, uint16_t length)
{
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < length; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief 获取偏航角
 * 
 * @return true 数据解析成功
 * @return false 数据解析失败
 */
bool HWT101_Read(void)
{
    // 计算前10个字节的校验和(不包括第11个校验字节)
    uint8_t Sum = HWT_CheckSum(HWT101_DataPack, HWT_FRAME_SIZE - 1);

    if (HWT101_Size < HWT_FRAME_SIZE) return false; // 数据长度不足
    if (HWT101_Size > HWT_BUFFER_SIZE) return false; // 数据长度超出
    if (Sum != HWT101_DataPack[10]) return false; // 校验和不匹配
    if (HWT101_DataPack[0] != 0x55) return false; // 包头不正确

    bool Res = false;

    //解析角度值
    if (HWT101_DataPack[1] == 0x53)
    {
        uint8_t YawL = HWT101_DataPack[6];
        uint8_t YawH = HWT101_DataPack[7];
        int16_t Temp = (int16_t)((YawH << 8) | YawL);
        HWT_Yaw_Angle = (float)Temp * 180.0f / 32768.0f;

        Res = true;
    }
    //解析角速度
    else if (HWT101_DataPack[1] == 0x52)
    {
        uint8_t ySpeed_L = HWT101_DataPack[4];
        uint8_t ySpeed_H = HWT101_DataPack[5];
        int16_t yTemp = (int16_t)((ySpeed_H << 8) | ySpeed_L);
        HWT_Y_Speed = ((float)yTemp / 32768.0f) * 2000.0f;

        uint8_t zSpeed_L = HWT101_DataPack[6];
        uint8_t zSpeed_H = HWT101_DataPack[7];
        int16_t zTemp = (int16_t)((zSpeed_H << 8) | zSpeed_L);
        HWT_Z_Speed = ((float)zTemp / 32768.0f) * 2000.0f;

        Res = true;
    }

    memset(HWT101_DataPack, 0, HWT_BUFFER_SIZE);
    return Res; // 返回实际的解析结果
}

/**
 * @brief 空闲中断
 * 
 * @param size 接收到的数据大小
 */
void HWT101_Handler(uint16_t size)
{
    HAL_UARTEx_ReceiveToIdle_DMA(HWT101_UART, HWT101_DataPack, HWT_BUFFER_SIZE);
    HWT_Ready_Flag = true;
    HWT101_Size = size;
}