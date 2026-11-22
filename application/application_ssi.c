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
// extern APP_CONFIG_T config;




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

        // case SSI_world:  // hello world example SSI
        // {
        //     printed = snprintf(pcInsert, iInsertLen, "%s", cfg->hello_world_config_variable);    
        // }
        // break;       

        case SSI_lpin: //lpin
        {
            printed = snprintf(pcInsert, iInsertLen, "%d", cfg->led_pin);
        }               
        break;

        case SSI_lrgbw: //lrgbw
        {
            printed = snprintf(pcInsert, iInsertLen, "%d", cfg->led_rgbw);
        }               
        break;

        case SSI_lnum: //lnum
        {
            printed = snprintf(pcInsert, iInsertLen, "%d", cfg->led_number);
        }               
        break;  

        case SSI_clpat: //clpat
        {
            printed = snprintf(pcInsert, iInsertLen, "%d", app_web.led_current_pattern);
        } 
        break;

        case SSI_cltran: //cltran
        {
            printed = snprintf(pcInsert, iInsertLen, "%d", app_web.led_current_transition_delay);
        } 
        break; 

        case SSI_clreq: //clreq
        {
            printed = snprintf(pcInsert, iInsertLen, "%s", app_web.led_last_request_ip);
        } 
        break;         

         // *** application  SSI end ***
        /************************************************************************************************/

        default:
        {
            printed = snprintf(pcInsert, iInsertLen, "Unhandled SSI tag [%d]", iIndex);    
        }
        break;                 
    }


    return ((u16_t)printed);
}



