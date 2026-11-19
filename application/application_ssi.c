/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "lwip/sockets.h"
#include "lwip/apps/httpd.h"
#include "calendar.h"
#include "utility.h"
#include "application_config.h"
#include "pluto.h"
#include "system_web.h"
#include "application_web.h"
// #include "hc_task.h"
#include "system_ssi.h"

#ifdef USE_GIT_HASH_AS_VERSION
#include "githash.h"
#endif

extern uint32_t unix_time;
extern APP_WEB_VARIABLES_T app_web;
extern SYS_WEB_VARIABLES_T sys_web;
extern APP_CONFIG_T config;




u16_t application_ssi_handler(int iIndex, char *pcInsert, int iInsertLen, void *connection_state)
{
    size_t printed;

    switch(iIndex) 
    {
        // *** application  SSI start ***

        case SSI_hello:  // hello world example SSI
        {
            printed = snprintf(pcInsert, iInsertLen, "%s", app_web.hello_world_web_variable);    
        }
        break; 

        case SSI_world:  // hello world example SSI
        {
            printed = snprintf(pcInsert, iInsertLen, "%s", cfg->hello_world_config_variable);    
        }
        break;       
                                 

         // *** application  SSI end ***
        /************************************************************************************************/

        default:
        {
            printed = snprintf(pcInsert, iInsertLen, "Unhandled SSI tag");    
        }
        break;                 
    }


    return ((u16_t)printed);
}



