#include "MyFilter.h"

/**
 * @brief qsort辅助函数，整型升序
 * 
 * @param value1 
 * @param value2 
 * @return int 
 */
static int _Compare_Up_Int(const void *value1, const void *value2)
{
    return (*(int *)value1 - *(int *)value2);
}

/**
 * @brief qsort辅助函数，整型降序
 * 
 * @param value1 
 * @param value2 
 * @return int 
 */
static int _Compare_Down_Int(const void *value1, const void *value2)
{
    return (*(int *)value2 - *(int *)value1);
}

/**
 * @brief qsort辅助函数，浮点数升序
 * 
 * @param value1 
 * @param value2 
 * @return int 
 */
static int _Compare_Up_Float(const void *value1, const void *value2)
{
    return (*(float *)value1 - *(float *)value2);
}

/**
 * @brief qsort辅助函数，浮点数降序
 * 
 * @param value1 
 * @param value2 
 * @return int 
 */
static int _Compare_Down_Float(const void *value1, const void *value2)
{
    return (*(float *)value2 - *(float *)value1);
}

/**
 * @brief 浮点数求和
 * 
 * @param ptr 浮点数数组指针
 * @param num 数组元素个数
 * @return float 数组元素的总和
 */
static float _SummUp_Float(float *ptr, uint16_t num)
{
    float sum = 0;
    for (uint16_t i = 0; i < num; i++)
    {
        sum += ptr[i];
    }
    return sum;
}

/**
 * @brief 初始化低通滤波器
 * 
 * @param filter 低通滤波器结构体指针
 * @param index 滤波系数 (0 < index < 1)
 * @return bool 是否初始化成功
 * 
 * @note 参数选择指南：
 *       【index滤波系数】取值范围：0.0 ~ 1.0 (不包含边界值)
 *         - 小系数(0.1~0.3): 响应快，滤波效果弱，适合动态信号
 *         - 中系数(0.4~0.6): 平衡响应速度和滤波效果
 *         - 大系数(0.7~0.9): 响应慢，滤波效果强，适合噪声大的信号
 *       
 *       【使用建议】：
 *         - 传感器去噪:       0.7~0.8  (强滤波)
 *         - 控制信号平滑:     0.3~0.5  (中等滤波)
 *         - 实时数据处理:     0.1~0.3  (轻滤波)
 *       
 *       【公式】：输出 = 新输入*(1-系数) + 历史输出*系数
 *       【特点】：计算简单，内存占用少，实时性好
 */
bool LowPass_Filter_Init(lowpass_filter_t *filter, float index)
{
    if (filter == NULL) return false;
    if (index >= 1.0f || index <= 0.0f) return false;

    filter->LF_Index = index;
    filter->LF_LastValue = 0.0f;
    filter->LF_NowValue = 0.0f;
    filter->LF_Res = 0.0f;

    return true;
}

/**
 * @brief 低通滤波
 * 
 * @param filter 低通滤波器结构体指针
 * @param input 输入的新值
 * @return bool 滤波是否成功
 * 
 * @note 使用说明：
 *       【使用步骤】：
 *         1. 先调用 LowPass_Filter_Init() 初始化
 *         2. 设置 filter->LF_NowValue = 传感器读值
 *         3. 调用 LowPass_Filter(filter)
 *         4. 从 filter->LF_Res 获取滤波结果
 *       
 *       【实时性】：计算复杂度 O(1)，适合高频调用
 *       【内存占用】：仅需存储一个历史值，内存占用最小
 */
bool LowPass_Filter(lowpass_filter_t *filter, float input)
{
    // 参数检查
    if (filter == NULL) return false;
    filter->LF_NowValue = input;
    // 低通滤波公式：输出 = 新输入值 * (1-系数) + 上次输出值 * 系数
    // 系数越大，滤波效果越强，但响应越慢
    filter->LF_Res = filter->LF_NowValue * (1 - filter->LF_Index) + filter->LF_LastValue * filter->LF_Index;

    // 更新上次的值为当前输出（为下次滤波做准备）
    filter->LF_LastValue = filter->LF_Res;

    return true;
}

