#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
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
#include "application_web.h"
#include "utility.h"

void animation_blank(rgbw_t *buffer, uint hold_cycles);
void animation_scan(rgbw_t *buffer, uint hold_cycles);
void animation_breath(rgbw_t *buffer, uint hold_cycles);
void animation_police(rgbw_t *buffer, uint hold_cycles);
void animation_snakes(rgbw_t *buffer, uint hold_cycles);

typedef void (*animation)(rgbw_t *buffer, uint hold_cycles);

// constants
const struct {
    animation pat;
    const char *name;
} pattern_table[] = {
        {animation_blank,   "Blank"},    
        {animation_snakes,  "Snakes"},
        {animation_scan,    "Scan"},
        {animation_police,  "Police"},
        {animation_breath,  "Breath"},     
};

extern APP_WEB_VARIABLES_T app_web;
extern atomic_uint_fast8_t  g_master_brightness;

static int live_pattern = -1;
static int live_speed = -1;
static DOUBLE_BUF_INT local_pattern;
static DOUBLE_BUF_INT local_speed;


void led_animation_task(void *params)
{
    int test = 0;
    rgbw_t *buf;

    // warm up cool-white SK6812 strips by slightly trimming Blue and White
    led_set_white_balance(255, 240, 210, 235);
    led_set_master_brightness(180);

    set_led_pattern_local(cfg->led_pattern); 

    for (;;) 
    {
        live_pattern = get_double_buf_integer(&local_pattern, 0);
        CLIP(live_pattern, 0, count_of(pattern_table)-1);

        live_speed = get_double_buf_integer(&local_speed, 0);
        CLIP(live_speed, 0, 3000);

        buf = led_get_back_buffer();
        pattern_table[live_pattern].pat(buf, live_speed);

        // animation_snakes(buf, cfg->led_speed);

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
void animation_scan(rgbw_t *buffer, uint hold_cycles)
{
    static uint position = 0;
    static uint direction = 0;
    static uint color = 0;
    static uint cycle = 0;
    static bool color_change = false;
    static int led_index = 0;

    // change color
    if ((color_change) && (cycle >= (hold_cycles/2)))
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
    if (cycle++ >= hold_cycles)
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
void animation_breath(rgbw_t *buffer, uint hold_cycles)
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
void animation_police(rgbw_t *buffer, uint hold_cycles)
{
    static uint8_t brightness = 0;
    static uint8_t pause = 0;
    static uint cycle = 0;
    static int state = 0;
    static int led_index;
    static int step;

        // change patten
    if (cycle++ >= hold_cycles)
    {
        cycle = 0;
        
        state = !state;
    }

    switch(state)
    {
        case 0: // =.==.=
            for (led_index=0; led_index < cfg->led_number; led_index++)
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



/*!
 * \brief led pattern generator 
 *
 * \param[in]  buffer  led control buffer
 * 
 * \return nothing
 */
void animation_snakes(rgbw_t *buffer, uint hold_cycles)
{
    static uint8_t brightness = 0;
    static uint8_t pause = 0;
    static uint cycle = 0;
    static int state = 0;
    static int led_index;
    static int step;
    static int direction = 1;
    static int sequence = 0;
    static int position = 0;

    // change patten
    if (cycle++ >= hold_cycles)
    {
        cycle = 0;
        
        state = !state;

        if (sequence++ > 1000)
        {
            sequence = 0;
            direction = (rand() >> 30) & 1 ? 1 : -1;
        }

        position += direction;
    }
    
    for (led_index = 0; led_index < cfg->led_number; ++led_index) 
    {
        uint x = (led_index + (position >> 1)) % 64;
    
        if (x < 10)
        {
            // red
            buffer[led_index].r = 0xff;
            buffer[led_index].g = 0;
            buffer[led_index].b = 0;
            buffer[led_index].w = 0;                        
        }
        else if (x >= 15 && x < 25)
        {
            // blue
            buffer[led_index].r = 0;
            buffer[led_index].g = 0xff;
            buffer[led_index].b = 0;
            buffer[led_index].w = 0;                        
        }                    
        else if (x >= 30 && x < 40)
        {
            // blue
            buffer[led_index].r = 0;
            buffer[led_index].g = 0;
            buffer[led_index].b = 0xff;
            buffer[led_index].w = 0;                        
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
}

/*!
 * \brief led pattern generator 
 *
 * \param[in]  buffer  led control buffer
 * 
 * \return nothing
 */
void animation_blank(rgbw_t *buffer, uint hold_cycles)
{
    static uint8_t brightness = 0;
    static uint8_t pause = 0;
    static uint cycle = 0;
    static int state = 0;
    static int led_index;
    static int step;

    // change update patten
    if (cycle++ >= hold_cycles)
    {
        cycle = 0;
        
        for (led_index=0; led_index < cfg->led_number; led_index++)
        {
            // off
            buffer[led_index].r = 0;
            buffer[led_index].g = 0;
            buffer[led_index].b = 0;
            buffer[led_index].w = 0;
        }
    }    
}

/*!
 * \brief set the current led strip pattern
 *
 * \param[in]  pattern  index into pattern_table
 * 
 * \return nothing
 */
void set_led_pattern_local(int pattern) 
{
    if (pattern < 0)
    {
        pattern = cfg->led_pattern;
    }

    CLIP(pattern, 0, count_of(pattern_table));

    set_double_buf_integer(&local_pattern, pattern);

    app_web.led_current_pattern = pattern;
}

/*!
 * \brief set the current led strip pattern transition delay
 *
 * \param[in]  speed  milliseconds to delay before next step of led sequence
 * 
 * \return nothing
 */
void set_led_speed_local(int speed) 
{
    if (speed < 0)
    {
        speed = cfg->led_speed;
    }

    CLIP(speed, 0, 3000);

    set_double_buf_integer(&local_speed, speed);

    app_web.led_current_transition_delay = speed;
}

 /*!
 * \brief get pattern name
 *
 * \param[in]  pattern        index into pattern table
 * 
 * \return pinter to constant string containing pattern name
 */
 const char *get_pattern_name(int pattern)  
 {
    CLIP(pattern, 0, NUM_ROWS(pattern_table));

    return(pattern_table[pattern].name);
 } 