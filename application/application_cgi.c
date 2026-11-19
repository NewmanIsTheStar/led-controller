/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #define _GNU_SOURCE
 
#include "lwip/apps/fs.h"
#include "lwip/sockets.h"
#include "FreeRTOS.h"
#include "task.h"
#include "utility.h"
#include "application_config.h"
#include "worker_tasks.h"
#include "pluto.h"
#include "system_web.h"
#include "application_web.h"
#include "shell.h"



extern APP_CONFIG_T config;
extern APP_WEB_VARIABLES_T app_web;
extern SYS_WEB_VARIABLES_T sys_web;
extern WORKER_TASK_T worker_tasks[];

extern char current_calendar_web_page[50];


/*!
 * \brief cgi handler   
 *
 * \param[in]  iIndex       index of cgi handler in cgi_handlers table
 * \param[in]  iNumParams   number of parameters
 * \param[in]  pcParam      parameter name
 * \param[in]  pcValue      parameter value 
 * 
 * \return nothing
 */
const char * cgi_hello_wworld_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[], WEB_SESSION_STATE_T *connection_state)
{
    int i = 0;
    //int whole_part = 0;
    //int tenths_part = 0;
    char *param = NULL;
    char *value = NULL;
    //int new_value = 0;

       
    //dump_parameters(iIndex, iNumParams, pcParam, pcValue);

    i = 0;
    while (i < iNumParams)
    {
        param = pcParam[i];
        value = pcValue[i];

        if (param && value)
        {
            //printf("Parameter: %s has Value: %s\n", param, value);
            if (strcasecmp("world", param) == 0)
            {
                // Example 1: in this exmple we store the value of parameter x in the application configuration
                sscanf(value, "%s", cfg->hello_world_config_variable); 
                urldecode(cfg->hello_world_config_variable, cfg->hello_world_config_variable);
                if (strcasecmp(cfg->hello_world_config_variable, "Hello World") == 0)
                {
                    snprintf(app_web.hello_world_web_variable, sizeof(app_web.hello_world_web_variable), "User entered the correct response: %s\n", cfg->hello_world_config_variable);
                    printf("User entered the correct response: %s\n", cfg->hello_world_config_variable);
                    shell_printf("User entered the correct response: %s\n", cfg->hello_world_config_variable);
                    
                }
                else
                {
                    snprintf(app_web.hello_world_web_variable, sizeof(app_web.hello_world_web_variable), "User entered an incorrect response: %s\n", cfg->hello_world_config_variable);                   
                    printf("User entered an incorrect response: %s\n", cfg->hello_world_config_variable);
                    shell_printf("User entered an incorrect response: %s\n", cfg->hello_world_config_variable);
                }

                // Example 2: in this exmple we store the value of parameter x in the connection_state structure
                // this structure is passed to subsequent SSI / CGI function calls for this web page
                // thus enabling behavior to be altered based on prior user actions  
                sscanf(value, "%d", &(connection_state->hello_world_state_variable));                                 
            }                 
        }

        i++;
    }

    // Send the next page back to the user
    return "/hello_world_edit.shtml";
}



static const MY_CGI_T application_cgi_handlers[] = 
{
    {"/cgi_hello_world.shtml",         cgi_hello_wworld_handler},  
        
};

/**
 * Global CGI handler used when LWIP_HTTPD_CGI_SSI == 1 (new style)
 * 
 * @param uri The URI requested by the client (e.g., "/cgi_leds.shtml")
 * @param iNumParams Number of parameters parsed from the query string
 * @param pcParam Array of parameter name strings
 * @param pcValue Array of parameter value strings
 * @return The filename/URI of the page to load next (e.g., "/index.shtml")
 */
void application_cgi_handler(struct fs_file *file, const char* uri, int iNumParams, 
                       char **pcParam, char **pcValue, void *connection_state) 
{
    int i;
    WEB_SESSION_STATE_T *session = NULL;

    // all html files requiring cgi have file names begining with "/cgi_"
    if (strncasecmp("/cgi_", uri, 5) == 0)
    {        
        for(i=0; i<NUM_ROWS(application_cgi_handlers); i++)
        {
            //printf("checking CGI URI: %s vs %s\n", application_cgi_handlers[i].pcCGIName, uri);
            if (strcasecmp(application_cgi_handlers[i].pcCGIName, uri) == 0) 
            {
                if (application_cgi_handlers[i].pfnCGIHandler)
                {                
                    printf("calling handler for %s\n", application_cgi_handlers[i].pcCGIName);
                    application_cgi_handlers[i].pfnCGIHandler(0, iNumParams, pcParam, pcValue, (WEB_SESSION_STATE_T *)connection_state);
                }
                break;
            }
        }
    }
}


/**
 * Called by lwIP when an HTTP file is opened.
 * Return a pointer to your state data or NULL if not needed.
 */
void *fs_state_init(struct fs_file *file, const char *name)
{
    WEB_SESSION_STATE_T *session = NULL;

    if (strncasecmp("/cgi_", name, 5) == 0)
    {
        if (file->state == NULL)
        {
            // allocate block memory for this specific file request session
            session = (WEB_SESSION_STATE_T *)mem_malloc(sizeof(WEB_SESSION_STATE_T));
            if (session) 
            {
                // set baseline defaults
                session->hello_world_state_variable = 0; 

                // bind custom tracking struct directly to the file instance
                file->state = session;             
                //printf("allocated memory for web context @%p\n", session);
            }
        }
        else
        {
            printf("refusing to allocate memory for web context as it already exists\n");
        } 
    }

    return session; 
}

/**
 * Called by lwIP when an HTTP file is closed.
 * Use this to free memory allocated in fs_state_init.
 */
void fs_state_free(struct fs_file *file, void *state)
{
    if (state != NULL) 
    {
        //printf("RELEASING web context memory @%p\n", file->state);    
        mem_free(state);
        file->state = NULL;
    }    
}