#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdatomic.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.pio.h"
#include "led_strip.h"
#include "pluto.h"
#include "watchdog.h"
#include "application_config.h"

void animation_scan(rgbw_t *buffer);
void animation_breath(rgbw_t *buffer);
void animation_police(rgbw_t *buffer);

extern atomic_uint_fast8_t  g_master_brightness;


void led_animation_task(void *params)
{
    int test = 0;


    // Optional: warm up cool-white SK6812 strips by slightly trimming Blue and White
    led_set_white_balance(255, 240, 210, 235);
    led_set_master_brightness(180);

    for (;;) 
    {
        rgbw_t *buf = led_get_back_buffer();

        animation_police(buf);

        // Atomically hand off frame to vLedRenderTask
        led_submit_frame();

        watchdog_pulse((int *)params);
        vTaskDelay(pdMS_TO_TICKS(16)); // Producer rate is completely decoupled from render rate
    }
}

/*!
 * \brief led pattern generator 
 *
 * \param[in]  buffer  led control buffer
 * 
 * \return nothing
 */
void animation_scan(rgbw_t *buffer)
{
    static uint position = 0;
    static uint direction = 0;
    static uint color = 0;
    static uint cycle = 0;
    static bool color_change = false;
    static int led_index = 0;

    // change color
    if ((color_change) && (cycle >= 50))
    {
        color_change = false;
        color++;
    }

    // render current position
    for(led_index=0; led_index < cfg->led_number; led_index++)
    {
        if (led_index == position)
        {
            // turn on led
            switch(color%3)
            {
                default:
                case 0:
                    buffer[led_index].r = 0xff;
                    buffer[led_index].g = 0;
                    buffer[led_index].b = 0;
                    buffer[led_index].w = 0;                    
                    break;
                case 1:
                    buffer[led_index].r = 0;
                    buffer[led_index].g = 0xff;
                    buffer[led_index].b = 0;
                    buffer[led_index].w = 0;                    
                    break;
                case 2:
                    buffer[led_index].r = 0;
                    buffer[led_index].g = 0;
                    buffer[led_index].b = 0xff;
                    buffer[led_index].w = 0;                    
                    break;
            }                  
        }
        else
        {
            // turn off led
            buffer[led_index].r = 0;
            buffer[led_index].g = 0;
            buffer[led_index].b = 0;
            buffer[led_index].w = 0;

        }
    }

    // change patten
    if (cycle++ >= 100)   // hold patten for X cycles
    {
        cycle = 0;

        // shift position
        if (direction == 0)
        {
            position++;
            if (position >= (cfg->led_number-1))
            {
                // reverse direction
                direction = 1;
                color_change = true;
            }
        }
        else
        {
            position--;
            if (position == 0)
            {
                // reverse direction
                direction = 0;
                color_change = true;
            } 
        }
    }
}

/*!
 * \brief led pattern generator 
 *
 * \param[in]  buffer  led control buffer
 * 
 * \return nothing
 */
void animation_breath(rgbw_t *buffer)
{
    static uint8_t brightness = 0;
    static uint8_t pause = 0;
    static int state = 0;
    static int i;
    static int step;
    static int led_index = 0;


    for(led_index=0; led_index < cfg->led_number; led_index++)
    {           
        buffer[led_index].r = 0;
        buffer[led_index].g = brightness;
        buffer[led_index].b = 0;
        buffer[led_index].w = 0;
    }

    step = 1;

    switch(state)
    {
    default:
    case 0:
        if (brightness > (255-step))
        {
            brightness = 255;
            state = 1;
        }
        else
        {
            brightness += step;
        }
        break;
    case 1:
        if (brightness < step)
        {
            brightness = 0;
            state = 2;
        } 
        else
        {
            brightness -= step;
        }           
        break;
    case 2:
        if (++pause == 8)
        {
            state = 0;
            pause = 0;
        } 
        break;        
    }
}

/*!
 * \brief led pattern generator 
 *
 * \param[in]  buffer  led control buffer
 * 
 * \return nothing
 */
void animation_police(rgbw_t *buffer)
{
    static uint8_t brightness = 0;
    static uint8_t pause = 0;
    static uint cycle = 0;
    static int state = 0;
    static int led_index;
    static int step;

        // change patten
    if (cycle++ >= 100)   // hold patten for X cycles
    {
        cycle = 0;
        
        state = !state;
    }

    switch(state)
    {
        case 0: // =.==.=
            for (led_index=0; led_index<cfg->led_number; led_index++)
            {
                if (((led_index >= cfg->led_number/6*1) && (led_index < cfg->led_number/6*2)) ||
                    ((led_index >= cfg->led_number/6*4) && (led_index < cfg->led_number/6*5)))
                {
                    // off
                    buffer[led_index].r = 0;
                    buffer[led_index].g = 0;
                    buffer[led_index].b = 0;
                    buffer[led_index].w = 0;
                }
                else
                {
                    if (led_index < cfg->led_number/2)
                    {
                        // red
                        buffer[led_index].r = 0xff;
                        buffer[led_index].g = 0;
                        buffer[led_index].b = 0;
                        buffer[led_index].w = 0;                        
                    }
                    else
                    {
                        // blue
                        buffer[led_index].r = 0;
                        buffer[led_index].g = 0;
                        buffer[led_index].b = 0xff;
                        buffer[led_index].w = 0;                        
                    }                    
                }
            }
            //state = 1;
            break;
        case 1: // .=..=. 
            for (led_index=0; led_index<cfg->led_number; led_index++)
            {
                if (((led_index >= cfg->led_number/6*1) && (led_index < cfg->led_number/6*2)) ||
                    ((led_index >= cfg->led_number/6*4) && (led_index < cfg->led_number/6*5)))
                {
                    if (led_index < cfg->led_number/2)
                    {
                        // red
                        buffer[led_index].r = 0xff;
                        buffer[led_index].g = 0;
                        buffer[led_index].b = 0;
                        buffer[led_index].w = 0;
                    }
                    else
                    {
                        // blue
                        buffer[led_index].r = 0;
                        buffer[led_index].g = 0;
                        buffer[led_index].b = 0xff;
                        buffer[led_index].w = 0;  
                    }
                }
                else
                {
                    // off
                    buffer[led_index].r = 0;
                    buffer[led_index].g = 0;
                    buffer[led_index].b = 0;
                    buffer[led_index].w = 0;
                }
            }
            //state = 0;
            break;  
          
    }
}