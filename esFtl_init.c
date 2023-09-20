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
#include "esFtl_bbm.h"
#include "esFtl_cache.h"
#include "esFtl_defragment.h"
#include "esFtl_init.h"

/*
 * @brief initialize the disk first time and format it if it is requested
 *
 * @param format
 * @return 0
 */
int esFtl_Init(uint8_t format)
{
    uint8_t firstBlockMarked = 0;
    int i = 0;

    esFtl_NandFlashInit();

    if (format)
    {
        for (i = 0; i < ESFTL_NANDNUMBLOCKS; i++)
        {
            if (esFtl_NandFlashBlockErase(i) != 0)
            {
                ESFTL_LOG("Erase block fail %d!!\n", i);
            }
            else if (!firstBlockMarked)
            {
                if (!esFtl_MarkedFirstBlock(i))
                    firstBlockMarked = 1;
            }
        }
    }

    esFtl_TestForBadBlocks();
    esFtl_EvaluateCursorAndCache();
    esFtl_ControlPageCorruptions();
    return 0;
}