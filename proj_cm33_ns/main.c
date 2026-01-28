/*******************************************************************************
* File Name        : main.c
*
* Description      : This source file contains the main routine for non-secure
*                    application in the CM33 CPU
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
#include "cybsp.h"
#include "ipc_communication.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define MCWDT_HW                    (CYBSP_CM33_LPTIMER_0_HW)
#define MCWDT_CTR0_MATCH_VAL        (32768U)
#define MCWDT_CTR1_MATCH_VAL        (32768U)
#define MCWDT_TOGGLE_BIT            (16U)
#define MCWDT_INT_PRIORITY          (3U)

/* The timeout value in microseconds used to wait for CM55 core to be booted */
#define CM55_BOOT_WAIT_TIME_USEC    (10U)

/* App boot address for CM55 project */
#define CM55_APP_BOOT_ADDR          (CYMEM_CM33_0_m55_nvm_START + \
                                        CYBSP_MCUBOOT_HEADER_SIZE)

/* TRNG constants */
#define GARO31_INITSTATE            (0x04c11db7)
#define FIRO31_INITSTATE            (0x04c11db7)
#define MAX_TRNG_BIT_SIZE           (32UL)

#define GPIO_HIGH                   (1U)
#define GPIO_LOW                    (0U)
#define CM33_APP_DELAY_MS           (50U)
#define RESET_VAL                   (0U)

/*******************************************************************************
* Global Variable(s)
*******************************************************************************/
static uint32_t random_number;

CY_SECTION_SHAREDMEM static ipc_msg_t cm33_msg_data;

static volatile uint8_t msg_cmd = RESET_VAL;

static const cy_stc_sysint_t mcwdt_int_cfg = 
{
    .intrSrc          = srss_interrupt_mcwdt_0_IRQn,
    .intrPriority     = MCWDT_INT_PRIORITY
};

/* Watchdog timer configuration */
static const cy_stc_mcwdt_config_t mcwdt_cfg =
{
    .c0Match          = MCWDT_CTR0_MATCH_VAL,
    .c1Match          = MCWDT_CTR1_MATCH_VAL,
    .c0Mode           = CY_MCWDT_MODE_INT,
    .c1Mode           = CY_MCWDT_MODE_NONE ,
    .c2ToggleBit      = MCWDT_TOGGLE_BIT,
    .c2Mode           = CY_MCWDT_MODE_NONE ,
    .c0ClearOnMatch   = true,
    .c1ClearOnMatch   = false,
    .c0c1Cascade      = false,
    .c1c2Cascade      = false,
    .c0LowerLimitMode = CY_MCWDT_LOWER_LIMIT_MODE_NOTHING,
    .c0LowerLimit     = RESET_VAL,
    .c1LowerLimitMode = CY_MCWDT_LOWER_LIMIT_MODE_NOTHING,
    .c1LowerLimit     = RESET_VAL
};


/*******************************************************************************
* Function Name: handle_error
********************************************************************************
* Summary:
*  Function to handle error status
*
* Parameters:
*  none
*
* Return :
*  void
*
*******************************************************************************/
static void handle_error(void)
{
    /* Disable all interrupts. */
    __disable_irq();

    CY_ASSERT(0);

    while(true);
}


/*******************************************************************************
* Function Name: mcwdt_handler
********************************************************************************
* Summary:
*  Watchdog handler to periodically wake up the CM33.
*
* Parameters:
*  none
*
* Return :
*  void
*
*******************************************************************************/
static void mcwdt_handler(void)
{
    uint32 mcwdtIsrMask;

    /* Get the Watchdog Interrupt Status */
    mcwdtIsrMask = Cy_MCWDT_GetInterruptStatus(MCWDT_HW);

    if(CY_MCWDT_CTR0 & mcwdtIsrMask)
    {
        /* Clear Watchdog Interrupt */
        Cy_MCWDT_ClearInterrupt(MCWDT_HW, CY_MCWDT_CTR0);

        /* Set the message command to be processed in the main loop */
        msg_cmd = IPC_CMD_STATUS;
    }
}


/*******************************************************************************
* Function Name: cm33_msg_callback
********************************************************************************
* Summary:
*  Callback function called when endpoint-1 (CM33) has received a message
*
* Parameters:
*  msg_data: Message data received throuig IPC
*
* Return :
*  void
*
*******************************************************************************/
void cm33_msg_callback(uint32_t * msg_data)
{
    ipc_msg_t *ipc_recv_msg;

    if (msg_data != NULL)
    {
        /* Cast the message received to the IPC structure */
        ipc_recv_msg = (ipc_msg_t *) msg_data;

        /* Extract the command to be processed in the main loop */
        msg_cmd = ipc_recv_msg->cmd;
    }
}


