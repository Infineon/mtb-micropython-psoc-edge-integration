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
#define CM55_APP_DELAY_MS           (50U)
#define RESET_VAL                   (0U)
#define BTN_DEBOUNCE_DELAY          (200U)
#define BTN_INT_PRIORITY            (7U)

/*******************************************************************************
* Global Variable(s)
*******************************************************************************/
static bool cm55_pipe2_msg_received = false;

CY_SECTION_SHAREDMEM static ipc_msg_t cm55_msg_data;

static volatile uint32_t msg_val = RESET_VAL;

typedef enum
{
    BUTTON_START,
    BUTTON_STOP,
} en_button_status_t;

static en_button_status_t button_status = BUTTON_STOP;

/* Flag to control LED heartbeat - can be disabled by IPC commands */
static volatile bool led_heartbeat_enabled = true;


/*******************************************************************************
* Function Name: cm33_msg_callback
********************************************************************************
* Summary:
*  Callback function called when endpoint-2 (CM55) has received a message
*
* Parameters:
*  msg_data: Message data received throuig IPC
*
* Return :
*  void
*
*******************************************************************************/
void cm55_msg_callback(uint32_t * msgData)
{
    ipc_msg_t *ipc_recv_msg;

    if (msgData != NULL)
    {
        /* Cast the message received to the IPC structure */
        ipc_recv_msg = (ipc_msg_t *) msgData;

        /* Check the command type */
        uint8_t cmd = ipc_recv_msg->cmd;
        
        /* Debug: Print all received commands */
        printf("[CM55] IPC callback received command: 0x%02X\r\n", cmd);
        
        if (cmd == IPC_CMD_LED_INIT)
        {
            /* Initialize LED (already done at boot, but can re-init) */
            printf("[CM55] Received LED_INIT command\r\n");
            Cy_GPIO_Pin_Init(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, &CYBSP_USER_LED_config);
            /* Re-enable heartbeat after reinit */
            led_heartbeat_enabled = true;
        }
        else if (cmd == IPC_CMD_LED_SET_ON)
        {
            /* Set LED to constant ON state */
            printf("[CM55] Received LED_SET_ON command - LED will stay ON\r\n");
            /* Disable heartbeat so main loop doesn't override */
            led_heartbeat_enabled = false;
            /* Set LED ON */
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, CYBSP_LED_STATE_ON);
        }
        else if (cmd == IPC_CMD_LED_SET_OFF)
        {
            /* Set LED to constant OFF state */
            printf("[CM55] Received LED_SET_OFF command - LED will stay OFF\r\n");
            /* Disable heartbeat so main loop doesn't override */
            led_heartbeat_enabled = false;
            /* Set LED OFF */
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, CYBSP_LED_STATE_OFF);
        }
        else
        {
            /* Extract the command to be processed in the main loop */
            msg_val = ipc_recv_msg->value;
        }
    }

    cm55_pipe2_msg_received = true;
}



/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function for CM55 application. 
* 
* This function...
* 1. Initializes retarget-io middleware used for printing logs over UART.
* 2. Initializes IPC Pipe for CM55 CPU (Endpoint-2)
* 3. Sends IPC message commands to CM33 CPU to start/stop
*    generating random numbers.
* 4. Prints random number received from IPC pipe over UART terminal.
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
    cy_en_sysint_status_t int_status;
    cy_en_syspm_status_t syspm_status;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_app_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io middleware */
    init_retarget_io();

        /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** "
           "PSOC Edge MCU: IPC Pipes "
           "****************** \r\n\n");

    /* Setup IPC communication for CM55*/
    cm55_ipc_communication_setup();

    Cy_SysLib_Delay(CM55_APP_DELAY_MS);

    /* Register a callback function to handle events on the CM55 IPC pipe */
    pipeStatus = Cy_IPC_Pipe_RegisterCallback(CM55_IPC_PIPE_EP_ADDR, &cm55_msg_callback,
                                                      (uint32_t)CM55_IPC_PIPE_CLIENT_ID);

    if(CY_IPC_PIPE_SUCCESS != pipeStatus)
    {
        handle_app_error();
    }

    cm55_msg_data.client_id = CM33_IPC_PIPE_CLIENT_ID;
    cm55_msg_data.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP2;
    cm55_msg_data.cmd = IPC_CMD_INIT;
    cm55_msg_data.value = RESET_VAL;

    pipeStatus = Cy_IPC_Pipe_SendMessage(CM33_IPC_PIPE_EP_ADDR, 
                                         CM55_IPC_PIPE_EP_ADDR, 
                                         (void *) &cm55_msg_data, RESET_VAL);
    Cy_SysLib_Delay(CM55_APP_DELAY_MS);

    /* Boot indication: Blink LED 5 times fast to show CM55 is alive */
    printf("[CM55] CM55 Core started!\r\n");
    for(int i = 0; i < 5; i++)
    {
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 0);  // LED ON
        Cy_SysLib_Delay(100);
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 1);  // LED OFF
        Cy_SysLib_Delay(100);
    }

    for (;;)
    {
        /* Heartbeat: Slow LED blink to show CM55 is running (only if enabled) */
        if (led_heartbeat_enabled) {
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 0);  // LED ON
            Cy_SysLib_Delay(500);  // 500ms on
            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, 1);  // LED OFF
            Cy_SysLib_Delay(500);  // 500ms off
        } else {
            /* If heartbeat disabled, just delay to keep loop timing */
            Cy_SysLib_Delay(50);
        }

        Cy_SysLib_Delay(CM55_APP_DELAY_MS);
    }
}

/* [] END OF FILE */