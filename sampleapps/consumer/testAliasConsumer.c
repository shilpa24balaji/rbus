/*
 * If not stated otherwise in this file or this component's Licenses.txt file
 * the following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <rbus.h>


int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);
    int additionalWaitTime = 0;

    if(argc == 2)
    {
        additionalWaitTime = atoi(argv[1]);
    }

    int rc = RBUS_ERROR_SUCCESS;
    rbusHandle_t handle;
    uint32_t inst1, inst2;
    char* value;
    char name[RBUS_MAX_NAME_LENGTH];

    printf("consumer: start\n");

    rc = rbus_open(&handle, "Consumer");
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("consumer: rbus_open failed: %d\n", rc);
        goto exit1;
    }

    /*add rows to the table with aliases */
    rbusTable_addRow(handle, "Device.IP.Interface.", "wan", &inst1);
    printf("Add table row Device.IP.Interface. with alias wan with instanceNumber %d\n", inst1);

    rbusTable_addRow(handle, "Device.IP.Interface.", "lan", &inst2);
    printf("Add table row Device.IP.Interface. with alias lan with instanceNumber %d\n", inst2);
    
    /*set property 'Name' on the table rows*/
    printf("Set table row Device.IP.Interface.[wan].Name with value eth0\n");
    rbus_setStr(handle, "Device.IP.Interface.[wan].Name", "eth0");
    printf("Set table row Device.IP.Interface.[lan].Name with value brlan0\n");
    rbus_setStr(handle, "Device.IP.Interface.[lan].Name", "brlan0");
    
     /*get data by alias 'wan'*/
    if((rc = rbus_getStr(handle, "Device.IP.Interface.[wan].Name", &value)) == RBUS_ERROR_SUCCESS)
    {
        printf("Get value of table row Device.IP.Interface.[wan].Name = %s\n", value);
        free(value);
    }
    else
    {
        printf("rbus_getStr error:%d\n", rc);
    }
    
    /*get data by alias 'lan' */
    if((rc = rbus_getStr(handle, "Device.IP.Interface.[lan].Name", &value)) == RBUS_ERROR_SUCCESS)
    {
        printf("Get value of table row Device.IP.Interface.[lan].Name = %s\n", value);
        free(value);
    }
    else
    {
        printf("rbus_getStr error:%d\n", rc);
    }

    if(additionalWaitTime)
    {
        printf("waiting for an additional %d seconds\n", additionalWaitTime);
        sleep(additionalWaitTime);
    }

    printf("####### remove Device.IP.Interface.[wan]. ###########\n");
    rbusTable_removeRow(handle, "Device.IP.Interface.[wan].");
    printf("########## remove Device.IP.Interface.[lan]. ###########\n");
    snprintf(name, RBUS_MAX_NAME_LENGTH, "Device.IP.Interface.[lan].");
    rbusTable_removeRow(handle,name);

    rbus_close(handle);

exit1:
    printf("consumer: exit\n");
    return rc;
}
