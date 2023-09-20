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

#ifndef ESFTL_BBM_H__
#define ESFTL_BBM_H__

void esFtl_TestForBadBlocks(void);
int esFtl_IsBadBlock(uint16_t block);
void esFtl_ControlPageCorruptions(void);
int esFtl_CheckIfPageInBadBlock(int pno);
int esFtl_CheckCorruption(uint16_t sno, uint8_t *buff);

#endif