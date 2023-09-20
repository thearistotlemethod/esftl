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

#ifndef ESFTL_DEFINITIONS_H__
#define ESFTL_DEFINITIONS_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define ESFTL_LOG(f_, ...) //printf((f_), ##__VA_ARGS__)
#define ESFTL_SECTORCACHESIZE 2048
#define ESFTL_FREEBLOCKLIMITFORDEFRAGMENT 128

#endif