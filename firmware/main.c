/**
 * Single File Spectrum Analyzer
 * Hardware: nrf52832, ST7565 LCD or SH1106 (CH1116) OLED, Radio
 * Updated: Added DFU Bootloader entry
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_saadc.h"
#include "boards.h"

// --------------------------------------------------------------------------
// CONFIGURATION & PIN DEFINITIONS
// --------------------------------------------------------------------------

// Select Display Type (Uncomment if SH1106 is used instead of ST7565)
// #define OLED_TYPE_SH1106

// debugging only: show FPS indicator
#define SHOW_FPS_INDICATOR

// LCD Pins
#define PIN_LCD_SCL 26
#define PIN_LCD_MOSI 25
#define PIN_LCD_CS 27
#define PIN_LCD_DC 28
#define PIN_LCD_RST 29
#define PIN_LCD_BL 30

// Button & Input Pins
#define PIN_BTN_LEFT 5
#define PIN_BTN_MID 6
#define PIN_BTN_RIGHT 7
#define PIN_ADC_INPUT 2
#define PIN_CHRG_STAT 3

// Spectrum Analyzer Settings
#define SCAN_START_FREQ 0
#define SCAN_END_FREQ 87
#define BANDWIDTH (SCAN_END_FREQ - SCAN_START_FREQ + 1)

// Display Settings
#define DISP_W 128
#define DISP_H 64
#define SPECTRUM_H 32
#define WATERFALL_START 32

// DFU Magic Number (Standard Nordic SDK value)
#define BOOTLOADER_DFU_START 0xB1

// Hardware Offset Definition:
#ifdef OLED_TYPE_SH1106
#define LCD_START_COL 2
#else
#define LCD_START_COL 0
#endif

// --------------------------------------------------------------------------
// GLOBAL VARIABLES & FONTS
// --------------------------------------------------------------------------

static uint8_t m_frame_buffer[1024];
static uint8_t m_rssi_current[BANDWIDTH];
static uint8_t m_rssi_peak[BANDWIDTH];
static float m_rssi_floating[BANDWIDTH];
static uint8_t m_waterfall_data[DISP_W][4];

#ifdef SHOW_FPS_INDICATOR
static uint32_t m_frame_count = 0;
static uint32_t m_fps = 0;
static uint32_t m_last_time = 0;
#endif

typedef struct
{
    uint8_t level; // 0..8
    float voltage;
    bool is_charging;
    int16_t raw_adc;
} bat_status_t;

bat_status_t current_bat_status = {0};
const float lipo_voltage_map[] = {4.1, 3.98, 3.92, 3.87, 3.8, 3.7, 3.6, 3.5, 3.4};

typedef enum
{
    STATE_SCANNER,
    STATE_MENU,
    STATE_INFO,
    STATE_CONTRAST
} app_state_t;

app_state_t current_state = STATE_SCANNER;
int menu_selection = 0;
#ifdef OLED_TYPE_SH1106
#define MENU_ITEMS 4
#else
#define MENU_ITEMS 5
static uint8_t m_contrast_level = 32;
#endif

const char *menu_items[MENU_ITEMS] =
{
    "Back",
    "About",
    "Sleep",
    "DFU mode",
#ifndef OLED_TYPE_SH1106    
    "LCD Contrast"
#endif
};

int scanner_selection = 0;
#define SCANNER_MODES 3

// Font 5x7
const uint8_t font5x7[][5] = {
    {0, 0, 0, 0, 0}, {0, 0, 95, 0, 0}, {0, 7, 0, 7, 0}, {20, 127, 20, 127, 20}, {36, 42, 127, 42, 18}, {35, 19, 8, 100, 98}, {54, 73, 85, 34, 80}, {0, 5, 3, 0, 0}, {0, 28, 34, 65, 0}, {0, 65, 34, 28, 0}, {20, 8, 62, 8, 20}, {8, 8, 62, 8, 8}, {0, 80, 48, 0, 0}, {8, 8, 8, 8, 8}, {0, 96, 96, 0, 0}, {32, 16, 8, 4, 2}, {62, 81, 73, 69, 62}, {0, 66, 127, 64, 0}, {66, 97, 81, 73, 70}, {33, 65, 69, 75, 49}, {24, 20, 18, 127, 16}, {39, 69, 69, 69, 57}, {60, 74, 73, 73, 48}, {1, 1, 113, 9, 7}, {54, 73, 73, 73, 54}, {6, 73, 73, 41, 30}, {0, 54, 54, 0, 0}, {0, 86, 54, 0, 0}, {8, 20, 34, 65, 0}, {20, 20, 20, 20, 20}, {0, 65, 34, 20, 8}, {2, 1, 81, 9, 6}, {50, 73, 121, 65, 62}, {126, 17, 17, 17, 126}, {127, 73, 73, 73, 54}, {62, 65, 65, 65, 34}, {127, 65, 65, 65, 62}, {127, 73, 73, 73, 65}, {127, 9, 9, 9, 1}, {62, 65, 73, 73, 58}, {127, 8, 8, 8, 127}, {0, 65, 127, 65, 0}, {32, 64, 65, 63, 1}, {127, 8, 20, 34, 65}, {127, 64, 64, 64, 64}, {127, 2, 12, 2, 127}, {127, 4, 8, 16, 127}, {62, 65, 65, 65, 62}, {127, 9, 9, 9, 6}, {62, 65, 81, 33, 94}, {127, 9, 25, 41, 70}, {70, 73, 73, 73, 49}, {1, 1, 127, 1, 1}, {63, 64, 64, 64, 63}, {31, 32, 64, 32, 31}, {63, 64, 56, 64, 63}, {99, 20, 8, 20, 99}, {7, 8, 112, 8, 7}, {97, 81, 73, 69, 67}, {0, 127, 65, 65, 0}, {2, 4, 8, 16, 32}, {0, 65, 65, 127, 0}, {4, 2, 1, 2, 4}, {64, 64, 64, 64, 64}, {0, 1, 2, 4, 0}, {32, 84, 84, 84, 120}, {127, 72, 68, 68, 56}, {56, 68, 68, 68, 32}, {56, 68, 68, 72, 127}, {56, 84, 84, 84, 24}, {8, 126, 9, 1, 2}, {12, 82, 82, 82, 62}, {127, 8, 4, 4, 120}, {0, 68, 125, 64, 0}, {32, 64, 68, 61, 0}, {127, 16, 40, 68, 0}, {0, 65, 127, 64, 0}, {124, 4, 24, 4, 120}, {124, 8, 4, 4, 120}, {56, 68, 68, 68, 56}, {124, 20, 20, 20, 8}, {8, 20, 20, 24, 124}, {124, 8, 4, 4, 8}, {72, 84, 84, 84, 32}, {4, 63, 68, 64, 32}, {60, 64, 64, 32, 124}, {28, 32, 64, 32, 28}, {60, 64, 48, 64, 60}, {68, 40, 16, 40, 68}, {12, 80, 80, 80, 60}, {68, 100, 84, 76, 68}};

// --------------------------------------------------------------------------
// LOW LEVEL LCD DRIVER
// --------------------------------------------------------------------------

static void lcd_spi_byte(uint8_t data)
{
    for (int i = 7; i >= 0; i--)
    {
        if (data & (1 << i))
            nrf_gpio_pin_set(PIN_LCD_MOSI);
        else
            nrf_gpio_pin_clear(PIN_LCD_MOSI);

        nrf_gpio_pin_set(PIN_LCD_SCL);
        nrf_gpio_pin_clear(PIN_LCD_SCL);
    }
}

void lcd_write_cmd(uint8_t cmd)
{
    nrf_gpio_pin_clear(PIN_LCD_CS);
    nrf_gpio_pin_clear(PIN_LCD_DC);
    lcd_spi_byte(cmd);
    nrf_gpio_pin_set(PIN_LCD_CS);
}

void lcd_write_data_block(const uint8_t *data, int len)
{
    nrf_gpio_pin_clear(PIN_LCD_CS);
    nrf_gpio_pin_set(PIN_LCD_DC);
    for (int i = 0; i < len; i++)
    {
        lcd_spi_byte(data[i]);
    }
    nrf_gpio_pin_set(PIN_LCD_CS);
}

void lcd_init(void)
{
    nrf_gpio_cfg_output(PIN_LCD_SCL);
    nrf_gpio_cfg_output(PIN_LCD_MOSI);
    nrf_gpio_cfg_output(PIN_LCD_CS);
    nrf_gpio_cfg_output(PIN_LCD_DC);
    nrf_gpio_cfg_output(PIN_LCD_RST);
    nrf_gpio_cfg_output(PIN_LCD_BL);

    nrf_gpio_pin_set(PIN_LCD_CS);
    nrf_gpio_pin_set(PIN_LCD_BL);

    nrf_gpio_pin_clear(PIN_LCD_RST);
    nrf_delay_ms(100);
    nrf_gpio_pin_set(PIN_LCD_RST);
    nrf_delay_ms(100);

#ifdef OLED_TYPE_SH1106
    lcd_write_cmd(0xAE);
    lcd_write_cmd(0x00 | (LCD_START_COL & 0x0F));
    lcd_write_cmd(0x10 | (LCD_START_COL >> 4));
    lcd_write_cmd(0x40);
    lcd_write_cmd(0xB0);
    lcd_write_cmd(0x81);
    lcd_write_cmd(0xCF);
    lcd_write_cmd(0xA1);
    lcd_write_cmd(0xA6);
    lcd_write_cmd(0xA8);
    lcd_write_cmd(0x3F);
    lcd_write_cmd(0xAD);
    lcd_write_cmd(0x8B);
    lcd_write_cmd(0x33);
    lcd_write_cmd(0xC8);
    lcd_write_cmd(0xD3);
    lcd_write_cmd(0x00);
    lcd_write_cmd(0xD5);
    lcd_write_cmd(0x80);
    lcd_write_cmd(0xD9);
    lcd_write_cmd(0x1F);
    lcd_write_cmd(0xDA);
    lcd_write_cmd(0x12);
    lcd_write_cmd(0xDB);
    lcd_write_cmd(0x40);
    lcd_write_cmd(0xAF);
#else
    lcd_write_cmd(0xE2); // 1 1 1 0 1 1 1 0, Reset
    nrf_delay_ms(10);    // sleep 10 ms
    lcd_write_cmd(0xA2); // 1 0 1 0 0 0 1 0, LCD Bias Set: 1/9 bias ratio
    lcd_write_cmd(0xA0); // 1 0 1 0 0 0 0 0, ADC Select (Segment Driver Direction Select): normal
    lcd_write_cmd(0xC8); // 1 1 0 0 1 0 0 0, Common Output Mode Select: reverse
    lcd_write_cmd(0x23); // 0 0 1 0 0 0 1 1, Voltage Regulator Resistor Ratio Set: 3 (0..7)
    lcd_write_cmd(0x81); // 1 0 0 0 0 0 0 1, Electronic Volume Mode Set (Contrast setting)
    lcd_write_cmd(m_contrast_level); // Electronic Volume Register Set (Contrast value): (0..63)
    lcd_write_cmd(0x2F); // 0 0 1 0 1 1 1 1, Power Control Set: all on
    lcd_write_cmd(0xB0); // 1 0 1 1 0 0 0 0, Set Page Address: page 0
    lcd_write_cmd(0xA6); // 1 0 1 0 0 1 1 0, Display Normal / Inverse: normal
    lcd_write_cmd(0xAF); // 1 0 1 0 1 1 1 1, Display ON
#endif
}

void lcd_uninit(void)
{
#ifdef OLED_TYPE_SH1106
    lcd_write_cmd(0xAE);
#endif
    nrf_gpio_pin_clear(PIN_LCD_BL);
    nrf_delay_ms(100);
    nrf_gpio_cfg_default(PIN_LCD_SCL);
    nrf_gpio_cfg_default(PIN_LCD_MOSI);
    nrf_gpio_cfg_default(PIN_LCD_CS);
    nrf_gpio_cfg_default(PIN_LCD_DC);
    nrf_gpio_cfg_default(PIN_LCD_RST);
    nrf_gpio_cfg_default(PIN_LCD_BL);
}

void lcd_flush(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        lcd_write_cmd(0xB0 + page);
        lcd_write_cmd(0x00 | (LCD_START_COL & 0x0F));
        lcd_write_cmd(0x10 | ((LCD_START_COL >> 4) & 0x0F));
        lcd_write_data_block(&m_frame_buffer[page * DISP_W], DISP_W);
    }
}

#define MAX_CONTRAST 63
#ifndef OLED_TYPE_SH1106
void lcd_set_contrast(uint8_t contrast)
{
    lcd_write_cmd(0x81);
    lcd_write_cmd(contrast & 0x3F);
}
#endif

// --------------------------------------------------------------------------
// SETTINGS
// --------------------------------------------------------------------------
// Settings are stored in UICR CUSTOMER register 0 (0x10001080 .. 0x100010FC)

typedef struct {
    uint8_t lcd_contrast : 7;
} settings_data_t; // please keep this as small as possible.

static settings_data_t m_settings_data;

static const settings_data_t def_settings_data = {
    .lcd_contrast = 32
};

int32_t settings_init();
int32_t settings_save();
int32_t settings_reset();
settings_data_t *settings_get_data();

static void validate_settings() {
    settings_data_t *data = settings_get_data();
    if (data->lcd_contrast > MAX_CONTRAST) {
        data->lcd_contrast = MAX_CONTRAST;
    }
}

int32_t settings_init() {
    // Try to read from UICR customer registers
    // UICR CUSTOMER registers start at address 0x10001080
    uint32_t *uicr_settings = (uint32_t *)0x10001080;
    
    // Check if UICR contains valid data (not 0xFFFFFFFF which is erased state)
    if (*uicr_settings != 0xFFFFFFFF) {
        memcpy(&m_settings_data, uicr_settings, sizeof(settings_data_t));
        validate_settings();
    } else {
        // No valid data in UICR, use defaults
        memcpy(&m_settings_data, &def_settings_data, sizeof(settings_data_t));
    }
    return 0;
}

int32_t settings_save() {
    // TODO: if we write more than 181 times, we need to do an erase cycle with ERASEUICR.
    
    // Enable flash writes
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    
    // Write settings to UICR CUSTOMER[0]
    uint32_t *uicr_addr = (uint32_t *)0x10001080;
    uint32_t data_to_write;
    memcpy(&data_to_write, &m_settings_data, sizeof(uint32_t));
    *uicr_addr = data_to_write;
    
    // Wait for write to complete
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    
    // Disable flash writes
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {}
    
    return 0;
}

settings_data_t *settings_get_data() { return &m_settings_data; }

int32_t settings_reset() {
    memcpy(&m_settings_data, &def_settings_data, sizeof(settings_data_t));
}


// --------------------------------------------------------------------------
// GRAPHIC PRIMITIVES
// --------------------------------------------------------------------------

void draw_pixel(int x, int y, bool on)
{
    if (x >= 0 && x < DISP_W && y >= 0 && y < DISP_H)
    {
        if (on)
            m_frame_buffer[x + (y / 8) * DISP_W] |= (1 << (y % 8));
        else
            m_frame_buffer[x + (y / 8) * DISP_W] &= ~(1 << (y % 8));
    }
}

void draw_vline(int x, int y1, int y2)
{
    if (y1 > y2)
    {
        int t = y1;
        y1 = y2;
        y2 = t;
    }
    for (int y = y1; y <= y2; y++)
        draw_pixel(x, y, true);
}

void draw_box(int x, int y, int w, int h, bool fill, bool color)
{
    for (int i = x; i < x + w; i++)
    {
        for (int j = y; j < y + h; j++)
        {
            if (i == x || i == x + w - 1 || j == y || j == y + h - 1 || fill)
            {
                draw_pixel(i, j, color);
            }
        }
    }
}

void draw_filled_bar(int x, int y, int w, int h, int fill_level, int fill_max)
{
    draw_box(x, y, w, h, false, true);
    
    // inside fill
    if (fill_level > 0)
    {
        if (fill_level > fill_max)
            fill_level = fill_max;
        int wf = (fill_level * (w - 2)) / fill_max;
        draw_box(x + 1, y + 1, wf, h - 2, true, true);
    }
}

void draw_char_buf(int x, int y,  char c)
{
    if (c < 32 || c > 122)
        c = 32;
    c -= 32;
    for (int i = 0; i < 5; i++)
    {
        uint8_t line = font5x7[(int)c][i];
        for (int j = 0; j < 8; j++)
        {
            if (line & (1 << j))
                draw_pixel(x + i, y + j, true);
        }
    }
}

void draw_text_buf(int x, int y, const char *str)
{
    while (*str)
    {
        draw_char_buf(x, y, *str++);
        x += 6;
    }
}

// --------------------------------------------------------------------------
// HARDWARE / PERIPHERALS
// --------------------------------------------------------------------------
// forward declaration
uint32_t get_time_ms(void);

/** 
 * @brief Detect if the mid button is pressed.
 * If `raw` is False, it will only return True on the transition from not pressed to pressed (button press event).
 * It will use debouncing logic in that case.
 * 
 * @param raw set to True to get the raw button state, False to get debounced leading edge only
 * @return True if button was pressed, False otherwise
 **/
