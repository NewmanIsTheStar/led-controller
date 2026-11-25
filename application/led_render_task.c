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
#include "application_config.h"
#include "watchdog.h"

// 256-entry 16-bit Gamma LUT (Gamma = 2.2, max = 65280)
static const uint16_t gamma16_lut[256] = 
{
        0,     0,     2,     4,     7,    11,    17,    24,    32,    42,    53,    65,    79,    94,   110,   128,
      147,   168,   191,   215,   241,   268,   297,   328,   360,   394,   429,   466,   505,   546,   588,   632,
      678,   725,   774,   825,   878,   933,   989,  1047,  1107,  1169,  1233,  1299,  1366,  1436,  1507,  1580,
     1655,  1732,  1811,  1892,  1975,  2060,  2147,  2236,  2327,  2420,  2514,  2611,  2710,  2811,  2914,  3019,
     3126,  3235,  3346,  3459,  3574,  3691,  3810,  3931,  4055,  4180,  4308,  4437,  4569,  4703,  4839,  4977,
     5117,  5259,  5404,  5550,  5699,  5850,  6003,  6158,  6315,  6475,  6636,  6800,  6966,  7134,  7305,  7477,
     7652,  7829,  8008,  8190,  8373,  8559,  8747,  8938,  9130,  9325,  9522,  9721,  9923, 10127, 10333, 10541,
    10751, 10964, 11179, 11396, 11616, 11838, 12062, 12288, 12517, 12748, 12981, 13216, 13454, 13694, 13936, 14181,
    14428, 14677, 14928, 15182, 15438, 15696, 15957, 16220, 16485, 16753, 17023, 17295, 17569, 17846, 18125, 18407,
    18691, 18977, 19265, 19556, 19849, 20145, 20443, 20743, 21046, 21351, 21658, 21968, 22280, 22594, 22911, 23230,
    23552, 23876, 24202, 24531, 24862, 25195, 25531, 25869, 26210, 26553, 26898, 27246, 27596, 27949, 28304, 28661,
    29021, 29383, 29748, 30115, 30484, 30856, 31230, 31607, 31986, 32368, 32752, 33138, 33527, 33919, 34312, 34709,
    35107, 35509, 35912, 36319, 36727, 37138, 37552, 37968, 38386, 38807, 39231, 39657, 40085, 40516, 40949, 41385,
    41823, 42264, 42707, 43153, 43601, 44052, 44505, 44961, 45419, 45880, 46343, 46809, 47277, 47748, 48221, 48697,
    49175, 49656, 50139, 50625, 51114, 51605, 52098, 52594, 53093, 53594, 54098, 54604, 55113, 55624, 56138, 56654,
    57173, 57695, 58219, 58746, 59275, 59807, 60341, 60878, 61418, 61960, 62505, 63052, 63602, 64154, 64709, 65280
};

// Triple-buffer slots: 1 active in render task, 1 owned by producer, 1 ready in mailbox
static rgbw_t frame_pool[3][MAX_LEDS];
static rgbw_t *producer_buf = frame_pool[0];
static rgbw_t *active_buf   = frame_pool[1];
static _Atomic(rgbw_t *) mailbox_buf = frame_pool[2];

// Thread-safe controls (packed white balance as 0xWWRRGGBB)
atomic_uint_fast8_t  g_master_brightness = 255;
static atomic_uint_fast32_t g_white_balance     = 0xFFFFFFFFU; // Default: 255, 255, 255, 255

static uint8_t  dither_err[MAX_LEDS][4];
static uint32_t grbw_dma_buf[MAX_LEDS];

static PIO  led_pio;
static uint led_sm;
static uint led_dma_chan;

// --- Public API for Producer / UI / Network Tasks ---

void led_set_master_brightness(uint8_t brightness) 
{
    atomic_store_explicit(&g_master_brightness, brightness, memory_order_relaxed);
}

void led_set_white_balance(uint8_t r_gain, uint8_t g_gain, uint8_t b_gain, uint8_t w_gain) 
{
    uint32_t packed = ((uint32_t)w_gain << 24) | ((uint32_t)r_gain << 16) |
                      ((uint32_t)g_gain << 8)  | (uint32_t)b_gain;
    atomic_store_explicit(&g_white_balance, packed, memory_order_relaxed);
}

// Get writable buffer for the producer task (single producer assumed)
rgbw_t *led_get_back_buffer(void) 
{
    return producer_buf;
}

// Publish completed frame and receive a recycled buffer to draw the next frame into
void led_submit_frame(void) 
{
    producer_buf = atomic_exchange_explicit(&mailbox_buf, producer_buf, memory_order_acq_rel);
}

// --- Internal Render & Hardware Driver ---

