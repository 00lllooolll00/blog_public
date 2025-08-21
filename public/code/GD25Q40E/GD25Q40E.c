/**
 * @file GD25Q40E.c
 * @author N1netyNine99
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * @note 本方案为硬件SPI方案，使用的时候需要移植 Delay_1ms宏 WTRITE_PIN 宏和 GD25Q40E_TransmitReceiveData函数
 *       .h文件中也要GD25Q40E_SPI为你使用的句柄，同时FLASH_SPI_PORT和FLASH_SPI_SS_SF分别是片选的Port
 *       和对应的Pin
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "GD25Q40E.h"

#define WTRITE_PIN(__PORT__, __PIN__, __VALUE) HAL_GPIO_WritePin(__PORT__, __PIN__, __VALUE)
#define Delay_1ms(X)                           HAL_Delay(X)

/* ========================= 宏定义区 ========================= */
#define GD25Q40E_SELECT()    WTRITE_PIN(FLASH_SPI_PORT, FLASH_SPI_SS_SF, 0) //选中芯片
#define GD25Q40E_DISSELECT() WTRITE_PIN(FLASH_SPI_PORT, FLASH_SPI_SS_SF, 1) //取消选择
#define ADDR_HIGH_8(X)       (((X) & 0xFF0000) >> 16) //将24位的高8位左移到低8位变成uint8_t发送
#define ADDR_MID_8(X)        (((X) & 0x00FF00) >> 8) //将24位的中8位左移到低8位变成uint8_t发送
#define ADDR_LOW_8(X)        ((X) & 0x0000FF) //获取地址的低8位

/* ========================= 基础通信函数 ========================= */
/**
 * @brief SPI数据收发函数
 * @param txdata 要发送的数据
 * @return 接收到的数据
 * @note 发送一个字节数据的同时接收一个字节数据
 */
static uint8_t GD25Q40E_TransmitReceiveData(uint8_t txdata)
{
    uint8_t rxdata = 0;
    //修改为你自己的SPI交换数据函数
    HAL_SPI_TransmitReceive(GD25Q40E_SPI, txdata, rxdata, sizeof(txdata), HAL_MAX_DELAY);
    return rxdata;
}

/* ========================= 初始化函数 ========================= */
/**
 * @brief 初始化GD25Q40E Flash
 * @return 0: 初始化成功，非0: 初始化失败
 * @note 该函数会检查Flash的ID，确保Flash正常工作
 */
uint32_t GD25Q40E_Init(void)
{
    uint32_t deviceID = 0;

    // 确保片选引脚初始为高电平（不选中）
    GD25Q40E_DISSELECT();

    // 短暂延时，等待Flash上电稳定
    Delay_1ms(10);

    // 唤醒Flash，确保不在掉电模式
    GD25Q40E_WakeUp();

    // 读取Flash ID
    deviceID = GD25Q40E_ReadID();

    return deviceID;
}

/* ========================= 写使能控制函数 ========================= */
/**
 * @brief 使能Flash写操作
 * @note 在进行任何写入或擦除操作前必须先调用此函数
 *       写使能位(WEL)将在每次写入或擦除操作后自动清除
 */
static void GD25Q40E_WriteEnable(void)
{
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_WRITE_ENABLE);
    GD25Q40E_DISSELECT();
}

/**
 * @brief 禁止Flash写操作
 * @note 清除写使能位(WEL)，防止意外写入
 */
static void GD25Q40E_WriteDisable(void)
{
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_WRITE_DISABLE);
    GD25Q40E_DISSELECT();
}

/* ========================= 状态查询与等待函数 ========================= */
/**
 * @brief 读取Flash状态寄存器
 * @return 状态寄存器的值
 * @note 状态寄存器包含WIP(写入进行中)和WEL(写使能锁存)等标志位
 */
static uint8_t GD25Q40E_ReadStatusReg(void)
{
    uint8_t State = 0;
    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_READ_STATUS_REG);
    //不能发送这个数据之后直接返回状态值 因为flash此时没有主备好数据
    State = GD25Q40E_TransmitReceiveData(DUMMY);
    //发送一个dummy之后Flash会返回状态数据，dummy为0xFF，全高电平，不会影响时序
    GD25Q40E_DISSELECT();
    return State;
}

/**
 * @brief 等待Flash空闲
 * @note 轮询状态寄存器WIP位，直到Flash完成当前操作
 *       所有写入或擦除操作都需要等待完成
 */
static void GD25Q40E_Wait(void)
{
    uint8_t temp = 0;
    do
    {
        temp = GD25Q40E_ReadStatusReg();
        // WIP=1表示Flash正在执行内部写入操作
    } while (temp & GD25Q40E_STATUS_WIP);
}

