/*
 * Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
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
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */
 /** @file
 *
 * This application shows a basic test of how to add a command
  *
 */

#include "cyhal.h"
#include "cybsp.h"
#include <cy_retarget_io.h>
#include <cybsp_wifi.h>
#include <stdio.h>
#include "cyabs_rtos.h"
#include "command_console.h"
#include "wifi_utility.h"
#include "cy_wcm.h"
#include "iperf_utility.h"
#include "bt_utility.h"
#include "bt_cfg.h"


#define THREAD_STACK 4*1024
static cy_thread_t nxs_thread;
static uint64_t nxs_stack[(THREAD_STACK)/sizeof(uint64_t)];

/* wcm parameters */
static cy_wcm_config_t wcm_config;
static cy_wcm_connect_params_t conn_params;

static cy_timer_t wdt_timer_t;

cy_rslt_t ConnectWifi();

#define CONSOLE_COMMAND_MAX_PARAMS     (32)
#define CONSOLE_COMMAND_MAX_LENGTH     (85)
#define CONSOLE_COMMAND_HISTORY_LENGTH (10)

const char* console_delimiter_string = " ";

static char command_buffer[CONSOLE_COMMAND_MAX_LENGTH];
static char command_history_buffer[CONSOLE_COMMAND_MAX_LENGTH * CONSOLE_COMMAND_HISTORY_LENGTH];

#define WIFI_SSID                        ""
#define WIFI_KEY                         ""
#define WIFI_SECURITY                    CY_WCM_SECURITY_WPA2_AES_PSK
#define WIFI_BAND                        CY_WCM_WIFI_BAND_ANY
#define CMD_CONSOLE_MAX_WIFI_RETRY_COUNT 15
#define IP_STR_LEN                       16
#define WDT_TIMEOUT_MS                   (4000)
#define CY_RSLT_ERROR                    ( -1 )

#if defined(CPU_CLOCK_FREQUENCY)
#if (CYHAL_API_VERSION >= 2)
static cy_rslt_t set_cpu_clock ( uint32_t freq, cyhal_clock_t *obj );
#endif
#endif

extern int handle_bt_on(int argc, char *argv[], tlv_buffer_t** data);
cy_rslt_t command_console_add_command();

static void get_ip_string(char* buffer, uint32_t ip)
{
    sprintf(buffer, "%lu.%lu.%lu.%lu",
            (unsigned long)(ip      ) & 0xFF,
            (unsigned long)(ip >>  8) & 0xFF,
            (unsigned long)(ip >> 16) & 0xFF,
            (unsigned long)(ip >> 24) & 0xFF);
}

cy_rslt_t ConnectWifi()
{
    cy_rslt_t res ;

    const char *ssid = WIFI_SSID ;
    const char *key = WIFI_KEY ;
    cy_wcm_wifi_band_t band = WIFI_BAND;
    int retry_count = 0;
    cy_wcm_ip_address_t ip_addr;
    char ipstr[IP_STR_LEN];

    memset(&conn_params, 0, sizeof(cy_wcm_connect_params_t));

    while (1)
    {
        /*
        * Join to WIFI AP
        */
        memcpy(&conn_params.ap_credentials.SSID, ssid, strlen(ssid) + 1);
        memcpy(&conn_params.ap_credentials.password, key, strlen(key) + 1);
        conn_params.ap_credentials.security = WIFI_SECURITY;
        conn_params.band = band;

        res = cy_wcm_connect_ap(&conn_params, &ip_addr);
        cy_rtos_delay_milliseconds(500);

        if(res != CY_RSLT_SUCCESS)
        {
            retry_count++;
            if (retry_count >= CMD_CONSOLE_MAX_WIFI_RETRY_COUNT)
            {
                printf("Exceeded max WiFi connection attempts\n");
                return CY_RSLT_ERROR;
            }
            printf("Connection to WiFi network failed. Retrying...\n");
            continue;
        }
        else
        {
            printf("Successfully joined wifi network '%s , result = %ld'\n", ssid, (long)res);
            get_ip_string(ipstr, ip_addr.ip.v4);
            printf("IP Address %s assigned\n", ipstr);
            break;
        }
    }

    return CY_RSLT_SUCCESS;
}