static void led_hw_init(void) 
{
    uint offset;
    bool ok = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &led_pio, &led_sm, &offset, WS2812_PIN, 1, true);
    hard_assert(ok);

    ws2812_program_init(led_pio, led_sm, offset, WS2812_PIN, 800000.0f, IS_RGBW);

    led_dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(led_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(led_pio, led_sm, true));

    dma_channel_configure(led_dma_chan, &c, &led_pio->txf[led_sm], grbw_dma_buf, cfg->led_number, false);  //NUM_LEDS
}

static void render_frame(void) 
{
    // Check if producer published a newer frame; if not, keep dithering current active_buf
    rgbw_t *latest = atomic_exchange_explicit(&mailbox_buf, NULL, memory_order_acq_rel);
    if (latest != NULL) 
    {
        // Return old active_buf to mailbox so producer always has a spare buffer
        rgbw_t *stale = atomic_exchange_explicit(&mailbox_buf, active_buf, memory_order_acq_rel);
        if (stale != NULL) 
        {
            // Producer submitted another frame mid-swap; keep the newest one
            active_buf = stale;
        } else 
        {
            active_buf = latest;
        }
    }

    uint8_t  raw_master   = atomic_load_explicit(&g_master_brightness, memory_order_relaxed);
    uint32_t master_scale = gamma16_lut[raw_master]; // 0..65280

    uint32_t wb_packed = atomic_load_explicit(&g_white_balance, memory_order_relaxed);
    uint32_t wb_gain[4] = 
    {
        (wb_packed >> 16) & 0xFFU, // R gain (0..255)
        (wb_packed >> 8)  & 0xFFU, // G gain (0..255)
        (wb_packed)       & 0xFFU, // B gain (0..255)
        (wb_packed >> 24) & 0xFFU  // W gain (0..255)
    };

    // Pre-combine master brightness and white-balance gain per channel (0..65280)
    // Using >> 8 keeps max scale at (65280 * 255) >> 8 = 65025, or / 255U for exact 65280
    uint32_t ch_scale[4];
    for (int c = 0; c < 4; c++) 
    {
        ch_scale[c] = (master_scale * wb_gain[c]) / 255U;
    }

    for (int i = 0; i < cfg->led_number; i++)  // NUM_LEDS
    {
        const uint8_t in_ch[4] = 
        {
            active_buf[i].r,
            active_buf[i].g,
            active_buf[i].b,
            active_buf[i].w
        };
        uint8_t out_c[4];

        for (int c = 0; c < 4; c++) 
        {
            uint8_t in_val = in_ch[c];
            if (in_val == 0 || ch_scale[c] == 0) 
            {
                dither_err[i][c] = 0;
                out_c[c] = 0;
                continue;
            }

            // Combine 16-bit gamma target with master brightness & white-balance gain
            uint32_t scaled_16 = ((uint32_t)gamma16_lut[in_val] * ch_scale[c]) / 65280U;

            // Accumulate fractional remainder for temporal dithering
            uint32_t acc = scaled_16 + dither_err[i][c];
            if (acc > 65280U) acc = 65280U;

            out_c[c] = (uint8_t)(acc >> 8);
            dither_err[i][c] = (uint8_t)(acc & 0xFFU);
        }

        // SK6812 RGBW wire order is G-R-B-W, shifted MSB-first
        grbw_dma_buf[i] = ((uint32_t)out_c[1] << 24) | // G
                          ((uint32_t)out_c[0] << 16) | // R
                          ((uint32_t)out_c[2] << 8)  | // B
                          ((uint32_t)out_c[3]);        // W
    }
}

void led_render_task(void *params) 
{
    int active_led_count;

    CLIP(cfg->led_number, 0, MAX_LEDS);

    // configure dma
    led_hw_init();

    // remember the active number of LEDS
    active_led_count = cfg->led_number;

    // 13 ms period (~76.9 Hz) leaves ~1.0 ms reset latch after the 12.0 ms RGBW transfer
    const TickType_t xPeriod = pdMS_TO_TICKS(13);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) 
    {
        dma_channel_wait_for_finish_blocking(led_dma_chan);
        render_frame();

        if (cfg->led_number == active_led_count)
        {
            // reset the read pointer and trigger transfer
            dma_channel_set_read_addr(led_dma_chan, grbw_dma_buf, true);
        }
        else
        {
            CLIP(cfg->led_number, 0, MAX_LEDS);

            // reset the read pointer without triggering the channel yet
            dma_channel_set_read_addr(led_dma_chan, grbw_dma_buf, false);

            // set the new transfer count (number of LEDs) and trigger the transfer
            dma_channel_set_trans_count(led_dma_chan, dma_encode_transfer_count(cfg->led_number), true);  
            
            // update the active led count
            active_led_count = cfg->led_number;          
        }

        watchdog_pulse((int *)params);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}