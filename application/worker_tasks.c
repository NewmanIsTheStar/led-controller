/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "task.h"
#include "worker_tasks.h"

#include "hello_world_task.h"
#include "led_strip.h"
#include "message.h"
#include "mqtt.h"

// worker tasks to launch and monitor
WORKER_TASK_T worker_tasks[] =
{
  //  function            name                    stack   priority            
    {   hello_world_task, "Hello World Task",     8096,   1},          
    {   led_strip_task,   "LED Strip Task",       1024,   1},  
    {   message_task,     "Message Task",         1024,   1},  
    {   mqtt_task,        "MQTT Task",            8096,   1},  
  
    // end of table
    {   NULL,             NULL,                   0,      0}
};