/* ========================= 数据读取函数 ========================= */
/**
 * @brief 读取Flash中的数据
 * @param rxdata 接收数据的缓冲区
 * @param addr 读取的起始地址 (0x00000 - 0x7FFFF)
 * @param num 读取的字节数
 * @note 支持跨页读取，无需等待
 */
void GD25Q40E_ReadBytes(uint8_t *rxdata, uint32_t addr, uint32_t num)
{
    // 地址边界检查
    if (addr >= GD25Q40E_CHIP_SIZE || (addr + num) > GD25Q40E_CHIP_SIZE || rxdata == NULL || num == 0)
    {
        return; // 地址越界或参数无效
    }

    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_READ_DATA);

    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));
    do
    {
        *rxdata++ = GD25Q40E_TransmitReceiveData(DUMMY);
    } while (--num);

    GD25Q40E_DISSELECT();
}

/**
 * @brief 快速读取Flash中的数据
 * @param rxdata 接收数据的缓冲区
 * @param addr 读取的起始地址 (0x00000 - 0x7FFFF)
 * @param num 读取的字节数
 * @note 比普通读取速度更快，支持更高的时钟频率
 */
void GD25Q40E_FastReadByte(uint8_t *rxdata, uint32_t addr, uint32_t num)
{
    // 地址边界检查
    if (addr >= GD25Q40E_CHIP_SIZE || (addr + num) > GD25Q40E_CHIP_SIZE || rxdata == NULL || num == 0)
    {
        return; // 地址越界或参数无效
    }

    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_FAST_READ);

    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));

    GD25Q40E_TransmitReceiveData(DUMMY); //发送一个无效字节
    do
    {
        *rxdata++ = GD25Q40E_TransmitReceiveData(DUMMY);
    } while (--num);

    GD25Q40E_DISSELECT();
}

/* ========================= 数据写入函数 ========================= */
/**
 * @brief 页编程
 * @param txdata 要写入的数据
 * @param addr 写入的起始地址 (0x00000 - 0x7FFFF)
 * @param num 写入的字节数
 * @note 页编程一次最多写入256字节，不能跨页边界
 *       写入前必须确保目标区域已擦除(0xFF状态)
 */
void GD25Q40E_PageProgram(uint8_t *txdata, uint32_t addr, uint16_t num)
{
    // 参数检查
    if (addr >= GD25Q40E_CHIP_SIZE || txdata == NULL || num == 0)
    {
        return; // 地址越界或参数无效
    }

    if (num > GD25Q40E_PAGE_SIZE)
    {
        num = GD25Q40E_PAGE_SIZE; // 限制在页大小内
    }

    // 检查是否跨页边界
    uint32_t page_boundary = (addr / GD25Q40E_PAGE_SIZE + 1) * GD25Q40E_PAGE_SIZE;
    if ((addr + num) > page_boundary)
    {
        num = page_boundary - addr; // 限制在当前页内
    }

    GD25Q40E_WriteEnable(); //写使能内部也有对CS拉高拉低，所以要放到这里CS操作之前以防止影响时序
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_PAGE_PROGRAM);

    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));

    do
    {
        GD25Q40E_TransmitReceiveData(*txdata++);
    } while (--num);
    GD25Q40E_DISSELECT();

    GD25Q40E_Wait();
}

/**
 * @brief 写入数据
 * @param txdata 要写入的数据
 * @param addr 写入的起始地址 (0x00000 - 0x7FFFF)
 * @param size 写入的字节数
 * @note 支持跨页写入，自动处理页边界
 *       写入前必须确保目标区域已擦除(0xFF状态)
 */
void GD25Q40E_WriteData(uint8_t *txdata, uint32_t addr, uint32_t size)
{
    // 参数检查
    if (addr >= GD25Q40E_CHIP_SIZE || (addr + size) > GD25Q40E_CHIP_SIZE || txdata == NULL || size == 0)
    {
        return; // 地址越界或参数无效
    }

    uint32_t remain;
    remain = GD25Q40E_PAGE_SIZE - (addr % GD25Q40E_PAGE_SIZE); //当前页数中的剩余字节数

    if (size <= remain) //如果要写入的数据字节数小于剩余的字节数，直接写入size个字节到当前页剩余空间即可
    {
        remain = size;
    }

    do
    {
        GD25Q40E_PageProgram(txdata, addr, remain); //先写入数据到当前页空闲的地方

        size -= remain; //更新未写的数据字节数，为下一次写入做准备
        addr += remain; //更新地址到下一次写入的位置
        txdata += remain; //更新待写入到数据到下一次要写入的数据

        remain = (size > GD25Q40E_PAGE_SIZE) ? GD25Q40E_PAGE_SIZE : size; //如果剩余的数据还大于一页，下一次也只能写一页

    } while (size > 0); //只要size还有值代表还有数据没写入，继续循环写入数据
}

