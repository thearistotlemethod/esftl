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
#include "esFtl_cache.h"
#include "esFtl_disk.h"
#include "esFtl_write.h"
#include "esFtl_bbm.h"

static uint8_t blockStatus[ESFTL_NANDNUMBLOCKS / 8];

/*
 * @brief determine bad blocks
 *
 */
void esFtl_TestForBadBlocks(void)
{
    uint8_t tmp[2] = {0};
    int i = 0;

    memset(blockStatus, 0, sizeof(blockStatus));
    for (i = 0; i < ESFTL_NANDNUMBLOCKS; i++)
    {
        tmp[0] = 0x55;
        if (esFtl_NandFlashWrite(i * ESFTL_NANDNUMPAGEBLOCK, ESFTL_NANDPAGEDATASIZE + 48, tmp, 1))
        {
            blockStatus[i / 8] |= 1 << (i % 8);
            ESFTL_LOG("Test block fail %d!!\n", i);
        }

        tmp[1] = 00;
        esFtl_NandFlashRead(i * ESFTL_NANDNUMPAGEBLOCK, ESFTL_NANDPAGEDATASIZE + 48, &tmp[1], 1);
        if (tmp[1] != 0x55)
        {
            blockStatus[i / 8] |= 1 << (i % 8);
            ESFTL_LOG("Test block fail %d!!\n", i);
        }
    }
}

/*
 * @brief ask whether the block is corrupted
 *
 * @param block
 * @return 1 if the block is corrupted
 */
int esFtl_IsBadBlock(uint16_t block)
{
    if (block >= ESFTL_NANDNUMBLOCKS)
        return 1;
    return blockStatus[block / 8] & (1 << (block % 8));
}

/*
 * @brief ask whether the page belongs to a bad block
 *
 * @param pno
 * @return 1 if the block is corrupted
 */
int esFtl_CheckIfPageInBadBlock(int pno)
{
    int bno = pno / ESFTL_NANDNUMPAGEBLOCK;
    return esFtl_IsBadBlock(bno);
}

/*
 * @brief determines corrupted pages by controling their crcs
 *
 */
void esFtl_ControlPageCorruptions(void)
{
    uint16_t sno, crc, crcTmp;
    int corruptedPages = 0, checkedPages = 0;
    uint8_t buff[ESFTL_NANDPAGEDATASIZE + 4];
    uint8_t sectorTable[ESFTL_SECTORCACHESIZE];
    int i = 0;

    memset(buff, 0, sizeof(buff));
    memset(sectorTable, 0, sizeof(sectorTable));

    if (cursorEnd != cursorStart)
    {
        for (i = cursorEnd - 1; i != cursorStart; i--)
        {
            if (i < 0)
            {
                i = ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK - 2;
            }

            memset(buff, 0, ESFTL_NANDPAGEDATASIZE + 4);
            if (!esFtl_NandFlashRead(i, ESFTL_NANDPAGEDATASIZE, &buff[ESFTL_NANDPAGEDATASIZE], 4))
            {
                memcpy(&sno, &buff[ESFTL_NANDPAGEDATASIZE], 2);
                memcpy(&crc, &buff[ESFTL_NANDPAGEDATASIZE + 2], 2);

                if (sectorTable[sno / 8] & (1 << sno % 8))
                {
                    continue;
                }
                else
                {
                    if (!esFtl_NandFlashRead(i, 0, buff, ESFTL_NANDPAGEDATASIZE))
                    {
                        crcTmp = esFtl_CalcCrc16(0xFFFF, buff, ESFTL_NANDPAGEDATASIZE);

                        if (crc != crcTmp)
                        {
                            ESFTL_LOG("Page %d is corrupted (Sector %d)\n", i, sno);
                            corruptedPages++;
                        }

                        checkedPages++;
                        sectorTable[sno / 8] |= 1 << sno % 8;
                    }
                    else
                    {
                        ESFTL_LOG("esFtl: FATAL ERROR: %d %s %d\n", i, __FILE__, __LINE__);
                    }
                }
            }
            else
            {
                ESFTL_LOG("esFtl: FATAL ERROR: %d %s %d\n", i, __FILE__, __LINE__);
            }
        }
    }

    ESFTL_LOG("%d pages are checked %d corrupted found\n", checkedPages, corruptedPages);
}

/*
 * @brief check if the page which belongs sector that comes from parameters, is corrupted
 *
 * @param sno
 * @param buff
 * @return -1 if the page is corrupted
 */
int esFtl_CheckCorruption(uint16_t sno, uint8_t *buff)
{
    int pno = 0;
    uint16_t snoTmp, crc, crcTmp;

    sno++;

    pno = esFtl_FindSectorPage(sno);
    if (pno >= 0)
    {
        memset(buff, 0, ESFTL_NANDPAGEDATASIZE + 4);
        if (!esFtl_NandFlashRead(pno, 0, buff, ESFTL_NANDPAGEDATASIZE + 4))
        {
            memcpy(&snoTmp, &buff[ESFTL_NANDPAGEDATASIZE], 2);
            if (sno == snoTmp)
            {
                memcpy(&crc, &buff[ESFTL_NANDPAGEDATASIZE + 2], 2);
                crcTmp = esFtl_CalcCrc16(0xFFFF, buff, ESFTL_NANDPAGEDATASIZE);
                if (crc != crcTmp)
                {
                    ESFTL_LOG("Page %d is corrupted (Sector %d)\n", pno, sno);
                    return -1;
                }
            }
            else
            {
                ESFTL_LOG("esFtl: FATAL ERROR: %d %s %d\n", pno, __FILE__, __LINE__);
            }
        }
        else
        {
            ESFTL_LOG("esFtl: FATAL ERROR: %d %s %d\n", pno, __FILE__, __LINE__);
        }
    }
    else
    {
        ESFTL_LOG("FtlDriverCheck %d not found\n", sno);
    }

    return 0;
}
