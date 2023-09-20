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
#include "esFtl_read.h"

/*
 * @brief read the page data of the sector
 *
 * @param sno
 * @param buffer
 * @param idx
 * @param count
 * @return -1 if it is not seccessful
 */
int esFtl_Read(uint16_t sno, uint8_t *buffer, uint32_t idx, uint32_t count)
{
    int pno = 0, rv = -1;
    sno++;

    memset(&buffer[idx], 0xFF, count);

    pno = esFtl_FindSectorPage(sno);
    if (pno >= 0)
    {
        rv = esFtl_NandFlashRead(pno, idx, buffer, count);
        if (rv)
        {
            ESFTL_LOG("esFTL: FATAL ERROR:%d %s %d\n", pno, __FILE__, __LINE__);
        }
    }

    return rv;
}