#ifndef __GD25Q40E_h
#define __GD25Q40E_h

#include "SysConfig.h"

#ifdef __cplusplus
}
#endif

#define GD25Q40E_SPI &hspi1

/* GD25Q40E 命令定义 */
#define GD25Q40E_CMD_WRITE_ENABLE       0x06 /* 写使能指令 */
#define GD25Q40E_CMD_WRITE_DISABLE      0x04 /* 写禁止指令 */
#define GD25Q40E_CMD_READ_STATUS_REG    0x05 /* 读状态寄存器指令 */
#define GD25Q40E_CMD_WRITE_STATUS_REG   0x01 /* 写状态寄存器指令 */
#define GD25Q40E_CMD_READ_DATA          0x03 /* 读数据指令 */
#define GD25Q40E_CMD_FAST_READ          0x0B /* 快速读指令 */
#define GD25Q40E_CMD_DUAL_READ          0x3B /* 双倍速读指令 */
#define GD25Q40E_CMD_QUAD_READ          0x6B /* 四倍速读指令 */
#define GD25Q40E_CMD_PAGE_PROGRAM       0x02 /* 页编程指令 */
#define GD25Q40E_CMD_SECTOR_ERASE       0x20 /* 扇区擦除指令 (4KB) */
#define GD25Q40E_CMD_BLOCK_ERASE_32K    0x52 /* 块擦除指令 (32KB) */
#define GD25Q40E_CMD_BLOCK_ERASE_64K    0xD8 /* 块擦除指令 (64KB) */
#define GD25Q40E_CMD_CHIP_ERASE         0xC7 /* 芯片擦除指令 */
#define GD25Q40E_CMD_READ_REMS          0x90 /* 读取设备ID、制造商ID */
#define GD25Q40E_CMD_READ_ID            0x9F /* 读取ID指令 */
#define GD25Q40E_CMD_READ_UNIQUE_ID     0x4B /* 读取唯一ID指令 */
#define GD25Q40E_CMD_POWER_DOWN         0xB9 /* 掉电模式指令 */
#define GD25Q40E_CMD_RELEASE_POWER_DOWN 0xAB /* 唤醒指令 */

/* GD25Q40E 特性 */
#define GD25Q40E_PAGE_SIZE      256 /* 页大小 256字节 */
#define GD25Q40E_SECTOR_SIZE    4096 /* 扇区大小 4KB */
#define GD25Q40E_BLOCK_SIZE_32K 32768 /* 32KB块大小 */
#define GD25Q40E_BLOCK_SIZE_64K 65536 /* 64KB块大小 */
#define GD25Q40E_CHIP_SIZE      (512 * 1024) /* 芯片容量 512KB (4Mbit) */

/* GD25Q40E 状态寄存器位定义 */
#define GD25Q40E_STATUS_WIP 0x01 /* 写入进行中 (Write In Progress) */
#define GD25Q40E_STATUS_WEL 0x02 /* 写使能锁存 (Write Enable Latch) */

#define DUMMY               0xFF /* DUMMY字节 */

uint32_t GD25Q40E_Init(void);
void GD25Q40E_ReadBytes(uint8_t *rxdata, uint32_t addr, uint32_t num);
void GD25Q40E_FastReadByte(uint8_t *rxdata, uint32_t addr, uint32_t num);
void GD25Q40E_PageProgram(uint8_t *txdata, uint32_t addr, uint16_t num);
void GD25Q40E_WriteData(uint8_t *txdata, uint32_t addr, uint32_t size);
void GD25Q40E_EraseSector(uint32_t addr);
void GD25Q40E_EraseBlock32K(uint32_t addr);
void GD25Q40E_EraseBlock64K(uint32_t addr);
void GD25Q40E_EraseChip(void);
void GD25Q40E_PowerDown(void);
void GD25Q40E_WakeUp(void);
uint16_t GD25Q40E_ReadREMS(void);
uint32_t GD25Q40E_ReadID(void);
uint16_t GD25Q40E_ReadUniqueID(void);

#ifdef __cplusplus
}
#endif

#endif