/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
* This is the main function of the CM33 non-secure application. 
* This function...
* 1. Initializes the device and board peripherals. 
* 2. Enables  the CM55 CPU.
* 3. Initializes IPC Pipe for CM33 CPU (Endpoint-1)
* 4. Received IPC message commands from CM55 and
*    returns a random number over the IPC pipe to CM55.
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_en_ipc_pipe_status_t pipeStatus;
    cy_en_sysint_status_t sysint_status;
    cy_en_mcwdt_status_t mcwdt_status;
    cy_en_crypto_status_t crypt_status;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        handle_error();
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Setup IPC communication for CM33 */
    cm33_ipc_communication_setup();

    Cy_SysLib_Delay(CM33_APP_DELAY_MS);

    /* Register a callback function to handle events on the CM33 IPC pipe */
    pipeStatus = Cy_IPC_Pipe_RegisterCallback(CM33_IPC_PIPE_EP_ADDR, &cm33_msg_callback,
                                              (uint32_t)CM33_IPC_PIPE_CLIENT_ID);

    if(CY_IPC_PIPE_SUCCESS != pipeStatus)
    {
        handle_error();
    }

    /* Enable CM55. */
    /* CM55_APP_BOOT_ADDR must be updated if CM55 memory layout is changed.*/
    Cy_SysEnableCM55(MXCM55, CM55_APP_BOOT_ADDR, CM55_BOOT_WAIT_TIME_USEC);
    
    /* Delay to allow CM55 app to execute */
    Cy_SysLib_Delay(CM33_APP_DELAY_MS);

    for(;;)
    {
        // ToDo: None of the below code might be really needed. This will be cleaned up step - by- step
        switch(msg_cmd)
        {
            case IPC_CMD_INIT:

            /* Initialize the MCWDT Interrupt */
            sysint_status = Cy_SysInt_Init(&mcwdt_int_cfg, mcwdt_handler);
            if (CY_SYSINT_SUCCESS != sysint_status)
            {
                handle_error();
            }
            NVIC_ClearPendingIRQ((IRQn_Type) mcwdt_int_cfg.intrSrc);
            NVIC_EnableIRQ((IRQn_Type) mcwdt_int_cfg.intrSrc);

            /* Init the MCWDT */
            mcwdt_status = Cy_MCWDT_Init(MCWDT_HW, &mcwdt_cfg);
            if (CY_MCWDT_SUCCESS != mcwdt_status)
            {
                handle_error();
            }
            Cy_MCWDT_SetInterruptMask(MCWDT_HW, CY_MCWDT_CTR0);

            break;

            case IPC_CMD_START:

            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, GPIO_HIGH);

            NVIC_ClearPendingIRQ((IRQn_Type) mcwdt_int_cfg.intrSrc);
            NVIC_EnableIRQ((IRQn_Type) mcwdt_int_cfg.intrSrc);

            /* MCWDT Enable */
            Cy_MCWDT_Enable(MCWDT_HW, CY_MCWDT_CTR0, 0u);

            /* Crypto block Enable */
            crypt_status = Cy_Crypto_Core_Enable(CRYPTO);
            if (CY_CRYPTO_SUCCESS != crypt_status)
            {
                handle_error();
            }

            break;

            case IPC_CMD_STOP:

            Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN, GPIO_LOW);

            /* MCWDT Disable */
            Cy_MCWDT_Disable(MCWDT_HW, CY_MCWDT_CTR0, 0u);

            /* Crypto block Disable */
            crypt_status = Cy_Crypto_Core_Disable(CRYPTO);
            if (CY_CRYPTO_SUCCESS != crypt_status)
            {
                handle_error();
            }

            break;

            case IPC_CMD_STATUS:

            /* Generate a random number --> #ToDo: Not needed when application is changed? */
            crypt_status = Cy_Crypto_Core_Trng(CRYPTO, GARO31_INITSTATE, \
                                                        FIRO31_INITSTATE, \
                                                        MAX_TRNG_BIT_SIZE, &random_number);
            if (CY_CRYPTO_SUCCESS != crypt_status)
            {
                handle_error();
            }

            /* Send number in IPC */
            cm33_msg_data.client_id = CM55_IPC_PIPE_CLIENT_ID;
            cm33_msg_data.intr_mask = CY_IPC_CYPIPE_INTR_MASK_EP1;
            cm33_msg_data.cmd = RESET_VAL;
            cm33_msg_data.value = random_number;

            pipeStatus = Cy_IPC_Pipe_SendMessage(CM55_IPC_PIPE_EP_ADDR, \
                                                 CM33_IPC_PIPE_EP_ADDR, \
                                                 (void *) &cm33_msg_data, 0);

            if(CY_IPC_PIPE_SUCCESS != pipeStatus)
            {
                handle_error();
            }

            break;

            default:
            /* Do nothing */
            break;
        }

        msg_cmd = RESET_VAL;

        Cy_SysLib_Delay(CM33_APP_DELAY_MS);
    }
}