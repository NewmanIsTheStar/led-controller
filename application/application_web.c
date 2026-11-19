/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "pico/cyw43_arch.h"
#include "calendar.h"
#include "pluto.h"
#include "system_web.h"
#include "application_web.h"

// global variables
APP_WEB_VARIABLES_T app_web;

/*!
 * \brief Initialize web interface variables
 *
 * \return 0 on success
 */
int application_web_init(void)
{
    // zero web structure
    memset(&app_web, 0, sizeof(app_web));

    app_web.hello_world_web_variable[0] = 0;
    sprintf(app_web.hello_world_web_variable, "Nothing entered yet");

    return(0);
}