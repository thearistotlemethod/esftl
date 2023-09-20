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

#ifndef ESFTL_CACHE_H__
#define ESFTL_CACHE_H__

uint8_t esFtl_IsDefragNeeded(void);
void esFtl_EvaluateCursorAndCache(void);
int esFtl_FindSectorPage(uint16_t sno);
void esFtl_IncrementCursorEnd(void);
void esFtl_SetSectorCache(uint16_t sno, uint16_t pno);

extern int cursorEnd;
extern int cursorStart;
extern uint16_t lastOpSectorNo;
extern uint8_t defragmentNeeded;

#endif