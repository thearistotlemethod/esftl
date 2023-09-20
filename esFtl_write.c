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
#include "esFtl_cache.h"
#include "esFtl_defragment.h"
#include "esFtl_bbm.h"
#include "esFtl_write.h"

static int CheckIfDefragmentNeeded(void);

/*
 * @brief read the sector data to a page
 *
 * @param sno
 * @param buffer
 * @param idx
 * @param count
 * @return 0
 */
int esFtl_FtlDriverWrite(uint16_t sno, uint8_t *buffer, uint32_t idx, uint32_t count)
{
    uint16_t crc;

    sno++;

    crc = esFtl_CalcCrc16(0xFFFF, (unsigned char *)buffer, ESFTL_NANDPAGEDATASIZE);

    memcpy(&buffer[ESFTL_NANDPAGEDATASIZE], &sno, 2);
    memcpy(&buffer[ESFTL_NANDPAGEDATASIZE + 2], &crc, 2);

    while (1)
    {
        if (esFtl_CheckIfPageInBadBlock(cursorEnd))
        {
            esFtl_IncrementCursorEnd();
            continue;
        }

        if (esFtl_NandFlashWrite(cursorEnd, 0, buffer, ESFTL_NANDPAGEDATASIZE + 4))
        {
            esFtl_IncrementCursorEnd();

            ESFTL_LOG("esFtl: FATAL ERROR:%d %s %d\n", cursorEnd, __FILE__, __LINE__);
        }
        else
        {
            esFtl_SetSectorCache(sno, cursorEnd);
            esFtl_IncrementCursorEnd();

            if (lastOpSectorNo < sno)
                lastOpSectorNo = sno;
            break;
        }
    }

    defragmentNeeded = CheckIfDefragmentNeeded();
    return 0;
}

/*
 * @brief mark the page as released in order to get it return to the system
 *
 * @param sno
 * @return 0
 */
int esFtl_FtlDriverRelease(uint16_t sno)
{
    int pno = 0;
    uint8_t markedAsReleasedByte = 0xF0;

    sno++;

    pno = esFtl_FindSectorPage(sno);
    if (pno >= 0)
    {
        if (esFtl_NandFlashWrite(pno, ESFTL_NANDPAGEDATASIZE + 4, &markedAsReleasedByte, 1))
        {
            ESFTL_LOG("esFtl: FATAL ERROR:%d %s %d\n", pno, __FILE__, __LINE__);
        }
        else
        {
            esFtl_SetSectorCache(sno, 0xFFFF);
        }
    }
    else
    {
        ESFTL_LOG("FtlDriverRelease %d not found\n", sno);
    }

    return 0;
}

/*
 * @brief calculate crc
 *
 * @param crc
 * @param data_p
 * @param length
 * @return value of the crc
 */
uint16_t esFtl_CalcCrc16(uint16_t crc, uint8_t *data_p, uint32_t length)
{
    uint8_t x;

    while (length--)
    {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
    }
    return crc;
}

/*
 * @brief check if the system reaches the limit for the defragmentation
 *
 * @return 1 if it is true
 */
static int CheckIfDefragmentNeeded(void)
{
    int freePages = esFtl_CalcFreePages();

    if (freePages < ESFTL_FREEBLOCKLIMITFORDEFRAGMENT * ESFTL_NANDNUMPAGEBLOCK)
    {
        return 1;
    }

    return 0;
}
