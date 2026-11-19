
/**
 * Copyright (c) 2026 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "application_config.h"
#include "application_web.h"
#include "watchdog.h"
#include "pluto.h"
#include "shell.h"

//prototypes


// external variables
extern uint32_t unix_time;
extern APP_CONFIG_T config;
extern APP_WEB_VARIABLES_T app_web;

//static variables


/*!
 * \brief hello world task
 *
 * \param[in]  params  alive counter that must be incremented periodically to prevent watchdog reset
 * 
 * \return nothing
 */
void hello_world_task(void *params) 
{
    printf("Hello World task started\n");

    while (true)
    {        
        printf("Hello World!\n");
        shell_printf("Hello World!\n");

        SLEEP_MS(5000);

        // tell watchdog task that we are still alive
        watchdog_pulse((int *)params);  
    } 
}