bool btn_mid(bool raw) 
{
    static bool previous_state = false;

    bool raw_state = (nrf_gpio_pin_read(PIN_BTN_MID) == 0); // active low
    if (raw)
    {
        previous_state = raw_state; // keep previous state for next call, that may need debounce
        return raw_state;
    }

    if (raw_state != previous_state)
    {        
        nrf_delay_ms(10); // Debounce delay
        raw_state = (nrf_gpio_pin_read(PIN_BTN_MID) == 0); // re-read after debounce delay
        if (raw_state != previous_state)
        {
            previous_state = raw_state;
            if (raw_state)
            {
                return true; // Button was just pressed
            }
        }
    }
    return false; // Button not pressed or still pressed
}

/**  
 * @brief Detect if the left button is pressed.
 * @param raw set to True to get the raw button state, False to get auto repeating (3 per sec), and debounced leading edge only
 * @return True if button was pressed, False otherwise
 **/
bool btn_left(bool raw) 
{ 
    static bool previous_state = false;
    static uint32_t last_press = 0;

    bool raw_state = (nrf_gpio_pin_read(PIN_BTN_LEFT) == 0); // active low
    if (raw)
    {
        previous_state = raw_state; // keep previous state for next call, that may need debounce
        return raw_state;
    }
    if (raw_state != previous_state)
    {        
        nrf_delay_ms(10); // Debounce delay
        raw_state = (nrf_gpio_pin_read(PIN_BTN_LEFT) == 0); // re-read after debounce delay
        if (raw_state != previous_state)
        {
            previous_state = raw_state;
            if (raw_state)
            {
                last_press = get_time_ms();
                return true; // Button was just pressed
            }
        }
    }
    else if (raw_state) // still pressed
    {
        if (last_press == 0)
            last_press = get_time_ms(); // should not happen, but just in case        
        uint32_t now = get_time_ms();
        if ((now - last_press) >= 333) // 333ms delay for auto repeat
        {
            last_press = now;
            return true; // Auto repeat press
        }
    }
    return false; // Button not pressed or still pressed (but not long enough to provoke auto repeat)
}

