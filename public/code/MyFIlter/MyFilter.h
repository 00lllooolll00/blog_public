#ifndef __MyFilter_h
#define __MyFilter_h

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/**
 * @brief 低通滤波器结构体
 * 
 */
typedef struct
{
    float LF_LastValue; //上次输入的值
    float LF_NowValue; //这次的输入值
    float LF_Index; //滤波系数 系数越大滤波力度越大
    float LF_Res; //滤波后的值
} lowpass_filter_t;

#define MIDVALUE_FILTER_MAX_SIZE    5 // 定义中值滤波器的最大窗口大小

#define AVGVALUE_FILTER_MAX_SAMPLES 20 //定义均值滤波器中的最大样本数量

/**
 * @brief 中值滤波器结构体
 * 
 */
typedef struct
{
    float MF_Buffer[MIDVALUE_FILTER_MAX_SIZE]; //数据缓存数组
    uint8_t MF_Wsize; //滤波器窗口大小
    uint8_t MF_Index; //当前写入位置索引
    bool MF_IsFull; //缓存区是否已满
    float MF_Res; //滤波后的值
} midvalue_filter_t;

/**
 * @brief 均值滤波器结构体
 * 
 */
typedef struct
{
    float AF_Buffer[AVGVALUE_FILTER_MAX_SAMPLES]; //数据缓存
    uint16_t AF_Size; //样本数量
    uint16_t AF_Index; //当前写入位置索引
    bool AF_IsFull; //缓存区是否满了
    float AF_Res; //滤波后的值

} average_filter_t;

/**
 * @brief 卡尔曼滤波器结构体
 * 
 */
typedef struct
{
    float KF_X; //状态估计值（滤波后的值）
    float KF_P; //估计误差协方差
    float KF_Q; //过程噪声协方差（预测过程中的不确定性）
    float KF_R; //测量噪声协方差（传感器测量的不确定性）
    float KF_K; //卡尔曼增益
    float KF_Res; //滤波结果
    bool KF_IsInit; //是否已初始化
} kalman_filter_t;

//低通滤波
bool LowPass_Filter_Init(lowpass_filter_t *filter, float index);
bool LowPass_Filter(lowpass_filter_t *filter, float input);

//中值滤波
bool MidValue_Filter_Init(midvalue_filter_t *filter, uint16_t size);
bool MidValue_Filter(midvalue_filter_t *filter, float input);

//均值滤波
bool Avg_Filter_Init(average_filter_t *filter, uint16_t size);
bool Avg_Filter(average_filter_t *filter, float input);

//卡尔曼滤波
void Kalman_Filter_Init(kalman_filter_t *filter, float q, float r, float initial_value);
bool Kalman_Filter(kalman_filter_t *filter, float measurement);

#endif
