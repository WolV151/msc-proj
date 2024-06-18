/************************************************************************
 * NASA Docket No. GSC-18,719-1, and identified as “core Flight System: Bootes”
 *
 * Copyright (c) 2020 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

/**
 * \file
 *   This file contains the source code for the Command Ingest task.
 */

/*
**   Include Files:
*/

#include <string.h>
#include <time.h>
#include "ci_lab_app.h"
#include "ci_lab_perfids.h"
#include "ci_lab_msgids.h"
#include "ci_lab_version.h"
#include "ci_lab_decode.h"
#include "RFM69.h"

/*
** CI Global Data
*/
#define FREQUENCY   RF69_433MHZ
#define SAT_NODE    2
#define GS_NODE     1
#define NETWORKID   100
#define INT_PIN     18
#define RST_PIN     29
#define SPI_BUS     0

CI_LAB_GlobalData_t CI_LAB_Global;
time_t              start_time, current_time;
uint8               seconds = 0;
char                buf[5000];
uint32              offset = 0;

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                            */
/* Application entry point and main process loop                              */
/* Purpose: This is the Main task event loop for the Command Ingest Task      */
/*            The task handles all interfaces to the data system through      */
/*            the software bus. There is one pipeline into this task          */
/*            The task is scheduled by input into this pipeline.              */
/*            It can receive Commands over this pipeline                      */
/*            and acts accordingly to process them.                           */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void CI_LAB_AppMain(void)
{
    CFE_Status_t     status;
    uint32           RunStatus = CFE_ES_RunStatus_APP_RUN;
    CFE_SB_Buffer_t *SBBufPtr;


    CFE_ES_PerfLogEntry(CI_LAB_MAIN_TASK_PERF_ID);

    CI_LAB_TaskInit();

    /*
    ** CI Runloop
    */
    time(&start_time);
    memset(buf, 0, 1000);
    while (CFE_ES_RunLoop(&RunStatus) == true)
    {
        CFE_ES_PerfLogExit(CI_LAB_MAIN_TASK_PERF_ID);

        /* Receive SB buffer, configurable timeout */
        time(&current_time);
        status = CFE_SB_ReceiveBuffer(&SBBufPtr, CI_LAB_Global.CommandPipe, CI_LAB_SB_RECEIVE_TIMEOUT);

        CFE_ES_PerfLogEntry(CI_LAB_MAIN_TASK_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            CI_LAB_TaskPipe(SBBufPtr);
        }

        /* Regardless of packet vs timeout, always process uplink queue if not scheduled */
        if (CI_LAB_Global.RadioInit && !CI_LAB_Global.Scheduled)
        {
            CI_LAB_ReadUpLink();
        }
    }

    CFE_ES_ExitApp(RunStatus);
}

/*
** CI delete callback function.
** This function will be called in the event that the CI app is killed.
** It will close the network socket for CI
*/
void CI_LAB_delete_callback(void)
{
    OS_printf("Switching off..\n");
}