/**  
 * @brief Detect if the right button is pressed.
 * @param raw set to True to get the raw button state, False to get auto repeating (3 per sec), and debounced leading edge only
 * @return True if button was pressed, False otherwise
 **/
bool btn_right(bool raw) 
{ 
    static bool previous_state = false;
    static uint32_t last_press = 0;

    bool raw_state = (nrf_gpio_pin_read(PIN_BTN_RIGHT) == 0); // active low
    if (raw)
    {
        previous_state = raw_state; // keep previous state for next call, that may need debounce
        return raw_state;
    }
    if (raw_state != previous_state)
    {        
        nrf_delay_ms(10); // Debounce delay
        raw_state = (nrf_gpio_pin_read(PIN_BTN_RIGHT) == 0); // re-read after debounce delay
        if (raw_state != previous_state)
        {
            previous_state = raw_state;
            if (raw_state)
            {
                last_press = get_time_ms();
                return true; // Button was just pressed
            }
        }
    }
    else if (raw_state) // still pressed
    {
        if (last_press == 0)
            last_press = get_time_ms(); // should not happen, but just in case
        uint32_t now = get_time_ms();
        if ((now - last_press) >= 333) // 333ms delay for auto repeat
        {
            last_press = now;
            return true; // Auto repeat press
        }
    }
    return false; // Button not pressed or still pressed (but not long enough to provoke auto repeat)
}

