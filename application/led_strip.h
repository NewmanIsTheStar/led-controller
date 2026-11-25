
/**
 * Copyright (c) 2024 NewmanIsTheStar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef WS2812_H
#define WS2812_H

#define MAX_LEDS     300
#define WS2812_PIN   cfg->led_pin
#define IS_RGBW      cfg->led_rgbw

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t w;
} rgbw_t;

void led_strip_task(__unused void *params);
int get_pattern(void);
int get_speed(void);
void set_led_pattern_local(int pattern); 
void set_led_speed_local(int speed);
const char *get_pattern_name(int pattern);

void led_animation_task(void *params);
void led_render_task(void *params);

void led_set_master_brightness(uint8_t brightness);
void led_set_white_balance(uint8_t r_gain, uint8_t g_gain, uint8_t b_gain, uint8_t w_gain); 
rgbw_t *led_get_back_buffer(void); 
void led_submit_frame(void);

#endif