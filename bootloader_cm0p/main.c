/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the bootloader application that runs
* on CM0p in afr-example-ota. 
*
* Related Document: See README.md
*
*******************************************************************************
* (c) 2020, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/

/* Drive header files */
#include "cy_pdl.h"
#include "cycfg.h"
#include "cy_result.h"
#include "cy_retarget_io_pdl.h"

/* Flash PAL header files */
#ifdef CY_BOOT_USE_EXTERNAL_FLASH
#include "flash_qspi.h"
#include "cy_smif_psoc6.h"
#endif
#include "sysflash/sysflash.h"
#include "flash_map_backend/flash_map_backend.h"

/* MCUboot header files */
#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/sign_key.h"
#include "bootutil/bootutil_log.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Delay for which CM0+ waits before enabling CM4 so that the messages written
 * to UART by CM0+ can finish printing. This delay is required since CM4 uses 
 * the same UART for printf redirect. 
 */
#define CM4_BOOT_DELAY_MS       (100UL)

/* Slave Select line to which the external memory is connected.
 * Acceptable values are:
 * 0 - SMIF disabled (no external memory)
 * 1, 2, 3, or 4 - slave select line to which the memory module is connected. 
 */
#define QSPI_SLAVE_SELECT_LINE  (1UL)


/******************************************************************************
 * Function Name: hw_deinit
 ******************************************************************************
 * Summary:
 *  This function de-initializes hardware resources like QSPI and UART.
 *
 ******************************************************************************/
void hw_deinit(void)
{
    cy_retarget_io_pdl_deinit();
    Cy_GPIO_Port_Deinit(CYBSP_UART_RX_PORT);
    Cy_GPIO_Port_Deinit(CYBSP_UART_TX_PORT);

#ifdef CY_BOOT_USE_EXTERNAL_FLASH
    qspi_deinit(QSPI_SLAVE_SELECT_LINE);
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */
}


/******************************************************************************
 * Function Name: do_boot
 ******************************************************************************
 * Summary:
 *  This function extracts the image address and enables CM4 to let it boot
 *  from that address.
 *
 * Parameters:
 *  rsp - Pointer to a structure holding the address to boot from. 
 *
 ******************************************************************************/
static void do_boot(struct boot_rsp *rsp)
{
    uint32_t app_addr = (rsp->br_image_off + rsp->br_hdr->ih_hdr_size);

    BOOT_LOG_INF("Starting User Application on CM4. Please wait...");
    BOOT_LOG_INF("Start Address: 0x%08lx", app_addr);
    BOOT_LOG_INF("Deinitializing hardware...");
    cy_retarget_io_wait_tx_complete(CYBSP_UART_HW, CM4_BOOT_DELAY_MS);
    hw_deinit();
    Cy_SysEnableCM4(app_addr);
}


/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes peripherals, initializes
 *  retarget IO, and performs a boot by calling the MCUboot functions. 
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main(void)
{
    struct boot_rsp rsp;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize system resources and peripherals.*/
    init_cycfg_all();

    /* Enable interrupts */
    __enable_irq();

    /* Initialize retarget-io to redirect the printf output */
    result = cy_retarget_io_pdl_init(CY_RETARGET_IO_BAUDRATE);

    CY_ASSERT(CY_RSLT_SUCCESS == result);
    BOOT_LOG_INF("MCUboot Bootloader Started");

#ifdef CY_BOOT_USE_EXTERNAL_FLASH
    /* Initialize QSPI NOR flash using SFDP. */
    result = qspi_init_sfdp(QSPI_SLAVE_SELECT_LINE);

    if(CY_SMIF_SUCCESS == result)
    {
        BOOT_LOG_INF("External Memory initialized using SFDP");
    }
    else
    {
        BOOT_LOG_ERR("External Memory initialization using SFDP FAILED: 0x%02x", (int)result);
        CY_ASSERT(0);
    }
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */

    result = boot_go(&rsp);

    if (CY_RSLT_SUCCESS == result)
    {
        BOOT_LOG_INF("User Application validated successfully");
        do_boot(&rsp);
    }
    else
    {
        BOOT_LOG_INF("MCUboot Bootloader found no bootable image");
    }

    while (true)
    {
        if (CY_RSLT_SUCCESS == result)
        {
            Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
        }
        else
        {
            __WFI();
        }
    }

    /* The function never returns.*/
    return 0;
}


/* [] END OF FILE */