void my_super_secret_func(void)
{
    OS_printf("This cannot run! Program will most likely crash now.\n");
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* CI initialization                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void CI_LAB_TaskInit(void)
{
    int32  status;
    char VersionString[CI_LAB_CFG_MAX_VERSION_STR_LEN];

    unsigned char key[16] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
    };

    memset(&CI_LAB_Global, 0, sizeof(CI_LAB_Global));

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("CI_LAB: Error registering for Event Services, RC = 0x%08X\n", (unsigned int)status);
    }

    status = CFE_SB_CreatePipe(&CI_LAB_Global.CommandPipe, CI_LAB_PIPE_DEPTH, "CI_LAB_CMD_PIPE");
    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_CMD_MID), CI_LAB_Global.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(CI_LAB_SB_SUBSCRIBE_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error subscribing to SB Commands, RC = 0x%08X", (unsigned int)status);
        }

        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_SEND_HK_MID), CI_LAB_Global.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(CI_LAB_SB_SUBSCRIBE_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error subscribing to SB HK Request, RC = 0x%08X", (unsigned int)status);
        }

        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(CI_LAB_READ_UPLINK_MID), CI_LAB_Global.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(CI_LAB_SB_SUBSCRIBE_UL_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Error subscribing to SB Read Uplink Request, RC = 0x%08X", (unsigned int)status);
        }
    }
    else
    {
        CFE_EVS_SendEvent(CI_LAB_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Error creating SB Command Pipe, RC = 0x%08X", (unsigned int)status);
    }

    // Radio Config
    status = initialize(FREQUENCY, SAT_NODE, NETWORKID, INT_PIN, RST_PIN, SPI_BUS);
    
    if (status != CFE_SUCCESS)
    {
        CFE_EVS_SendEvent(CI_LAB_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Error initializing radio, RC = 0x%08X", (unsigned int)status);

    }

    OS_printf("Frequency set to %d:\n", getFrequency()); 
    OS_printf("Radio Temperature: %d\n", readTemperature(0));
    
    setHighPower(1);
    set_encrypt_key(key);
    
    CFE_ES_WriteToSysLog("CI_LAB Radio working at %uMHZ, on NetworkID: %u, RadioID: %u\n", getFrequency(), NETWORKID, SAT_NODE);
    CI_LAB_Global.RadioInit = true;


    CI_LAB_ResetCounters_Internal();

    /*
    ** Install the delete handler
    */
    OS_TaskInstallDeleteHandler(&CI_LAB_delete_callback);

    CFE_MSG_Init(CFE_MSG_PTR(CI_LAB_Global.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(CI_LAB_HK_TLM_MID),
                 sizeof(CI_LAB_Global.HkTlm));

    CFE_Config_GetVersionString(VersionString, CI_LAB_CFG_MAX_VERSION_STR_LEN, "CI Lab App",
        CI_LAB_VERSION, CI_LAB_BUILD_CODENAME, CI_LAB_LAST_OFFICIAL);

    CFE_EVS_SendEvent(CI_LAB_INIT_INF_EID, CFE_EVS_EventType_INFORMATION, "CI Lab Initialized.%s",
                      VersionString);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void CI_LAB_ResetCounters_Internal(void)
{

    OS_printf("\nWe are resetting the counters....\n");

    /* Status of commands processed by CI task */
    CI_LAB_Global.HkTlm.Payload.CommandCounter      = 0;
    CI_LAB_Global.HkTlm.Payload.CommandErrorCounter = 0;

    /* Status of packets ingested by CI task */
    CI_LAB_Global.HkTlm.Payload.IngestPackets = 0;
    CI_LAB_Global.HkTlm.Payload.IngestErrors  = 0;
}

size_t custom_strlen(const char *buffer, size_t buffer_size) {
    size_t len = 0;
    int i = buffer_size; // Start from the last byte
    // Iterate in reverse until a non-zero character is encountered or we reach the beginning of the buffer
    int flag = 0;
    
    while(1)
    {
        if (flag == 1)
            len++;

        if (buf[i] != 0)
            flag = 1;

        if (i == 0)
            break;
        
        i--;
    }
    return len;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* --                                                                         */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void CI_LAB_ReadUpLink(void)
{
    int   i;
//    int32 OsStatus;

    CFE_Status_t     CfeStatus;
    CFE_SB_Buffer_t *SBBufPtr;
    char             vuln[10];

    for (i = 0; i <= 30; i++)
    {
        if (CI_LAB_Global.RadioBufPtr == NULL)
        {
            CI_LAB_GetInputBuffer(&CI_LAB_Global.RadioBufPtr, &CI_LAB_Global.RadioBufSize);
        }

        if (CI_LAB_Global.RadioBufPtr == NULL)
        {
            break;
        }

        if (receiveDone())
        {
            memcpy(buf + offset, DATA, DATALEN);
            memcpy((uint8*)CI_LAB_Global.RadioBufPtr, DATA, CI_LAB_Global.RadioBufSize);
            offset += DATALEN;
            time(&start_time);
            for (unsigned int x = 0; x < DATALEN; x++)
            {
                OS_printf("%x", DATA[x]);
            }
            OS_printf("Received buffer with RSSI: %d by Sender: %d\n", RSSI, SENDERID);
        }
//        OsStatus = OS_SocketRecvFrom(CI_LAB_Global.SocketID, CI_LAB_Global.NetBufPtr, CI_LAB_Global.NetBufSize,
                          //           &CI_LAB_Global.SocketAddress, CI_LAB_UPLINK_RECEIVE_TIMEOUT);
        seconds = (unsigned int)difftime(current_time, start_time);
        OS_printf("Seconds %d: \n", seconds);

        if ((seconds > 4) && (offset > 0))
        {
            for (unsigned int x = 0; x < 5000; x++)
            {
                OS_printf("%x", buf[x]);
            }

            size_t len_to_copy = custom_strlen(buf, 4999);

            OS_printf("Length to copy is: \n%d\n", len_to_copy);

            memcpy(vuln, buf, len_to_copy);
            memset(buf, 0, 5000);
            offset = 0;
            CFE_ES_PerfLogEntry(CI_LAB_SOCKET_RCV_PERF_ID);
            CfeStatus = CI_LAB_DecodeInputMessage(CI_LAB_Global.RadioBufPtr, CI_LAB_Global.RadioBufSize, &SBBufPtr);

            //for (unsigned int i=1; i < DATALEN; i++)
           // {
            //    OS_printf("%x", DATA[i]);
            //}
            
            if (CfeStatus != CFE_SUCCESS)
            {
                CI_LAB_Global.HkTlm.Payload.IngestErrors++;
                OS_printf("Failed to decode the input message!!\n");
            }
            else
            {
                CI_LAB_Global.HkTlm.Payload.IngestPackets++;
                CfeStatus = CFE_SB_TransmitBuffer(SBBufPtr, false);
            }
            CFE_ES_PerfLogExit(CI_LAB_SOCKET_RCV_PERF_ID);

            if (CfeStatus == CFE_SUCCESS)
            {
                /* Set NULL so a new buffer will be obtained next time around */
                CI_LAB_Global.RadioBufPtr  = NULL;
                CI_LAB_Global.RadioBufSize = 0;
            }
            else
            {
                CFE_EVS_SendEvent(CI_LAB_INGEST_SEND_ERR_EID, CFE_EVS_EventType_ERROR,
                                  "CI_LAB: Ingest failed, status=%d\n", (int)CfeStatus);
            }
        }
        else
        {
            break; /* no (more) messages */
        }
    }
}
