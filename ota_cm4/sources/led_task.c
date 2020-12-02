/******************************************************************************
* File Name: led_task.c
*
* Description: This file contains definition of a function that handles
* blinking an LED.
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
#include "platform/iot_threads.h"
#include "cybsp.h"
#include "cyhal.h"
#include "led_task.h"


/*******************************************************************************
 * Function prototypes
 ******************************************************************************/
static void blink_led_task(void* param);


/*******************************************************************************
 * Function definitions
 ******************************************************************************/

/*******************************************************************************
 * Function Name: LED_init
 *******************************************************************************
 * Summary:
 *  This function starts a task to blink an LED.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  None
 *
 ******************************************************************************/
void led_init(void)
{
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_LED_STATE_OFF);
    Iot_CreateDetachedThread(blink_led_task, NULL, BLINK_LED_TASK_PRIORITY, BLINK_LED_TASK_STACK_SIZE);

}


/*******************************************************************************
 * Function Name: blink_led_task
 *******************************************************************************
 * Summary:
 *  This task toggles the GPIO pin connected to the LED. The delay between
 *  successive toggles is specified by LED_BLINK_DELAY_MS.
 *
 * Parameters:
 *  void* param: unused argument
 *
 * Return:
 *  None
 *
 ******************************************************************************/
static void blink_led_task(void* param)
{
    for(;;)
    {
        cyhal_gpio_toggle(CYBSP_USER_LED);
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
    }
}

/* [] END OF FILE */
