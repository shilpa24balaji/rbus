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
#include <string.h>
#include <getopt.h>
#include <rbus.h>
#include "utilities.h"


int runtime = 20;
int g_count = 0;

#define MAX_TABLE_ROWS 16
#define MAX_LENGTH 64

typedef struct T1
{
    bool inUse;
    uint32_t instNum;
    char alias[MAX_LENGTH];
    char name[MAX_LENGTH];
} T1;

typedef struct DataModel
{
    T1 t1[MAX_TABLE_ROWS];
    uint32_t t1InstNum;
} DataModel;

DataModel gDM;

void printDataModel()
{
    int i;
    printf("begin datamodel\n");
    for(i = 0; i < MAX_TABLE_ROWS; ++i)
    {
        T1* t1 = &gDM.t1[i];
        if(t1->inUse)
        {
            printf("%d: instNum=%d, alias=%s, name=%s t1=\n", i, t1->instNum, t1->alias, t1->name);
        }
    }
    printf("end datamodel\n");
}

T1* findT1(char const* rowName)
{
    TableRowID rowID;
    int i;

    getTableRowID(rowName, 3, &rowID);

    for(i = 0; i < MAX_TABLE_ROWS; ++i)
    {
        T1* t1 = &gDM.t1[i];
        printf("comparing %d,%s,%d,%s,%d\n", t1->instNum, t1->alias, rowID.instNum, rowID.alias, rowID.type);
        if(t1->inUse && compareTableRowID(&rowID, t1->instNum, t1->alias))
            return t1;
    }

    return NULL;
}


rbusError_t tableAddRowHandler1(rbusHandle_t handle, char const* tableName, char const* aliasName, uint32_t* instNum)
{
    int i;
    (void)handle;

    printf(
        "tableAddRowHandler1 called:\n" \
        "\ttableName=%s\n" \
        "\taliasName=%s\n",
        tableName, aliasName);

    if(g_count >= MAX_TABLE_ROWS)
    {
        return RBUS_ERROR_OUT_OF_RESOURCES;
    }
    for(i = 0; i <= g_count; ++i)
    {
        T1* t1 = &gDM.t1[i];

        if(!t1->inUse)
        {
            memset(t1, 0, sizeof(T1));

            t1->inUse = true;
            t1->instNum = ++gDM.t1InstNum;
            ++g_count;
            if(aliasName)
                strncpy(t1->alias, aliasName, MAX_LENGTH);

            *instNum = t1->instNum;
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t tableRemoveRowHandler1(rbusHandle_t handle, char const* rowName)
{
    T1* t1;
    (void)handle;

    printf(
        "tableRemoveRowHandler1 called:\n" \
        "\trowName=%s\n", rowName);

    t1 = findT1(rowName);

    if(t1)
    {
        memset(t1, 0, sizeof(T1));
        --g_count;
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }

    return RBUS_ERROR_BUS_ERROR;
}

rbusError_t getHandler1(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    rbusValue_t value;
    T1* t1;
    char const* name = rbusProperty_GetName(property);

    (void)handle;
    (void)opts;

    printf(
        "getHandler1 called:\n" \
        "\tproperty=%s\n",
        name);

    t1 = findT1(name);

    if(!t1)
        return RBUS_ERROR_BUS_ERROR;

    rbusValue_Init(&value);

    if(propertyNameEquals(name, "Alias"))
    {
        rbusValue_SetString(value, t1->alias);
    }
    else if(propertyNameEquals(name, "Name"))
    {
        rbusValue_SetString(value, t1->name);
    }
    else
    {
        rbusValue_Release(value);
        return RBUS_ERROR_BUS_ERROR;
    }

    rbusProperty_SetValue(property, value);
    rbusValue_Release(value);
    return RBUS_ERROR_SUCCESS;
}

rbusError_t setHandler1(rbusHandle_t handle, rbusProperty_t property, rbusSetHandlerOptions_t* opts)
{
    T1* t1;
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value = rbusProperty_GetValue(property);

    (void)handle;
    (void)opts;

    printf(
        "setHandler1 called:\n" \
        "\tproperty=%s\n",
        name);

    t1 = findT1(name);

    if(!t1)
        return RBUS_ERROR_BUS_ERROR;

    if(propertyNameEquals(name, "Alias"))
    {
        strncpy(t1->alias, rbusValue_GetString(value, NULL), MAX_LENGTH-1);
        printDataModel();
        return RBUS_ERROR_SUCCESS;
    }
    else if(propertyNameEquals(name, "Name"))
    {
        if(rbusValue_GetType(value) != RBUS_STRING)
        {
            printf("Invalid Input. Give string input\n");
            return RBUS_ERROR_INVALID_INPUT;
        }
        else
        {
            strncpy(t1->name, rbusValue_GetString(value, NULL), MAX_LENGTH-1);
            printDataModel();
            return RBUS_ERROR_SUCCESS;
        }
    }
    else
    {
        return RBUS_ERROR_BUS_ERROR;
    }
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    rbusHandle_t handle;
    int rc = RBUS_ERROR_SUCCESS;

    if(argc == 2)
    {
        runtime = atoi(argv[1]);
    }

    char componentName[] = "TableProvider1";

    static rbusDataElement_t dataElements[3] = {
        {"Device.IP.Interface.{i}.", RBUS_ELEMENT_TYPE_TABLE, {NULL, NULL, tableAddRowHandler1, tableRemoveRowHandler1, NULL, NULL}},
        {"Device.IP.Interface.{i}.Alias", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler1, setHandler1, NULL, NULL, NULL, NULL}},
        {"Device.IP.Interface.{i}.Name", RBUS_ELEMENT_TYPE_PROPERTY, {getHandler1, setHandler1, NULL, NULL, NULL, NULL}}
    };

    printf("provider: start\n");

    memset(&gDM, 0, sizeof(DataModel));

    rc = rbus_open(&handle, componentName);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbus_open failed: %d\n", rc);
        goto exit2;
    }

    rc = rbus_regDataElements(handle, 3, dataElements);
    if(rc != RBUS_ERROR_SUCCESS)
    {
        printf("provider: rbusEventProvider_Register failed: %d\n", rc);
        goto exit1;
    }

    while (runtime != 0)
    {
        printf("provider: exiting in %d seconds\n", runtime);
        sleep(1);
        runtime--;
    }

    rbus_unregDataElements(handle, 3, dataElements);
exit1:
    rbus_close(handle);
exit2:
    printf("provider: exit\n");
    return rc;
}
