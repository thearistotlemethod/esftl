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

#include "esFtl.h"

const char *testData = "Test Data";

int test(void)
{
    int rv = -1;
    char buffer[ESFTL_NANDPAGESIZE];

    esFtl_Init(1);
    esFtl_FtlDriverWrite(0, testData, 0, strlen(testData));

    esFtl_FtlDriverRead(0, buffer, 0, ESFTL_NANDPAGEDATASIZE);

    if (memcmp(testData, buffer, strlen(testData)))
        printf("Test Failed!!!\n");
    else
    {
        printf("Test Passed\n");
        rv = 0;
    }

    return rv;
}