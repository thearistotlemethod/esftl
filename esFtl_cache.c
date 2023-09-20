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
#include "esFtl_read.h"
#include "esFtl_cache.h"

typedef struct
{
    uint16_t sno;
    uint16_t crc;
    uint8_t released;
    uint8_t firstBlock;
} SpareData;

static uint16_t sectorCache[ESFTL_SECTORCACHESIZE];
int cursorEnd = 0;
int cursorStart = 0;
uint16_t lastOpSectorNo = 0;
uint8_t defragmentNeeded = 0;

/*
 * @brief ask whether the defragment is necessary
 *
 * @return 1 if defragment is necessary
 */
uint8_t esFtl_IsDefragNeeded(void)
{
    return defragmentNeeded;
}

/*
 * @brief determine the cursor points and fill the cache data
 *
 */
void esFtl_EvaluateCursorAndCache(void)
{
    SpareData sData;
    uint16_t pno = 0;
    int i = 0;

    memset(sectorCache, 0xFF, sizeof(sectorCache));

    for (i = 0; i < ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK; i++)
    {
        if (!esFtl_NandFlashRead(i, ESFTL_NANDPAGEDATASIZE, (uint8_t *)&sData, sizeof(SpareData)))
        {
            if (i % ESFTL_NANDNUMPAGEBLOCK == 0 && sData.firstBlock == 0x55)
            {
                cursorStart = i;
                break;
            }
        }
        else
        {
            ESFTL_LOG("esFTL: FATAL ERROR: %d %s %d\n", i, __FILE__, __LINE__);
        }
    }

    for (i = 0; i < ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK; i++)
    {
        pno = (cursorStart + i) % (ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK);
        if (pno == 0xFFFF)
        {
            continue;
        }

        if (!esFtl_NandFlashRead(pno, ESFTL_NANDPAGEDATASIZE, (uint8_t *)&sData, sizeof(SpareData) - 1))
        {
            if (sData.sno == 0xFFFF)
            {
                cursorEnd = pno;
                break;
            }
            else
            {
                if (sData.released == 0xFF)
                {
                    esFtl_SetSectorCache(sData.sno, pno);

                    if (lastOpSectorNo < sData.sno)
                        lastOpSectorNo = sData.sno;
                }
                else
                {
                    esFtl_SetSectorCache(sData.sno, 0xFFFF);
                }
            }
        }
        else
        {
            ESFTL_LOG("esFTL: FATAL ERROR: %d %s %d\n", i, __FILE__, __LINE__);
        }
    }
}

/*
 * @brief find which page belongs to the sector
 *
 * @param sno
 * @return -1 if the sector is not assigned yet
 */
int esFtl_FindSectorPage(uint16_t sno)
{
    SpareData sData;
    int i = 0;

    if (sno < ESFTL_SECTORCACHESIZE)
    {
        if (sectorCache[sno] != 0xFFFF)
            return sectorCache[sno];
        else
            return -1;
    }

    if (sno > lastOpSectorNo)
        return -1;

    if (cursorEnd == cursorStart)
        return -1;

    for (i = cursorEnd - 1; i != cursorStart; i--)
    {
        if (i < 0)
        {
            i = ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK - 2;
        }

        if (!esFtl_NandFlashRead(i, ESFTL_NANDPAGEDATASIZE, (uint8_t *)&sData, 5))
        {
            if (sData.sno != 0xFFFF && sData.sno == sno)
            {
                if (sData.released == 0xFF)
                {
                    return i;
                }
                else
                {
                    return -1;
                }
            }
        }
        else
        {
            ESFTL_LOG("esFtl: FATAL ERROR: %d %s %d\n", i, __FILE__, __LINE__);
        }
    }

    return -1;
}

/*
 * @brief increment one the end point of the cursor
 *
 */
void esFtl_IncrementCursorEnd(void)
{
    cursorEnd++;
    if (cursorEnd >= 0xFFFF)
        cursorEnd = 0;
}

/*
 * @brief assign page to a sector in the cache
 *
 * @param sno
 * @param pno
 */
void esFtl_SetSectorCache(uint16_t sno, uint16_t pno)
{
    if (sno < ESFTL_SECTORCACHESIZE)
        sectorCache[sno] = pno;
}
