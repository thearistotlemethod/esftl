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
#include "esFtl_write.h"
#include "esFtl_bbm.h"
#include "esFtl_defragment.h"

/*
 * @brief mark the block as the starting point in order to find at the beginning
 *
 * @param bno
 * @return 0 if it is successful
 */
int esFtl_MarkedFirstBlock(int bno)
{
    int rv = -1;
    uint8_t markedByte = 0x55;

    bno = bno % ESFTL_NANDNUMBLOCKS;

    rv = esFtl_NandFlashWrite(bno * ESFTL_NANDNUMPAGEBLOCK, ESFTL_NANDPAGEDATASIZE + 5, &markedByte, 1);
    if (rv)
        ESFTL_LOG("MarkedFirstBlock %d Error\n", bno * ESFTL_NANDNUMPAGEBLOCK);

    return rv;
}

/*
 * @brief eliminate the useless pages by moving valid ones to new blocks
 *
 */
void esFtl_Defrag(void)
{
    uint8_t tempBuff[ESFTL_NANDPAGESIZE + 1];
    uint16_t sno = 0, pno = 0, pnoOrg = 0;
    int endBlock = 0, startBlock = 0, i = 0;

    ESFTL_LOG("Defragment Start:%d %d\n", cursorStart, cursorEnd);

    endBlock = cursorEnd / ESFTL_NANDNUMPAGEBLOCK;
    startBlock = cursorStart / ESFTL_NANDNUMPAGEBLOCK;

    if (endBlock != startBlock)
    {
        i = startBlock;
        do
        {
            if (esFtl_IsBadBlock(i))
            {
                continue;
            }

            for (int j = 0; j < ESFTL_NANDNUMPAGEBLOCK; j++)
            {
                pno = i * ESFTL_NANDNUMPAGEBLOCK + j;

                if (pno == 0xFFFF)
                    break;

                if (!esFtl_NandFlashRead(pno, ESFTL_NANDPAGEDATASIZE, &tempBuff[ESFTL_NANDPAGEDATASIZE], 2))
                {
                    memcpy(&sno, &tempBuff[ESFTL_NANDPAGEDATASIZE], 2);
                    pnoOrg = esFtl_FindSectorPage(sno);
                    if (pnoOrg == pno)
                    {
                        if (!esFtl_NandFlashRead(pno, 0, tempBuff, ESFTL_NANDPAGEDATASIZE))
                        {
                            ESFTL_LOG("Sector %d is moved to page from %d to %d\n", sno, pno, cursorEnd);
                            esFtl_FtlDriverWrite(sno - 1, tempBuff, 0, ESFTL_NANDPAGEDATASIZE);
                        }
                        else
                        {
                            ESFTL_LOG("esFtl: FATAL ERROR:%s %d\n", __FILE__, __LINE__);
                        }
                    }
                }
                else
                {
                    ESFTL_LOG("esFtl: FATAL ERROR:%s %d\n", __FILE__, __LINE__);
                }
            }

            esFtl_MarkedFirstBlock(i + 1);
            esFtl_NandFlashBlockErase(i);

            ESFTL_LOG("Block %d processed\n", i);

            i++;
            if (i >= ESFTL_NANDNUMBLOCKS)
                i = 0;
        } while (i != endBlock);
    }

    ESFTL_LOG("Defragment End\n");

    esFtl_EvaluateCursorAndCache();
}

/*
 * @brief determine the free space as count of page
 *
 * @return count of pages
 */
int esFtl_CalcFreePages(void)
{
    int freePages = 0;

    if (cursorStart > cursorEnd)
    {
        freePages = cursorStart - cursorEnd;
    }
    else
    {
        freePages = ((ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK) - cursorEnd) + cursorStart;
    }

    return freePages;
}

/*
 * @brief determine the used space as count of page
 *
 * @return count of pages
 */
int esFtl_CalcUsedPages(void)
{
    int used = 0;

    used = (ESFTL_NANDNUMBLOCKS * ESFTL_NANDNUMPAGEBLOCK) - esFtl_CalcFreePages();
    ESFTL_LOG("esFtl_CalcUsedPages:%d\n", used);

    return used;
}
