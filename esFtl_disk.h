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

#ifndef ESFTL_DISK_H__
#define ESFTL_DISK_H__

#define ESFTL_NANDPAGESIZE 2176
#define ESFTL_NANDPAGEDATASIZE 2048
#define ESFTL_NANDPAGESPARESIZE 128
#define ESFTL_NANDNUMBLOCKS 1024
#define ESFTL_NANDNUMPAGEBLOCK 64

int esFtl_NandFlashInit(void);
int esFtl_NandFlashRead(uint32_t page, uint32_t offset, uint8_t *buff, uint32_t count);
int esFtl_NandFlashWrite(uint32_t page, uint32_t offset, const uint8_t *buff, uint32_t count);
int esFtl_NandFlashBlockErase(uint32_t block);

#endif