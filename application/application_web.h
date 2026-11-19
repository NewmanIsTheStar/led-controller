/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef APPLICATION_WEB_H
#define APPLICATION_WEB_H


typedef struct APP_WEB_VARIABLES
{
  char hello_world_web_variable[128];   // example web variable          
} APP_WEB_VARIABLES_T;                  //remember to add initialization code when adding to this structure !!!

typedef struct 
{
  int hello_world_state_variable;   // example web state variable -- add more to the structure as needed
} WEB_SESSION_STATE_T;

typedef const char *(*tMYCGIHandler)(int iIndex, int iNumParams, char *pcParam[], char *pcValue[], WEB_SESSION_STATE_T *connection_state);
typedef struct
{
    const char *pcCGIName;
    tMYCGIHandler pfnCGIHandler;
} MY_CGI_T;

int application_web_init(void);


#endif