void buttons_init(void)
{
    nrf_gpio_cfg_input(PIN_BTN_LEFT, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(PIN_BTN_MID, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(PIN_BTN_RIGHT, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(PIN_CHRG_STAT, NRF_GPIO_PIN_PULLUP);
    // and init their local variables
    btn_mid(true); // initialize previous state
    btn_left(true); // initialize previous state
    btn_right(true); // initialize previous state
}

void enter_deep_sleep(void)
{
    memset(m_frame_buffer, 0, 1024);
    lcd_flush();
    lcd_uninit();

    NRF_RADIO->TASKS_DISABLE = 1;
    while (!NRF_RADIO->EVENTS_DISABLED)
        ;

    nrf_gpio_cfg_sense_input(PIN_BTN_MID, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

    NRF_POWER->SYSTEMOFF = 1;
    while (1)
        ;
}

void timer_init(void)
{
    NRF_TIMER0->TASKS_STOP = 1;
    NRF_TIMER0->MODE = TIMER_MODE_MODE_Timer;
    NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER0->PRESCALER = 4; // 1MHz resolution
    NRF_TIMER0->TASKS_CLEAR = 1;
    NRF_TIMER0->TASKS_START = 1;
}

uint32_t get_time_ms(void)
{
    NRF_TIMER0->TASKS_CAPTURE[0] = 1;
    return (NRF_TIMER0->CC[0]) / 1000;
}

void saadc_init_simple(void)
{
    nrf_gpio_cfg_input(PIN_ADC_INPUT, NRF_GPIO_PIN_NOPULL);
    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit;
    NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass;
    NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos) |
                              (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
                              (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
                              (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
                              (SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos);
    NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;
    NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC << SAADC_CH_PSELN_PSELN_Pos;
    NRF_SAADC->ENABLE = 1;
}

void saadc_calibrate(void)
{
    saadc_init_simple();
    NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
    while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0)
        ;
    NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
    NRF_SAADC->ENABLE = 0;
}

void bat_measure_update(void)
{
    volatile int16_t adc_val = 0;
    saadc_init_simple();
    NRF_SAADC->RESULT.PTR = (uint32_t)&adc_val;
    NRF_SAADC->RESULT.MAXCNT = 1;
    NRF_SAADC->EVENTS_STARTED = 0;
    NRF_SAADC->EVENTS_END = 0;
    NRF_SAADC->EVENTS_STOPPED = 0;
    NRF_SAADC->TASKS_START = 1;
    while (NRF_SAADC->EVENTS_STARTED == 0)
        ;
    NRF_SAADC->EVENTS_STARTED = 0;
    NRF_SAADC->TASKS_SAMPLE = 1;
    while (NRF_SAADC->EVENTS_END == 0)
        ;
    NRF_SAADC->EVENTS_END = 0;
    NRF_SAADC->TASKS_STOP = 1;
    while (NRF_SAADC->EVENTS_STOPPED == 0)
        ;
    NRF_SAADC->EVENTS_STOPPED = 0;
    NRF_SAADC->ENABLE = 0;

    if (adc_val < 0)
        adc_val = 0;
    current_bat_status.raw_adc = adc_val;

    float voltage = (3.6f / 1024.0f) * (float)adc_val * 1.451f;
    uint8_t level = 0;
    for (uint32_t i = 0; i < 9; i++)
    {
        if (voltage >= lipo_voltage_map[i])
        {
            level = 9 - i - 1;
            break;
        }
    }
    current_bat_status.voltage = voltage;
    current_bat_status.level = level;
    current_bat_status.is_charging = (nrf_gpio_pin_read(PIN_CHRG_STAT) == 0);
}

void radio_init_scanner(void)
{
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0)
        ;
    NRF_RADIO->POWER = 1;
    NRF_RADIO->MODE = (RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos);
}

void scan_band(void)
{
    if ((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) !=
        (CLOCK_HFCLKSTAT_SRC_Xtal << CLOCK_HFCLKSTAT_SRC_Pos | CLOCK_HFCLKSTAT_STATE_Running << CLOCK_HFCLKSTAT_STATE_Pos))
    {
        NRF_CLOCK->TASKS_HFCLKSTART = 1;
        while ((NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk) == 0)
            ;
    }
    for (uint32_t freq = SCAN_START_FREQ; freq <= SCAN_END_FREQ; freq++)
    {
        NRF_RADIO->FREQUENCY = freq;
        NRF_RADIO->DATAWHITEIV = 0x40;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_RXEN = 1;
        while (NRF_RADIO->EVENTS_READY == 0)
            ;
        NRF_RADIO->EVENTS_RSSIEND = 0;
        NRF_RADIO->TASKS_RSSISTART = 1;
        while (NRF_RADIO->EVENTS_RSSIEND == 0)
            ;
        uint8_t val = NRF_RADIO->RSSISAMPLE;
        if ((freq - SCAN_START_FREQ) < BANDWIDTH)
            m_rssi_current[freq - SCAN_START_FREQ] = val;
        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0)
            ;
    }
}

// --------------------------------------------------------------------------
// APPLICATION LOGIC
// --------------------------------------------------------------------------

#define WATERFALL_MINIMUM_VALUE 88

void process_waterfall(void)
{
    for (int x = 0; x < DISP_W; x++)
    {
        int freq_idx = (x * BANDWIDTH) / DISP_W;
        uint8_t val = m_rssi_current[freq_idx];
        bool pixel_on = (val < WATERFALL_MINIMUM_VALUE); // this is a threshold for turning the pixel on
        uint32_t col = (m_waterfall_data[x][3] << 24) | (m_waterfall_data[x][2] << 16) | (m_waterfall_data[x][1] << 8) | m_waterfall_data[x][0];
        col <<= 1;
        if (pixel_on)
            col |= 1;
        m_waterfall_data[x][0] = (col & 0xFF);
        m_waterfall_data[x][1] = (col >> 8) & 0xFF;
        m_waterfall_data[x][2] = (col >> 16) & 0xFF;
        m_waterfall_data[x][3] = (col >> 24) & 0xFF;
    }
}

void show_boot_screen(void)
{
    memset(m_frame_buffer, 0, 1024);

    draw_box(2, 2, 124, 60, false, true);
    draw_box(4, 4, 120, 56, false, true);

    draw_text_buf(46, 15, "2.4GHz");
    draw_text_buf(40, 25, "SPECTRUM");
    draw_text_buf(40, 35, "ANALYZER");
    draw_text_buf(44, 45, "ATC1441");

    lcd_flush();
    nrf_delay_ms(1500);
}

void check_power_on_sequence(void)
{
	// Sometimes not working so only activate in development or so
    /*uint32_t reset_reason = NRF_POWER->RESETREAS;
    NRF_POWER->RESETREAS = 0xFFFFFFFF;

    // If not a Power-Off Reset, start normally
    if ((reset_reason & POWER_RESETREAS_OFF_Msk) == 0)
    {
        return;
    }*/

    // If from Deep Sleep: Check if button is held
    for (int i = 0; i < 20; i++)
    {
        if (!btn_mid(true)) // this is the only use of raw button read: need to see if it is still pressed.
        {
            memset(m_frame_buffer, 0, 1024);
            lcd_flush();
            enter_deep_sleep();
        }

        memset(m_frame_buffer, 0, 1024);
        draw_text_buf(25, 25, "HOLD TO START");

        draw_box(14, 38, 100, 8, false, true);
        int fill = (i * 96) / 19;
        draw_box(16, 40, fill, 4, true, true);

        lcd_flush();
        nrf_delay_ms(50);
    }
}

void render_scanner(void)
{
    memset(m_frame_buffer, 0, 1024);

    for (int x = 0; x < DISP_W; x++)
    {
        int freq_idx = (x * BANDWIDTH) / DISP_W;
        if (freq_idx >= BANDWIDTH)
            freq_idx = BANDWIDTH - 1;
        uint8_t val = m_rssi_current[freq_idx];
        int height = 0;
        if (val < 95)
            height = 95 - val;
        if (height > SPECTRUM_H)
            height = SPECTRUM_H;
        if (height < 0)
            height = 0;

        // Peak Hold Logic
        if (height >= m_rssi_peak[freq_idx])
            m_rssi_peak[freq_idx] = height;
        else if (m_rssi_peak[freq_idx] > 0)
            m_rssi_peak[freq_idx]--;

        int bar_h = m_rssi_peak[freq_idx];
        if (bar_h > 0)
            draw_vline(x, SPECTRUM_H - bar_h, SPECTRUM_H - 1);

        // Floating Dot
        float fh = (float)height;
        if (fh >= m_rssi_floating[freq_idx])
            m_rssi_floating[freq_idx] = fh;
        else
        {
            m_rssi_floating[freq_idx] -= 0.5f;
            if (m_rssi_floating[freq_idx] < 0)
                m_rssi_floating[freq_idx] = 0;
        }

        int dot_y = SPECTRUM_H - (int)m_rssi_floating[freq_idx] - 2;
        if (dot_y >= 0 && dot_y < SPECTRUM_H)
            draw_pixel(x, dot_y, true);
    }

    process_waterfall();

    // Draw Waterfall (Bottom half)
    int start_page = WATERFALL_START / 8;
    for (int x = 0; x < DISP_W; x++)
    {
        for (int p = 0; p < 4; p++)
        {
            if (start_page + p < 8)
                m_frame_buffer[x + (start_page + p) * DISP_W] = m_waterfall_data[x][p];
        }
    }

    // Info Text
    char buf[20];
#ifdef SHOW_FPS_INDICATOR
    sprintf(buf, "%luhz", m_fps);
    draw_text_buf(0, 0, buf);
#else
    // Debug only
    sprintf(buf, "%d", scanner_selection);
    draw_text_buf(0, 0, buf);
#endif    
    sprintf(buf, "%d", 2400 + SCAN_START_FREQ);
    draw_text_buf(0, 56, buf);
    sprintf(buf, "%d", 2400 + SCAN_END_FREQ);
    draw_text_buf(100, 56, buf);

    // Battery Update (rarely)
    static int batt_ctr = 0;
    if (batt_ctr++ > 60)
    {
        bat_measure_update();
        batt_ctr = 0;
    }

    if (current_bat_status.is_charging)
    {
        draw_text_buf(100, 0, "CHRG");
    }
    else
    {
        int pct = (current_bat_status.level * 100) / 8;
        sprintf(buf, "%d%%", pct);
        draw_text_buf(100, 0, buf);
    }

    lcd_flush();
}

void render_info(void)
{
    memset(m_frame_buffer, 0, 1024);

    // Border
    draw_box(0, 0, 128, 64, false, true);

    draw_text_buf(4, 4, "Made by ATC1441");

    draw_text_buf(4, 13, "github.com");
    draw_text_buf(4, 21, "/atc1441");
    draw_text_buf(4, 29, "/pixlAnalyzer");

    draw_text_buf(4, 44, "Credit Codebase:");
    draw_text_buf(4, 52, "solosky/pixl.js");

    lcd_flush();
}

#ifndef OLED_TYPE_SH1106
void render_contrast_setting(void)
{
    memset(m_frame_buffer, 0, 1024);

    char buf[30];
    sprintf(buf, "Contrast: %d", m_contrast_level);
    int x = (DISP_W - (strlen(buf) * 6)) / 2; // center text
    draw_text_buf(x, 20, buf);

    x = (DISP_W - (MAX_CONTRAST + 2)) / 2;
    draw_filled_bar(x, 30, MAX_CONTRAST + 2, 8, m_contrast_level, MAX_CONTRAST);

    lcd_flush();
}
#endif

#define MENU_LEFT_X 25

void render_menu_item(int line)
{
    if (line < 0) 
    {
        // this is the header
        draw_text_buf(52, 3, "MENU");
        return;
    }

    if (line > MENU_ITEMS - 1)
        return; // out of range
    
    int x = MENU_LEFT_X;
    int y = (line * 8) + 21;
    if (line == menu_selection)
    {
        draw_text_buf(x - 10, y, ">");
    }
    const char *text = menu_items[line];
    draw_text_buf(x, y, text);
}

void render_menu(void)
{
    bat_measure_update();
    memset(m_frame_buffer, 0, 1024);

    draw_box(0, 0, 128, 64, false, true);

    render_menu_item(-1); // header

    // Battery Status
    char buf[30];
    int v_int = (int)current_bat_status.voltage;
    int v_dec = (int)((current_bat_status.voltage - v_int) * 100);
    int y = 11;
    sprintf(buf, "Batt: %d.%02dV", v_int, v_dec);
    draw_text_buf(MENU_LEFT_X, y, buf);

    int x = MENU_LEFT_X + ((strlen(buf) + 1) * 6); // N chars width + 2 chars space
    int h = 7;
    // the battery
    int wm = (2 * 8) + 2; // 2 pixels per level + 2 pixels border
    draw_filled_bar(x, y, wm, h, current_bat_status.level, 8);
    // tip of the battery
    draw_box(x + wm, y+2, 2, h-4, false, true);

    // Menu Items
    for (int line = 0; line < MENU_ITEMS; line++)
    {
        render_menu_item(line);
    }

    lcd_flush();
}

// --------------------------------------------------------------------------
// MAIN
// --------------------------------------------------------------------------

int main(void)
{
    // Init Hardware
    settings_init();
#ifndef OLED_TYPE_SH1106
    m_contrast_level = settings_get_data()->lcd_contrast;
#endif
    timer_init();
    buttons_init();
    lcd_init();

    // Clear Screen
    memset(m_frame_buffer, 0, 1024);
    lcd_flush();

    // Boot Sequence
    check_power_on_sequence();
    show_boot_screen();

    // Calibration & Init Radio
    saadc_calibrate();
    bat_measure_update();
    radio_init_scanner();

    // Clear Data
    memset(m_rssi_peak, 0, sizeof(m_rssi_peak));
    memset(m_rssi_floating, 0, sizeof(m_rssi_floating));
    memset(m_waterfall_data, 0, sizeof(m_waterfall_data));
#ifdef SHOW_FPS_INDICATOR
    m_last_time = get_time_ms();
#endif

    while (1)
    {
        if (current_state == STATE_SCANNER)
        {
            scan_band();
            render_scanner();

            // Interaction
            if (btn_mid(false))
            {
                current_state = STATE_MENU;
                menu_selection = 0; // Reset selection on entry
            }
            if (btn_left(false)) scanner_selection--;
            if (btn_right(false)) scanner_selection++;
            // wrap around scanner mode selections
            if (scanner_selection < 0)
                scanner_selection = SCANNER_MODES - 1;
            if (scanner_selection > SCANNER_MODES - 1)
                scanner_selection = 0;

#ifdef SHOW_FPS_INDICATOR
            // FPS Counter
            m_frame_count++;
            uint32_t now = get_time_ms();
            if (now - m_last_time >= 1000)
            {
                m_fps = m_frame_count;
                m_frame_count = 0;
                m_last_time = now;
            }
#endif            
        }
        else if (current_state == STATE_MENU)
        {
            render_menu();

            if (btn_left(false)) menu_selection--;
            if (btn_right(false)) menu_selection++;
            // wrap around menu selection
            if (menu_selection < 0)
                menu_selection = MENU_ITEMS - 1;
            if (menu_selection > MENU_ITEMS - 1)
                menu_selection = 0;
        
            if (btn_mid(false))
            {
                if (menu_selection == 0)
                {
                    current_state = STATE_SCANNER;
                }
                else if (menu_selection == 1)
                {
                    current_state = STATE_INFO;
                }
                else if (menu_selection == 2)
                {
                    memset(m_frame_buffer, 0, 1024);
                    draw_text_buf(40, 30, "GOODBYE");
                    lcd_flush();
                    nrf_delay_ms(800);
                    enter_deep_sleep();
                }
                else if (menu_selection == 3)
                {
                    // DFU ENTRY
                    memset(m_frame_buffer, 0, 1024);
                    draw_text_buf(35, 30, "BOOTLOADER");
                    lcd_flush();
                    nrf_delay_ms(500);

                    // Signal to bootloader to enter DFU mode
                    NRF_POWER->GPREGRET = BOOTLOADER_DFU_START;
                    NVIC_SystemReset();
                }
#ifndef OLED_TYPE_SH1106
                else if (menu_selection == 4)
                {
                    current_state = STATE_CONTRAST;
                }
#endif                
            }
            nrf_delay_ms(150);
        }
        else if (current_state == STATE_INFO)
        {
            render_info();
            // Press any key to go back
            if (btn_mid(false) || btn_left(false) || btn_right(false))
            {
                current_state = STATE_MENU;
                nrf_delay_ms(150);
            }
        }
#ifndef OLED_TYPE_SH1106        
        else if (current_state == STATE_CONTRAST)
        {
            render_contrast_setting();

            if (btn_left(false))
            {
                if (m_contrast_level > 0)
                    m_contrast_level--;
                lcd_set_contrast(m_contrast_level);
            }
            if (btn_right(false))
            {
                if (m_contrast_level < MAX_CONTRAST)
                    m_contrast_level++;
                lcd_set_contrast(m_contrast_level);
            }
            if (btn_mid(false))
            {
                current_state = STATE_MENU;
                if (m_contrast_level != settings_get_data()->lcd_contrast)
                {
                    // save new contrast setting
                    settings_get_data()->lcd_contrast = m_contrast_level;
                    settings_save();                    
                }
                nrf_delay_ms(150);
            }
        }
#endif
    }
}