/* ========================= 擦除函数 ========================= */
/**
 * @brief 擦除扇区
 * @param addr 扇区地址 (应该是4KB对齐的地址)
 * @note 擦除4KB扇区，将所有字节设置为0xFF
 *       扇区擦除时间约20-200ms
 */
void GD25Q40E_EraseSector(uint32_t addr)
{
    // 地址边界检查
    if (addr >= GD25Q40E_CHIP_SIZE)
    {
        return; // 地址越界
    }

    GD25Q40E_WriteEnable();
    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_SECTOR_ERASE);
    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));

    GD25Q40E_DISSELECT();
    GD25Q40E_Wait();
}

/**
 * @brief 擦除32K块
 * @param addr 块地址
 */
void GD25Q40E_EraseBlock32K(uint32_t addr)
{
    GD25Q40E_WriteEnable();
    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_BLOCK_ERASE_32K);
    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));

    GD25Q40E_DISSELECT();
    GD25Q40E_Wait();
}

/**
 * @brief 擦除64K块
 * @param addr 块地址
 */
void GD25Q40E_EraseBlock64K(uint32_t addr)
{
    GD25Q40E_WriteEnable();
    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_BLOCK_ERASE_64K);
    GD25Q40E_TransmitReceiveData(ADDR_HIGH_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_MID_8(addr));
    GD25Q40E_TransmitReceiveData(ADDR_LOW_8(addr));

    GD25Q40E_DISSELECT();
    GD25Q40E_Wait();
}

/**
 * @brief 擦除整个芯片
 */
void GD25Q40E_EraseChip(void)
{
    GD25Q40E_WriteEnable();
    GD25Q40E_SELECT();

    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_CHIP_ERASE);

    GD25Q40E_DISSELECT();
    GD25Q40E_Wait();
}

/* ========================= 电源管理函数 ========================= */
/**
 * @brief 进入掉电模式
 */
void GD25Q40E_PowerDown(void)
{
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_POWER_DOWN);
    GD25Q40E_DISSELECT();
}

/**
 * @brief 退出掉电模式
 */
void GD25Q40E_WakeUp(void)
{
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_RELEASE_POWER_DOWN);
    GD25Q40E_DISSELECT();
}

/* ========================= 设备信息读取函数 ========================= */
/**
 * @brief 读取REMS ID
 * @return REMS ID
 */
uint16_t GD25Q40E_ReadREMS(void)
{
    uint8_t REMS_ID_High = 0;
    uint8_t REMS_ID_Low = 0;
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_READ_REMS);
    GD25Q40E_TransmitReceiveData(0x00);
    GD25Q40E_TransmitReceiveData(0x00);
    GD25Q40E_TransmitReceiveData(0x00);
    REMS_ID_High = GD25Q40E_TransmitReceiveData(DUMMY);
    REMS_ID_Low = GD25Q40E_TransmitReceiveData(DUMMY);

    GD25Q40E_DISSELECT();

    return (REMS_ID_High << 8 | REMS_ID_Low);
}

/**
 * @brief 读取设备ID
 * @return 设备ID
 * @note 设备ID格式：[制造商ID(8bit)][存储类型ID(8bit)][容量ID(8bit)]
 *       GD25Q40E预期ID: 0xC84013
 */
uint32_t GD25Q40E_ReadID(void)
{
    uint8_t Manufacturer_ID = 0;
    uint8_t MemoryType_ID = 0;
    uint8_t Capacity_ID = 0;
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_READ_ID);

    Manufacturer_ID = GD25Q40E_TransmitReceiveData(DUMMY);
    MemoryType_ID = GD25Q40E_TransmitReceiveData(DUMMY);
    Capacity_ID = GD25Q40E_TransmitReceiveData(DUMMY);

    GD25Q40E_DISSELECT();
    return ((Manufacturer_ID << 16) | (MemoryType_ID << 8) | (Capacity_ID));
}

/**
 * @brief 读取唯一ID
 * @return 唯一ID (16位)
 * @note 读取Flash芯片的唯一标识符
 */
uint16_t GD25Q40E_ReadUniqueID(void)
{
    uint8_t Unique_ID_High = 0;
    uint8_t Unique_ID_Low = 0;
    GD25Q40E_SELECT();
    GD25Q40E_TransmitReceiveData(GD25Q40E_CMD_READ_UNIQUE_ID);
    GD25Q40E_TransmitReceiveData(0x00);
    GD25Q40E_TransmitReceiveData(0x00);
    GD25Q40E_TransmitReceiveData(0x00);

    GD25Q40E_TransmitReceiveData(DUMMY);
    Unique_ID_High = GD25Q40E_TransmitReceiveData(DUMMY);
    Unique_ID_Low = GD25Q40E_TransmitReceiveData(DUMMY);

    GD25Q40E_DISSELECT();

    return ((Unique_ID_High << 8) | Unique_ID_Low);
}
