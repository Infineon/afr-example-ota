/*
 * Amazon FreeRTOS V1.4.7
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/*******************************************************************************
* Includes
*******************************************************************************/
/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#ifdef CY_BOOT_USE_EXTERNAL_FLASH
#include "flash_qspi.h"
#include "cy_smif_psoc6.h"
#include "cy_serial_flash_qspi.h"
#endif

#ifdef CY_USE_LWIP
#include "lwip/tcpip.h"
#endif

/* Demo includes */
#include "aws_demo.h"

/* AWS library includes. */
#include "iot_system_init.h"
#include "iot_logging_task.h"
#include "iot_wifi.h"
#include "aws_clientcredential.h"
#include "aws_application_version.h"
#include "aws_dev_mode_key_provisioning.h"

/* BSP & Abstraction inclues */
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "iot_network_manager_private.h"

#include "led_task.h"


/*******************************************************************************
* Macros
*******************************************************************************/
/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 90 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 8 )

/* The task delay for allowing the lower priority logging task to print out Wi-Fi
 * failure status before blocking indefinitely. */
#define mainLOGGING_WIFI_STATUS_DELAY       pdMS_TO_TICKS( 1000 )

/* Unit test defines. */
#define mainTEST_RUNNER_TASK_STACK_SIZE     ( configMINIMAL_STACK_SIZE * 16 )

/* The name of the devices for xApplicationDNSQueryHook. */
#define mainDEVICE_NICK_NAME                "cypress_demo" /* FIX ME.*/

/* Maximum number of attempts at retrying to connect to AP. */
#define MAX_WIFI_RETRY_COUNT                (3)

/* Delay in milliseconds between successive Wi-Fi connection attempts. */
#define WIFI_CONN_RETRY_INTERVAL_MSEC       (100u)


/*******************************************************************************
* Function prototypes
*******************************************************************************/
void vApplicationDaemonTaskStartupHook( void );
static void prvWifiConnect( void );
static void prvMiscInitialization( void );


/*******************************************************************************
* Global variables
********************************************************************************/
extern int uxTopUsedPriority;


/*******************************************************************************
 * Function definitions
*******************************************************************************/
/******************************************************************************
 * Function Name: main
 ******************************************************************************
 * Summary:
 *  System entrance point. This function initializes peripherals and starts the
 *  FreeRTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
 ******************************************************************************/
int main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    BaseType_t xReturnMessage;

    prvMiscInitialization();

    /* Create tasks that are not dependent on the Wi-Fi being initialized. */
    xReturnMessage = xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                                             tskIDLE_PRIORITY,
                                             mainLOGGING_MESSAGE_QUEUE_LENGTH );

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the Wi-Fi initialization, is performed in the RTOS daemon task
     * startup hook. */
    if (pdPASS == xReturnMessage)
    {
        vTaskStartScheduler();
    }

    if (pdPASS == xReturnMessage)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


/******************************************************************************
 * Function Name: prvMiscInitialization
 ******************************************************************************
 * Summary:
 *  Initializes board resources.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void prvMiscInitialization( void )
{
    cy_rslt_t result = cybsp_init();
    configASSERT(CY_RSLT_SUCCESS == result);

    __enable_irq();

    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    configASSERT(CY_RSLT_SUCCESS == result);
}


/******************************************************************************
 * Function Name: vApplicationDaemonTaskStartupHook
 ******************************************************************************
 * Summary:
 *  Application task startup hook for applications using Wi-Fi. If you are not
 *  using Wi-Fi, then start network dependent applications in the 
 *  vApplicationIPNetorkEventHook function. If you are not using Wi-Fi, this
 *  hook can be disabled by setting configUSE_DAEMON_TASK_STARTUP_HOOK to 0.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void vApplicationDaemonTaskStartupHook( void )
{
    /* FIX ME: If your MCU is using Wi-Fi, delete surrounding compiler directives to
     * enable the unit tests and after MQTT, Bufferpool, and Secure Sockets libraries
     * have been imported into the project. If you are not using Wi-Fi, see the
     * vApplicationIPNetworkEventHook function. */
    CK_RV xResult;

#ifdef CY_BOOT_USE_EXTERNAL_FLASH
    if (psoc6_qspi_init() != 0)
    {
       configPRINTF(("psoc6_qspi_init() FAILED!!\r\n"));
       configASSERT(0);
    }
#endif /* CY_BOOT_USE_EXTERNAL_FLASH */

    if( SYSTEM_Init() == pdPASS )
    {
#ifdef CY_USE_LWIP
        /* Initialize lwIP stack. This needs the RTOS to be up since this function will spawn
         * the tcp_ip thread.
         */
        tcpip_init(NULL, NULL);
#endif
        /* Connect to the Wi-Fi before running the tests. */
        prvWifiConnect();

#if ( pkcs11configVENDOR_DEVICE_CERTIFICATE_SUPPORTED == 0 )
        /* Provision the device with AWS certificate and private key. */
        xResult = vDevModeKeyProvisioning();
#endif
        led_init();

        /* Start the demo tasks. */
        if (xResult == CKR_OK)
        {
            DEMO_RUNNER_RunDemos();
        }
    }
}


