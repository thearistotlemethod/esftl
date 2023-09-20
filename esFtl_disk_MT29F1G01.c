/*
 *   Copyright (c) 2023 thearistotlemethod@gmail.com
 *   All rights reserved.

 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at

 *   http://www.apache.org/licenses/LICENSE-2.0

 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#include "esFtl_definitions.h"
#include "esFtl_disk.h"

#include "stm32f4xx_hal.h"
#include "spi.h"
#include "bsp.h"

#define MT29F1G01_DEVICE_ID 0x2c14
#define W25N01GV_DEVICE_ID 0xefaa
#define SE_TIMEOUT 10000

typedef enum
{
    SPI_NAND_BLKLOCK_REG_ADDR = 0xA0,
    SPI_NAND_CONFIGURATION_REG_ADDR = 0xB0,
    SPI_NAND_STATUS_REG_ADDR = 0xC0,
    SPI_NAND_DIE_SELECT_REC_ADDR = 0xD0,
} Register;

enum
{
    SPI_NAND_BLOCK_ERASE_INS = 0xD8,
    SPI_NAND_GET_FEATURE_INS = 0x0F,
    SPI_NAND_PAGE_READ_INS = 0x13,
    SPI_NAND_PROGRAM_EXEC_INS = 0x10,
    SPI_NAND_PROGRAM_LOAD_INS = 0x02,
    SPI_NAND_PROGRAM_LOAD_RANDOM_INS = 0x84,
    SPI_NAND_READ_CACHE_INS = 0x03,
    SPI_NAND_READ_CACHE_X2_INS = 0x3B, // dual wire I/O
    SPI_NAND_READ_CACHE_X4_INS = 0x6B, // quad wire I/O
    SPI_NAND_READ_ID = 0x9F,
    SPI_NAND_RESET = 0xFF,
    SPI_NAND_SET_FEATURE = 0x1F,
    SPI_NAND_WRITE_DISABLE = 0x04,
    SPI_NAND_WRITE_ENABLE = 0x06
};

enum
{
    SPI_NAND_CRBSY = 0x80, // Cache read busy
    SPI_NAND_ECCS2 = 0x40, // ECC status register 0
    SPI_NAND_ECCS1 = 0x20, // ECC status register 0
    SPI_NAND_ECCS0 = 0x10, // ECC status register 0
    SPI_NAND_PF = 0x08,    /* program fail */
    SPI_NAND_EF = 0x04,    /* erase fail */
    SPI_NAND_WE = 0x02,    /* write enable */
    SPI_NAND_OIP = 0x01,   /* operation in progress */
};

typedef struct _structCharStream
{
    uint8_t *pChar;
    uint32_t length;
} CharStream;

static uint32_t DeviceId = 0;

static int FlashSetFeature(Register ucRegAddr, uint8_t ucpRegValue);
static int FlashReset(void);
static int Serialize_SPI(const CharStream *char_stream_send, CharStream *char_stream_recv, unsigned char cs);
static int IsFlashBusy(void);
static void SPI_NAND_Select(void);
static void SPI_NAND_Deselect(void);
static int WAIT_EXECUTION_COMPLETE(uint32_t m_second);
static int FlashReadStatusRegister(uint8_t *ucpStatusRegister);
static int FlashReadDeviceIdentification(uint16_t *uwpDeviceIdentification);
static void Set_Column_Stream(uint32_t page_id, uint16_t offset, uint8_t cCMD, uint8_t *chars);
static void Set_Row_Stream(uint32_t page_id, uint8_t cCMD, uint8_t *chars);
static int FlashWriteEnable(void);
static int FlashUnlockAll(void);

/*
 * @brief initialize the MT29F1G01 chip
 *
 * @return 0 if it is successful
 */
int esFtl_NandFlashInit(void)
{
    uint16_t NandId;
    FlashReset();
    FlashReadDeviceIdentification(&NandId);
    if ((NandId != MT29F1G01_DEVICE_ID) && (NandId != W25N01GV_DEVICE_ID))
        return -1;

    DeviceId = NandId;
    FlashSetFeature(SPI_NAND_BLKLOCK_REG_ADDR, 0);
    if (NandId == W25N01GV_DEVICE_ID)
        FlashSetFeature(SPI_NAND_CONFIGURATION_REG_ADDR, 0x08);
    else
        FlashSetFeature(SPI_NAND_CONFIGURATION_REG_ADDR, 0);
    FlashSetFeature(SPI_NAND_STATUS_REG_ADDR, 0);
    FlashSetFeature(SPI_NAND_DIE_SELECT_REC_ADDR, 0);

    if (FlashUnlockAll() != 0)
        return -2;
    return 0;
}

/*
 * @brief fetch data from the chip
 *
 * @param page
 * @param offset
 * @param buff
 * @param count
 * @return 0 if it is successful
 */
