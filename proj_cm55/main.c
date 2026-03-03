/*******************************************************************************
* File Name        : main.c
*
* Description      : CM55 ping-pong IPC application.
*                    Receives any command from CM33 and echoes it back unchanged.
*
* Related Document : See README.md
*
********************************************************************************
* (c) 2025, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "ipc_communication.h"
#include "retarget_io_init.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define CM55_APP_DELAY_MS   (50U)

/*******************************************************************************
* Typedefs
*******************************************************************************/
typedef struct
{
    uint8_t     client_id;
    uint16_t    intr_mask;
    uint8_t     cmd;
    uint32_t    value;
} ipc_msg_t;

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Shared-memory buffer for the outgoing echo message */
CY_SECTION_SHAREDMEM static ipc_msg_t cm55_msg_data;
CY_SECTION_SHAREDMEM static ipc_msg_t cm55_msg_data2;

/* Pending echo state — written in ISR callback, consumed in main loop */
/* Service 1 (client_id=5) */
static volatile bool     echo_pending = false;
static volatile uint8_t  echo_cmd     = 0U;
static volatile uint32_t echo_value   = 0UL;

/* Service 2 (client_id=6) */
static volatile bool     echo2_pending = false;
static volatile uint8_t  echo2_cmd     = 0U;
static volatile uint32_t echo2_value   = 0UL;


/*******************************************************************************
* Function Name: cm55_msg_callback
********************************************************************************
* Summary:
*  IPC callback executed on CM55 when a message arrives from CM33.
*  Stores the received command/value so the main loop can echo them back.
*
* Parameters:
*  msgData - Pointer to the received IPC message (cast to ipc_msg_t *)
*
* Return:
*  void
*
*******************************************************************************/
void cm55_msg_callback(uint32_t *msgData)
{
    if (msgData == NULL)
    {
        return;
    }

    ipc_msg_t *rx = (ipc_msg_t *)msgData;

    /* Buffer for echo in main loop (avoids IPC send from ISR context) */
    echo_cmd     = rx->cmd;
    echo_value   = rx->value;
    echo_pending = true;
}


/*******************************************************************************
* Function Name: cm55_svc2_callback
********************************************************************************
* Summary:
*  IPC callback for Service 2 (registered at client_id=6).
*  Stores the received command/value so the main loop can echo them back
*  to CM33 Service 2 (client_id=4).
*
* Parameters:
*  msgData - Pointer to the received IPC message (cast to ipc_msg_t *)
*
* Return:
*  void
*
*******************************************************************************/
void cm55_svc2_callback(uint32_t *msgData)
{
    if (msgData == NULL)
    {
        return;
    }

    ipc_msg_t *rx = (ipc_msg_t *)msgData;

    echo2_cmd     = rx->cmd;
    echo2_value   = rx->value;
    echo2_pending = true;
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  CM55 entry point – ping-pong IPC application.
*  1. Initialises peripherals.
*  2. Sets up IPC Pipe (EP2) and registers cm55_msg_callback.
*  3. Main loop: whenever a command arrives from CM33, echoes it straight back.
*
* Parameters:
*  void
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_ipc_pipe_status_t pipeStatus;

    /* Initialise BSP */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialise UART for debug output */
    init_retarget_io();

    /* Set up IPC pipe for CM55 (Endpoint-2) */
    cm55_ipc_communication_setup();

    Cy_SysLib_Delay(CM55_APP_DELAY_MS);

    /* Register Service 1 callback at client_id=5 */
    pipeStatus = Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                              &cm55_msg_callback,
                                              (uint32_t)CM55_IPC_PIPE_CLIENT_ID);

    if (CY_IPC_PIPE_SUCCESS != pipeStatus)
    {
        printf("[CM55] Error: Service1 callback registration failed (status=%d)\r\n", pipeStatus);
        handle_app_error();
    }

    /* Register Service 2 callback at client_id=6 */
    pipeStatus = Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR,
                                              &cm55_svc2_callback,
                                              (uint32_t)(CM55_IPC_PIPE_CLIENT_ID + 1U));

    if (CY_IPC_PIPE_SUCCESS != pipeStatus)
    {
        printf("[CM55] Error: Service2 callback registration failed (status=%d)\r\n", pipeStatus);
        handle_app_error();
    }

    for (;;)
    {
        if (echo_pending)
        {
            echo_pending = false;

            /* Echo Service 1: directed to CM33 Service1 (client_id=3) */
            cm55_msg_data.client_id = CM33_IPC_PIPE_CLIENT_ID;
            cm55_msg_data.intr_mask = CY_IPC_CYPIPE_INTR_MASK;
            cm55_msg_data.cmd       = echo_cmd;
            cm55_msg_data.value     = (uint32_t)echo_value;

            pipeStatus = Cy_IPC_Pipe_SendMessage(
                CM33_IPC_PIPE_EP_ADDR,
                CM55_IPC_PIPE_EP_ADDR,
                (void *)&cm55_msg_data,
                NULL);

            if (CY_IPC_PIPE_SUCCESS != pipeStatus)
            {
                printf("[CM55] Service1 echo failed (status=%d)\r\n", pipeStatus);
            }
        }

        if (echo2_pending)
        {
            echo2_pending = false;

            /* Echo Service 2: directed to CM33 Service2 (client_id=4) */
            cm55_msg_data2.client_id = CM33_IPC_PIPE_CLIENT_ID + 1U;
            cm55_msg_data2.intr_mask = CY_IPC_CYPIPE_INTR_MASK;
            cm55_msg_data2.cmd       = echo2_cmd;
            cm55_msg_data2.value     = (uint32_t)echo2_value;

            pipeStatus = Cy_IPC_Pipe_SendMessage(
                CM33_IPC_PIPE_EP_ADDR,
                CM55_IPC_PIPE_EP_ADDR,
                (void *)&cm55_msg_data2,
                NULL);

            if (CY_IPC_PIPE_SUCCESS != pipeStatus)
            {
                printf("[CM55] Service2 echo failed (status=%d)\r\n", pipeStatus);
            }
        }

        Cy_SysLib_Delay(CM55_APP_DELAY_MS);
    }
}

/* [] END OF FILE */