/******************************************************************************
 * Function Name: prvWifiConnect
 ******************************************************************************
 * Summary:
 *  This function initializes the Wi-Fi device, attempts to connect to the Wi-Fi
 *  network for a maximum of MAX_WIFI_RETRY_COUNT times. Once connected, the IP
 *  address is displayed on the serial terminal.
 * 
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void prvWifiConnect( void )
{
    WIFINetworkParams_t xNetworkParams;
    WIFIReturnCode_t xWifiStatus;
    uint8_t ucTempIp[4] = { 0 };

    xWifiStatus = WIFI_On();

    if( xWifiStatus == eWiFiSuccess )
    {

        configPRINTF( ( "Wi-Fi module initialized. Connecting to AP %s...\r\n", clientcredentialWIFI_SSID ) );
    }
    else
    {
        configPRINTF( ( "Wi-Fi module failed to initialize.\r\n" ) );

        /* Delay to allow the lower priority logging task to print the above
         * status. The while loop below will block the above printing.
         */
        vTaskDelay( mainLOGGING_WIFI_STATUS_DELAY );
        configASSERT(0);
    }

    /* Setup parameters. */
    xNetworkParams.pcSSID = clientcredentialWIFI_SSID;
    xNetworkParams.ucSSIDLength = sizeof( clientcredentialWIFI_SSID ) - 1;
    xNetworkParams.pcPassword = clientcredentialWIFI_PASSWORD;
    xNetworkParams.ucPasswordLength = sizeof( clientcredentialWIFI_PASSWORD ) - 1;
    xNetworkParams.xSecurity = clientcredentialWIFI_SECURITY;
    xNetworkParams.cChannel = 0;

    for(uint32_t conn_retries = 0; conn_retries < MAX_WIFI_RETRY_COUNT; conn_retries++ )
    {
        xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );
        if (xWifiStatus == eWiFiSuccess)
        {
            configPRINTF(("Wi-Fi Connected to AP %s. Creating tasks which use network...\r\n", clientcredentialWIFI_SSID));

            xWifiStatus = WIFI_GetIP(ucTempIp);
            if (eWiFiSuccess == xWifiStatus)
            {
                configPRINTF(("IP Address acquired %d.%d.%d.%d\r\n",
                              ucTempIp[0], ucTempIp[1], ucTempIp[2], ucTempIp[3]));
            }

            break;
        }

        configPRINTF(("Connection to Wi-Fi network failed with error code %d. Retrying in %d ms...\n", (int)xWifiStatus, WIFI_CONN_RETRY_INTERVAL_MSEC));

        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MSEC));
    }

    if (xWifiStatus != eWiFiSuccess)
    {
        configPRINTF(("Failed to connect to AP. \n"));
        configASSERT(0);
    }
}


/******************************************************************************
 * Function Name: vApplicationIdleHook
 ******************************************************************************
 * Summary:
 *  User defined Idle task function. Please note that no blocking operations
 *  should be performed in this function.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void vApplicationIdleHook( void )
{
    /* FIX ME. If necessary, update to application idle periodic actions. */

    static TickType_t xLastPrint = 0;
    TickType_t xTimeNow;
    const TickType_t xPrintFrequency = pdMS_TO_TICKS( 5000 );

    xTimeNow = xTaskGetTickCount();

    if( ( xTimeNow - xLastPrint ) > xPrintFrequency )
    {
        xLastPrint = xTimeNow;
    }
}


/******************************************************************************
 * Function Name: vApplicationTickHook
 ******************************************************************************
 * Summary:
 *  User defined tick hook function.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void vApplicationTickHook()
{

}


/******************************************************************************
 * Function Name: vAssertCalled
 ******************************************************************************
 * Summary:
 * User defined assertion call. This function is plugged into configASSERT.
 * See FreeRTOSConfig.h to define configASSERT to something different.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void vAssertCalled(const char * pcFile,
    uint32_t ulLine)
{
    const uint32_t ulLongSleep = 1000UL;
    volatile uint32_t ulBlockVariable = 0UL;
    volatile char * pcFileName = (volatile char *)pcFile;
    volatile uint32_t ulLineNumber = ulLine;

    (void)pcFileName;
    (void)ulLineNumber;

    configPRINTF( ("vAssertCalled %s, %ld\n", pcFile, (long)ulLine) );

    /* Setting ulBlockVariable to a non-zero value in the debugger will allow
     *  this function to be exited.
     */
    taskDISABLE_INTERRUPTS();
    {
        while (ulBlockVariable == 0UL)
        {
            vTaskDelay( pdMS_TO_TICKS( ulLongSleep ) );
        }
    }
    taskENABLE_INTERRUPTS();
}


/* [] END OF FILE */