/**
 * @brief 中值滤波器初始化
 * 
 * @param filter 滤波器结构体指针
 * @param size 滤波器窗口大小（建议使用奇数，不能超过MIDVALUE_FILTER_MAX_SIZE）
 * @return bool 初始化是否成功
 * 
 * @note 参数选择指南：
 *       【size窗口大小】建议取奇数值：
 *         - 小窗口(3~5):    响应快，轻度滤波，适合快变信号
 *         - 中窗口(7~11):   平衡效果，适合一般应用
 *         - 大窗口(13~21):  强滤波，响应慢，适合噪声严重的信号
 *       
 *       【适用场景】：
 *         - 脉冲噪声抑制:    窗口5~7
 *         - 图像处理:        窗口3~5
 *         - 传感器异常值:    窗口7~11
 *       
 *       【特点】：对脉冲噪声有极好的抑制效果，保持边缘信息
 *       【计算复杂度】：O(n*log(n))，n为窗口大小
 */
bool MidValue_Filter_Init(midvalue_filter_t *filter, uint16_t size)
{
    // 参数检查
    if (filter == NULL) return false;
    if (size == 0 || size > MIDVALUE_FILTER_MAX_SIZE) return false;

    // 初始化结构体成员
    filter->MF_Wsize = size;
    filter->MF_Index = 0;
    filter->MF_IsFull = false;
    filter->MF_Res = 0.0f;

    // 如果传入偶数，强行转换为奇数（中值滤波建议使用奇数窗口）
    if (filter->MF_Wsize % 2 == 0)
    {
        filter->MF_Wsize -= 1;
    }

    // 初始化缓存区，将所有元素设为0
    memset(filter->MF_Buffer, 0, sizeof(filter->MF_Buffer));

    return true; // 修复：返回值应该是true表示成功
}

/**
 * @brief 中值滤波
 * 
 * @param filter 滤波器结构体指针
 * @param input 输入值
 * @return bool 滤波是否成功
 * 
 * @note 工作原理：
 *       【算法流程】：
 *         1. 新数据存入环形缓冲区
 *         2. 缓冲区满后，复制数据到临时数组
 *         3. 对临时数组进行快速排序
 *         4. 取排序后的中位数作为输出
 *       
 *       【优势】：对突发异常值具有很强的鲁棒性
 *       【注意】：缓冲区未满时直接输出输入值
 */
bool MidValue_Filter(midvalue_filter_t *filter, float input)
{
    // 参数检查
    if (filter == NULL) return false;

    // 将新数据存入环形缓存区
    filter->MF_Buffer[filter->MF_Index] = input;

    // 更新写入位置索引
    filter->MF_Index++;

    // 检查当前窗口是否满了（环形缓冲区）
    if (filter->MF_Index >= filter->MF_Wsize)
    {
        filter->MF_Index = 0; // 回到开头
        filter->MF_IsFull = true; // 标记缓冲区已满
    }

    // 如果缓存区还没满，直接返回输入值（数据不够做中值滤波）
    if (!filter->MF_IsFull)
    {
        filter->MF_Res = input;
        return true;
    }

    // 创建一个临时数组，用于快排（避免破坏原始数据）
    float temp_array[MIDVALUE_FILTER_MAX_SIZE];

    // 只复制有效的数据长度，而不是整个缓冲区
    memcpy(temp_array, filter->MF_Buffer, filter->MF_Wsize * sizeof(float));

    // 对数据进行升序排序
    qsort(temp_array, filter->MF_Wsize, sizeof(float), _Compare_Up_Float);

    // 取中值（中间位置的元素）
    filter->MF_Res = temp_array[filter->MF_Wsize / 2];

    return true;
}

