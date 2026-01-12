/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for CM55 CPU
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
#define RESET_VAL                   (0U)

/*******************************************************************************
* Global Variable(s)
*******************************************************************************/
static volatile bool cm55_msg_received = false;

CY_SECTION_SHAREDMEM static ipc_msg_t cm55_msg_data;

/*******************************************************************************
* Function Name: cm55_msg_callback
********************************************************************************
* Summary:
*  Callback function called when endpoint-2 (CM55) has received a message
*
* Parameters:
*  msg_data: Message data received through IPC
*
* Return :
*  void
*
*******************************************************************************/
void cm55_msg_callback(uint32_t * msgData)
{
    if (msgData != NULL)
    {
        /* Message received from CM33 */
        cm55_msg_received = true;
    }
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CM55 application. 
* 
* This simple application waits for a message from CM33 and prints a 
* confirmation message when received.
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

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    /* NOTE: retarget-io (UART printf) is NOT initialized on CM55 to avoid 
     * conflicts with CM33's MicroPython REPL which uses the same UART.
     * If you need debug output from CM55, use a separate UART or LED indicators.
     */

    /* Setup IPC communication for CM55 */
    cm55_ipc_communication_setup();

    /* Register a callback function to handle events on the CM55 IPC pipe */
    pipeStatus = Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR, &cm55_msg_callback,
                                              (uint32_t)CM55_IPC_PIPE_CLIENT_ID);

    if(CY_IPC_PIPE_SUCCESS != pipeStatus)
    {
        /* Error registering callback - blink LED rapidly as error indicator */
        for(;;)
        {
            Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
            Cy_SysLib_Delay(100);
        }
    }

    for (;;)
    {
        /* Check if message was received from CM33 */
        if (cm55_msg_received)
        {
            /* Toggle LED to indicate CM55 is running and received message */
            Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
            
            /* Reset flag */
            cm55_msg_received = false;
        }

        /* Small delay */
        Cy_SysLib_Delay(100);
    }
}

/* [] END OF FILE */