int esFtl_NandFlashRead(uint32_t page, uint32_t offset, uint8_t *buff, uint32_t count)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    uint8_t chars[4];
    uint8_t cReadFromCacheCMD;

    if ((page) >= (ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK))
        return -1;

    Set_Row_Stream(page, SPI_NAND_PAGE_READ_INS, chars);
    char_stream_send.length = 4;
    char_stream_send.pChar = chars;

    Serialize_SPI(&char_stream_send, NULL, 1);

    WAIT_EXECUTION_COMPLETE(SE_TIMEOUT);

    cReadFromCacheCMD = SPI_NAND_READ_CACHE_INS;

    Set_Column_Stream(page, offset, cReadFromCacheCMD, chars);

    char_stream_send.length = 4;
    char_stream_send.pChar = chars;
    char_stream_recv.length = count;
    char_stream_recv.pChar = buff;

    Serialize_SPI(&char_stream_send, &char_stream_recv, 1);
    return 0;
}

/*
 * @brief store data to the chip
 *
 * @param page
 * @param offset
 * @param buff
 * @param count
 * @return 0 if it is successful
 */
int esFtl_NandFlashWrite(uint32_t page, uint32_t offset, const uint8_t *buff, uint32_t count)
{
    CharStream char_stream_send;
    uint8_t chars[4] = {0};
    uint8_t status_reg = 0;

    if ((page) >= (ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK))
        return -1;

    if (IsFlashBusy())
        return -2;

    FlashWriteEnable();

    SPI_NAND_Select();
    Set_Column_Stream(page, offset, SPI_NAND_PROGRAM_LOAD_INS, chars);

    char_stream_send.length = 3;
    char_stream_send.pChar = chars;

    Serialize_SPI(&char_stream_send, NULL, 0);

    char_stream_send.length = count;
    char_stream_send.pChar = (uint8_t *)buff;

    Serialize_SPI(&char_stream_send, NULL, 0);
    SPI_NAND_Deselect();

    Set_Row_Stream(page, SPI_NAND_PROGRAM_EXEC_INS, chars);
    char_stream_send.length = 4;
    char_stream_send.pChar = chars;

    Serialize_SPI(&char_stream_send, NULL, 1);

    WAIT_EXECUTION_COMPLETE(SE_TIMEOUT);

    FlashReadStatusRegister(&status_reg);
    if (status_reg & SPI_NAND_PF)
        return -3;

    return 0;
}

/*
 * @brief reset a block to be ready to store data
 *
 * @param block
 * @return 0 if it is successful
 */
int esFtl_NandFlashBlockErase(uint32_t block)
{
    CharStream char_stream_send;
    uint8_t chars[4];
    uint8_t status_reg;

    if (block >= ESFTL_NANDNUMBLOCKS)
        return -1;

    block = block * ESFTL_NANDNUMPAGEBLOCK;

    if (IsFlashBusy())
        return -2;

    FlashWriteEnable();
    Set_Row_Stream(block, SPI_NAND_BLOCK_ERASE_INS, chars);

    char_stream_send.length = 4;
    char_stream_send.pChar = chars;

    Serialize_SPI(&char_stream_send, NULL, 1);

    WAIT_EXECUTION_COMPLETE(SE_TIMEOUT);
    FlashReadStatusRegister(&status_reg);
    if (status_reg & SPI_NAND_EF)
        return -3;

    return 0;
}

static int FlashSetFeature(Register ucRegAddr, uint8_t ucpRegValue)
{
    CharStream char_stream_send;
    uint8_t chars[3];

    // Step 1: Check the register address
    if ((ucRegAddr != SPI_NAND_BLKLOCK_REG_ADDR) &&
        (ucRegAddr != SPI_NAND_CONFIGURATION_REG_ADDR) &&
        (ucRegAddr != SPI_NAND_STATUS_REG_ADDR) &&
        (ucRegAddr != SPI_NAND_DIE_SELECT_REC_ADDR))
        return -1;
    else
    {
        chars[0] = (uint8_t)SPI_NAND_SET_FEATURE;
        chars[1] = (uint8_t)ucRegAddr;
        chars[2] = (uint8_t)ucpRegValue;

        char_stream_send.length = 3;
        char_stream_send.pChar = chars;

        Serialize_SPI(&char_stream_send, NULL, 1);
        if (WAIT_EXECUTION_COMPLETE(SE_TIMEOUT) == 0)
            return 0;
        else
            return -2;
    }
}

static int FlashReset(void)
{
    CharStream char_stream_send;
    uint8_t cRST = SPI_NAND_RESET;

    char_stream_send.length = 1;
    char_stream_send.pChar = &cRST;

    Serialize_SPI(&char_stream_send, NULL, 1);
    HAL_Delay(250);
    WAIT_EXECUTION_COMPLETE(SE_TIMEOUT);
    return 0;
}

static int IsFlashBusy(void)
{
    uint8_t ucSR;

    FlashReadStatusRegister(&ucSR);
    if (ucSR & SPI_NAND_OIP)
        return 1;
    else
        return 0;
}

#define FLASH_CS_Pin GPIO_PIN_0
#define FLASH_CS_GPIO_Port GPIOE