/**
 * @brief 初始化均值滤波器
 * 
 * @param filter 均值滤波器结构体指针
 * @param size 滤波器样本数量（不能超过AVGVALUE_FILTER_MAX_SAMPLES）
 * @return bool 初始化是否成功
 * 
 * @note 参数选择指南：
 *       【size样本数量】推荐范围：
 *         - 小样本(5~10):   响应快，轻度平滑，适合动态信号
 *         - 中样本(10~20):  平衡效果，适合一般传感器数据
 *         - 大样本(20~50):  强平滑，响应慢，适合稳态测量
 *       
 *       【适用场景】：
 *         - 随机噪声抑制:   样本10~20
 *         - 稳态测量:       样本20~50
 *         - 实时控制:       样本5~10
 *       
 *       【特点】：对高斯白噪声有很好的抑制效果
 *       【计算复杂度】：O(n)，n为样本数量
 */
bool Avg_Filter_Init(average_filter_t *filter, uint16_t size)
{
    // 参数检查
    if (filter == NULL) return false;
    if (size >= AVGVALUE_FILTER_MAX_SAMPLES || size <= 0) return false;

    // 初始化结构体成员
    filter->AF_Size = size; // 写入样本数量
    filter->AF_Index = 0; // 当前写入位置从0开始
    filter->AF_IsFull = false; // 缓冲区初始为空
    filter->AF_Res = 0; // 滤波结果初始为0

    // 初始化缓冲区，将所有元素设为0
    memset(filter->AF_Buffer, 0, sizeof(filter->AF_Buffer));

    return true;
}

/**
 * @brief 均值滤波
 * 
 * @param filter 均值滤波器结构体指针
 * @param input 输入值
 * @return bool 滤波是否成功
 * 
 * @note 工作原理：
 *       【算法流程】：
 *         1. 新数据存入环形缓冲区
 *         2. 缓冲区满后，计算所有样本的算术平均值
 *         3. 平均值作为滤波输出
 *       
 *       【数学公式】：输出 = (X1 + X2 + ... + Xn) / n
 *       【优势】：简单有效，对随机噪声抑制效果好
 *       【注意】：缓冲区未满时直接输出输入值
 */
bool Avg_Filter(average_filter_t *filter, float input)
{
    // 参数检查
    if (filter == NULL) return false;

    // 将新数据存入环形缓存区
    filter->AF_Buffer[filter->AF_Index] = input;

    // 更新写入位置索引
    filter->AF_Index++;

    // 判断当前缓冲区是否满了（环形缓冲区）
    if (filter->AF_Index >= filter->AF_Size)
    {
        filter->AF_Index = 0; // 回到开头
        filter->AF_IsFull = true; // 标记缓冲区已满
    }

    // 缓冲区没满，直接以输入值为最终结果（数据不够做均值滤波）
    if (filter->AF_IsFull == false)
    {
        filter->AF_Res = input;
        return true;
    }

    // 计算数组所有元素的和
    float sum = _SummUp_Float(filter->AF_Buffer, filter->AF_Size);

    // 计算均值
    filter->AF_Res = sum / (float)filter->AF_Size;

    return true;
}

/**
 * @brief 卡尔曼滤波器初始化
 * 
 * @param filter 卡尔曼滤波器结构体指针
 * @param q 过程噪声协方差（系统预测的不确定性，一般取较小值如0.001-0.1）
 * @param r 测量噪声协方差（传感器测量的不确定性，根据传感器精度设定）
 * @param initial_value 初始状态估计值
 * 
 * @note 参数选择指南：
 *       【q值(过程噪声)】推荐范围：0.001 ~ 0.1
 *         - 小值(0.001~0.01): 系统稳定，信任预测，滤波平滑但响应慢
 *         - 大值(0.05~0.1):   系统变化快，响应迅速但可能有抖动
 *       
 *       【r值(测量噪声)】根据传感器精度：
 *         - 高精度传感器:     0.01 ~ 0.1   (如精密温度传感器)
 *         - 中等精度传感器:   0.1 ~ 1.0    (如普通ADC)
 *         - 低精度传感器:     1.0 ~ 10.0   (如电位器)
 *         - GPS定位:         25 ~ 100     (单位：米²)
 *         - IMU传感器:       0.1 ~ 2.0    (加速度/陀螺仪)
 *       
 *       【initial_value初始值】选择策略：
 *         - 已知真实值:       使用实际初始状态
 *         - 传感器首次读值:   使用第一次测量值
 *         - 系统典型值:       使用常见工作状态值
 *         - 零值:            增量式系统可从0开始
 *       
 *       【调试技巧】q/r比值决定滤波特性：
 *         - q/r < 0.1:  更信任预测，滤波平滑
 *         - q/r > 0.1:  更信任测量，响应快速
 */