cy_rslt_t command_console_add_command(void) {

    cy_command_console_cfg_t console_cfg;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    printf( "executing command_console_add_remove_command \n");
    console_cfg.serial             = (void *)&cy_retarget_io_uart_obj;
    console_cfg.line_len           = sizeof(command_buffer);
    console_cfg.buffer             = command_buffer;
    console_cfg.history_len        = CONSOLE_COMMAND_HISTORY_LENGTH;
    console_cfg.history_buffer_ptr = command_history_buffer;
    console_cfg.delimiter_string   = console_delimiter_string;
    console_cfg.params_num         = CONSOLE_COMMAND_MAX_PARAMS;
    console_cfg.thread_priority    = CY_RTOS_PRIORITY_NORMAL;
    console_cfg.delimiter_string   = " ";

    /* Initialize command console library */
    result = cy_command_console_init(&console_cfg);
    if ( result != CY_RSLT_SUCCESS )
    {
        printf ("Error in initializing command console library : %ld \n", (long)result);
        goto error;
    }

#ifndef DISABLE_COMMAND_CONSOLE_WIFI
    /* Initialize Wi-Fi utility and add Wi-Fi commands */
    result = wifi_utility_init();
    if ( result != CY_RSLT_SUCCESS )
    {
        printf ("Error in initializing command console library : %ld \n", (long)result);
        goto error;
    }
#endif

#ifndef DISABLE_COMMAND_CONSOLE_BT
    /* Initialize Bluetooth utility and add BT commands */
    bt_utility_init();
#endif

#ifndef DISABLE_COMMAND_CONSOLE_IPERF
    /* Initialize IPERF utility and add IPERF commands */
    iperf_utility_init(&wcm_config.interface);
#endif

    return CY_RSLT_SUCCESS;

error:
    return CY_RSLT_ERROR;

}

static void wdt_handler(cy_timer_callback_arg_t arg)
{
    thread_ap_watchdog_ConfigureTime(5);
}

static void console_task(cy_thread_arg_t arg)
{
    cy_rslt_t res;

#ifndef DISABLE_COMMAND_CONSOLE_BT
    handle_bt_on(0, NULL, NULL);
#endif

#ifndef DISABLE_COMMAND_CONSOLE_WIFI
    /* Initialize wcm */
    wcm_config.interface = CY_WCM_INTERFACE_TYPE_AP_STA;
    res = cy_wcm_init(&wcm_config);
    if(res != CY_RSLT_SUCCESS)
    {
        printf("cy_wcm_init failed with error: %x\n", (unsigned int)res);
        return;
    }
    printf("WCM Initialized\n");

    /* Connect to an AP for which credentials are specified */
    ConnectWifi();
#endif

    command_console_add_command();

    /* Periodic timer is added to pet the WDT. This is required for running iperf tests without a Watchdog reset */
    cy_rtos_init_timer(&wdt_timer_t, CY_TIMER_TYPE_PERIODIC, wdt_handler, 0);
    cy_rtos_start_timer(&wdt_timer_t, WDT_TIMEOUT_MS);
    thread_ap_watchdog_ConfigureTime(5);

    while(1)
    {
        cy_rtos_delay_milliseconds(500);
    }
}


int main(void)
{
    cy_rslt_t result ;
#if defined(CPU_CLOCK_FREQUENCY)
    cyhal_clock_t obj;
#endif

    /* Initialize the board support package */
    result = cybsp_init() ;
    if( result != CY_RSLT_SUCCESS)
    {
        printf("cybsp_init failed %ld\n", (long)result);
        return CY_RSLT_ERROR;
    }
    CY_ASSERT(result == CY_RSLT_SUCCESS) ;
    /* Enable global interrupts */
    __enable_irq();

#ifdef COMPONENT_55900
    /* For CYW55913, the BTSS sleep is enabled by default.
     * command-console will not work with sleep enabled as wake on uart is not enabled.
     * Once wake on uart is enabled, the sleep lock can be removed.
     * mtb-tester-threadx-wifi-bluetooth application will not operate in low power mode until wake on uart is enabled.
     */
    cyhal_syspm_lock_deepsleep();
#endif

#if defined(CPU_CLOCK_FREQUENCY)
#if (CYHAL_API_VERSION >= 2)
    /* set CPU clock to CPU_CLOCK_FREQUENCY */
    result = set_cpu_clock(CPU_CLOCK_FREQUENCY, &obj);
    CY_ASSERT(result == CY_RSLT_SUCCESS) ;
#endif
#endif

    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    printf("Command console application - AnyCloud !!!\r\n\n");

    result = cy_rtos_thread_create(&nxs_thread,
                                   &console_task,
                                   "ConsoleTask",
                                   &nxs_stack,
                                   THREAD_STACK,
                                   CY_RTOS_PRIORITY_LOW,
                                   0);
                                   
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Main thread creation failed \r\n");
        CY_ASSERT(0);
    }

    return 0;
}

#if defined(CPU_CLOCK_FREQUENCY)
#if (CYHAL_API_VERSION >= 2)
static cy_rslt_t set_cpu_clock ( uint32_t freq, cyhal_clock_t *obj )
{
    cy_rslt_t rslt;

    if (CY_RSLT_SUCCESS == (rslt = cyhal_clock_get(obj, &CYHAL_CLOCK_RSC_CPU)))
    {
        rslt = cyhal_clock_set_frequency(obj, freq, NULL);
    }

    return rslt;
}
#endif
#endif