static void SPI_NAND_Select(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET);
}

static void SPI_NAND_Deselect(void)
{
    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);
}

static int Serialize_SPI(const CharStream *char_stream_send, CharStream *char_stream_recv, unsigned char cs)
{
    uint8_t *char_send, *char_recv;
    uint16_t rx_len = 0, tx_len = 0;
    tx_len = char_stream_send->length;
    char_send = char_stream_send->pChar;
    if (cs)
        SPI_NAND_Select();
    MX_SPI_Transmit(SPI_BAUDRATEPRESCALER_2, char_send, tx_len, 500);
    if (NULL != char_stream_recv)
    {
        rx_len = char_stream_recv->length;
        char_recv = char_stream_recv->pChar;
        MX_SPI_Receive(SPI_BAUDRATEPRESCALER_2, char_recv, rx_len, 500);
    }
    if (cs)
        SPI_NAND_Deselect();
    return 0;
}

static int WAIT_EXECUTION_COMPLETE(uint32_t m_second)
{
    int dwtTime = DWT_GetUs();
    int Timeout = 0;
    while (1)
    {
        if (abs(DWT_GetUs() - dwtTime) > 1000)
        {
            dwtTime = DWT_GetUs();
            Timeout++;
        }

        if (!IsFlashBusy())
            break;
        DWT_Delay(1);

        if (Timeout > m_second)
        {
            return -1;
        }
    }
    return 0;
}

static int FlashReadStatusRegister(uint8_t *ucpStatusRegister)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    uint8_t chars[2];

    chars[0] = (uint8_t)SPI_NAND_GET_FEATURE_INS;
    chars[1] = (uint8_t)SPI_NAND_STATUS_REG_ADDR;

    // Step 1: Initialize the data (i.e. Instruction) packet to be sent serially
    char_stream_send.length = 2;
    char_stream_send.pChar = chars;
    char_stream_recv.length = 1;
    char_stream_recv.pChar = ucpStatusRegister;

    // Step 2: Send the packet serially, get the Status Register content
    Serialize_SPI(&char_stream_send, &char_stream_recv, 1);

    return 0;
}

static int FlashReadDeviceIdentification(uint16_t *uwpDeviceIdentification)
{
    CharStream char_stream_send;
    CharStream char_stream_recv;
    uint8_t chars[2];
    uint8_t pIdentification[2];

    chars[0] = (uint8_t)SPI_NAND_READ_ID;
    chars[1] = 0x00; /* dummy byte */

    // Step 1: Initialize the data (i.e. Instruction) packet to be sent serially
    char_stream_send.length = 2;
    char_stream_send.pChar = chars;
    char_stream_recv.length = 2;
    char_stream_recv.pChar = &pIdentification[0];

    // Step 2: Send the packet serially
    Serialize_SPI(&char_stream_send, &char_stream_recv, 1);

    // Step 3: Device Identification is returned ( memory type + memory capacity )
    *uwpDeviceIdentification = char_stream_recv.pChar[0];
    *uwpDeviceIdentification <<= 8;
    *uwpDeviceIdentification |= char_stream_recv.pChar[1];

    return 0;
}

static void Set_Column_Stream(uint32_t page_id, uint16_t offset, uint8_t cCMD, uint8_t *chars)
{
    chars[0] = (uint8_t)cCMD;
    chars[1] = (uint8_t)((offset & 0xff00) >> 8);
    chars[1] |= (uint8_t)(((page_id >> 6) & 0x1) << 4);
    chars[2] = (uint8_t)(offset & 0x00ff);
    chars[3] = (uint8_t)(0xff); /* ! */
}

static void Set_Row_Stream(uint32_t page_id, uint8_t cCMD, uint8_t *chars)
{
    chars[0] = (uint8_t)cCMD;
    chars[1] = (uint8_t)((page_id & 0xff0000) >> 16);
    chars[2] = (uint8_t)((page_id & 0xff00) >> 8);
    chars[3] = (uint8_t)(page_id & 0x00ff);
}

static int FlashWriteEnable(void)
{
    CharStream char_stream_send;
    uint8_t cWREN = SPI_NAND_WRITE_ENABLE;
    uint8_t ucSR;

    char_stream_send.length = 1;
    char_stream_send.pChar = &cWREN;

    Serialize_SPI(&char_stream_send, NULL, 1);

    do
    {
        FlashReadStatusRegister(&ucSR);
    } while (~ucSR & SPI_NAND_WE);

    return 0;
}

static int FlashUnlockAll(void)
{
    CharStream char_stream_send;
    uint8_t chars[3];

    chars[0] = (uint8_t)SPI_NAND_SET_FEATURE;
    chars[1] = (uint8_t)SPI_NAND_BLKLOCK_REG_ADDR;
    chars[2] = (uint8_t)0x00;

    char_stream_send.length = 3;
    char_stream_send.pChar = chars;

    Serialize_SPI(&char_stream_send, NULL, 1);

    return 0;
}