void Kalman_Filter_Init(kalman_filter_t *filter, float q, float r, float initial_value)
{
    // 参数检查
    if (filter == NULL) return;

    // 初始化卡尔曼滤波器参数
    filter->KF_Q = q; // 过程噪声协方差
    filter->KF_R = r; // 测量噪声协方差
    filter->KF_X = initial_value; // 初始状态估计值
    filter->KF_P = 1.0f; // 初始估计误差协方差（可以设为较大值）
    filter->KF_K = 0.0f; // 卡尔曼增益初始值
    filter->KF_Res = initial_value; // 滤波结果初始值
    filter->KF_IsInit = true; // 标记已初始化
}

/**
 * @brief 卡尔曼滤波
 * 
 * @param filter 卡尔曼滤波器结构体指针
 * @param measurement 测量值（传感器读取的原始数据）
 * @return bool 滤波是否成功
 * 
 * @note 算法原理：
 *       【五个核心步骤】：
 *         1. 状态预测：X(k|k-1) = X(k-1|k-1)
 *         2. 协方差预测：P(k|k-1) = P(k-1|k-1) + Q
 *         3. 计算增益：K(k) = P(k|k-1) / (P(k|k-1) + R)
 *         4. 状态更新：X(k|k) = X(k|k-1) + K(k) * (Z(k) - X(k|k-1))
 *         5. 协方差更新：P(k|k) = (1 - K(k)) * P(k|k-1)
 *       
 *       【算法特点】：
 *         - 最优估计：在最小均方误差意义下的最优滤波
 *         - 自适应：根据噪声情况自动调整增益
 *         - 实时性：O(1)复杂度，适合实时应用
 *         - 内存友好：只需存储少量状态变量
 */
bool Kalman_Filter(kalman_filter_t *filter, float measurement)
{
    // 参数检查
    if (filter == NULL) return false;
    if (!filter->KF_IsInit) return false;

    // === 卡尔曼滤波五个核心步骤 ===

    // 1. 预测步骤：预测当前状态（假设状态不变）
    // X(k|k-1) = X(k-1|k-1)  （状态预测，假设系统状态保持不变）
    // 这里我们假设是静态系统，所以状态预测就是上一次的估计值

    // 2. 预测步骤：更新误差协方差
    // P(k|k-1) = P(k-1|k-1) + Q  （协方差预测）
    filter->KF_P = filter->KF_P + filter->KF_Q;

    // 3. 更新步骤：计算卡尔曼增益
    // K(k) = P(k|k-1) / (P(k|k-1) + R)  （卡尔曼增益）
    filter->KF_K = filter->KF_P / (filter->KF_P + filter->KF_R);

    // 4. 更新步骤：更新状态估计
    // X(k|k) = X(k|k-1) + K(k) * (Z(k) - X(k|k-1))  （状态更新）
    // Z(k)是测量值，(Z(k) - X(k|k-1))是测量残差
    filter->KF_X = filter->KF_X + filter->KF_K * (measurement - filter->KF_X);

    // 5. 更新步骤：更新误差协方差
    // P(k|k) = (1 - K(k)) * P(k|k-1)  （协方差更新）
    filter->KF_P = (1.0f - filter->KF_K) * filter->KF_P;

    // 保存滤波结果
    filter->KF_Res = filter->KF_X;

    return true;
}