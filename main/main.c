#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/i2c.h"

// OLED Display
#define SSD1306_I2C_ADDRESS 0x3C
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 32
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)

// Button
#define BUTTON_PIN 0

// Potentiometer
#define POT_CHANNEL ADC1_CHANNEL_0

// I2C
#define I2C_MASTER_SCL_IO 5
#define I2C_MASTER_SDA_IO 4
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 400000

// Motor pins
#define IN1_PIN 14
#define IN2_PIN 12
#define IN3_PIN 13
#define IN4_PIN 15

#define STEP_DELAY_MS 10
#define STEPS_PER_REVOLUTION 4096  // For 28BYJ-48 stepper motor

// Timer states
#define STATE_SET_MINUTES 0
#define STATE_SET_SECONDS 1
#define STATE_RUNNING 2

//SSD1306
#define SSD1306_DISPLAYOFF            0xAE
#define SSD1306_DISPLAYON             0xAF
#define SSD1306_SETDISPLAYCLOCKDIV    0xD5
#define SSD1306_SETMULTIPLEX          0xA8
#define SSD1306_SETDISPLAYOFFSET      0xD3
#define SSD1306_SETSTARTLINE          0x40
#define SSD1306_CHARGEPUMP            0x8D
#define SSD1306_MEMORYMODE            0x20
#define SSD1306_SEGREMAP              0xA1
#define SSD1306_COMSCANDEC            0xC8
#define SSD1306_SETCOMPINS            0xDA
#define SSD1306_SETCONTRAST           0x81
#define SSD1306_SETPRECHARGE          0xD9
#define SSD1306_SETVCOMDETECT         0xDB
#define SSD1306_DISPLAYALLON_RESUME   0xA4
#define SSD1306_NORMALDISPLAY         0xA6

// Font
const uint8_t font5x7_basic[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 0x20 ' ' (space)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 0x21 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 0x22 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 0x23 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 0x24 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 0x25 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 0x26 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 0x27 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 0x28 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 0x29 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // 0x2A '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 0x2B '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 0x2C ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 0x2D '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 0x2E '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 0x2F '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0x30 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 0x31 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 0x32 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 0x33 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 0x34 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 0x35 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 0x36 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 0x37 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 0x38 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 0x39 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 0x3A ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 0x3B ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 0x3C '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 0x3D '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 0x3E '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 0x3F '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 0x40 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 0x41 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 0x42 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 0x43 'C'
    {0x7F, 0x41, 0x41, 0x41, 0x3E}, // 0x44 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 0x45 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 0x46 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 0x47 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 0x48 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 0x49 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 0x4A 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 0x4B 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 0x4C 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 0x4D 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 0x4E 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 0x4F 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 0x50 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 0x51 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 0x52 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 0x53 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 0x54 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 0x55 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 0x56 'V'
    {0x3F, 0x40, 0x3C, 0x40, 0x3F}, // 0x57 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 0x58 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 0x59 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 0x5A 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 0x5B '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 0x5C '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 0x5D ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 0x5E '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 0x5F '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 0x60 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 0x61 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 0x62 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 0x63 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 0x64 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 0x65 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 0x66 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 0x67 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 0x68 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 0x69 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 0x6A 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 0x6B 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 0x6C 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 0x6D 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 0x6E 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 0x6F 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 0x70 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 0x71 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 0x72 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 0x73 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 0x74 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 0x75 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 0x76 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 0x77 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 0x78 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 0x79 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 0x7A 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 0x7B '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 0x7C '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 0x7D '}'
    {0x02, 0x01, 0x02, 0x04, 0x02}, // 0x7E '~'
};


static int timer_state = STATE_SET_MINUTES;
static int timer_minutes = 0;
static int timer_seconds = 0;
static int timer_running = 0;
static int motor_extended = 0;

// Full step sequence for 28BYJ-48
const int step_sequence[4][4] = {
    {1, 0, 0, 1},
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1}
};

// Function prototypes
void init_i2c();
void init_button();
void init_adc();
void init_motor();
esp_err_t ssd1306_init();
esp_err_t ssd1306_write_command(uint8_t command);
esp_err_t ssd1306_write_data(uint8_t* data, size_t len);
esp_err_t ssd1306_clear_display();
void ssd1306_write_string(const char *str, uint8_t page, uint8_t column);
void update_display();
void timer_task(void *pvParameters);
void motor_task(void *pvParameters);
void set_motor_pins(int in1, int in2, int in3, int in4);
void move_motor(int steps);


void timer_task(void *pvParameters)
{
    int last_button_state = 1;
    int last_pot_value = 0;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        // Read button state
        int button_state = gpio_get_level(BUTTON_PIN);

        // Read potentiometer value
        uint16_t pot_value ;
        adc_read(&pot_value);

        // Button press handling
        if (button_state && !last_button_state) {
            if (timer_state == STATE_SET_MINUTES) {
                timer_state = STATE_SET_SECONDS;
            } else if (timer_state == STATE_SET_SECONDS) {
                printf("Set seconds & Start\n");
                timer_state = STATE_RUNNING;
                timer_running = 1;
            } else {
                timer_running = !timer_running;
            }
            update_display();
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Wait 0-.1 second
        }

        // Potentiometer handling
        if (abs(pot_value - last_pot_value) > 50) {  // Threshold to avoid jitter
            if (timer_state == STATE_SET_MINUTES) {
                timer_minutes = (pot_value * 60) / 4095;  // 0-60 minutes
            } else if (timer_state == STATE_SET_SECONDS) {
                timer_seconds = (pot_value * 60) / 1024;  // 0-60 seconds
            }
            last_pot_value = pot_value;
            update_display();
        }

        // Timer countdown
        if (timer_running) {
            if (timer_seconds > 0) {
                timer_seconds--;
            } else if (timer_minutes > 0) {
                timer_minutes--;
                timer_seconds = 59;
            } else {
                timer_running = 0;
                timer_state = STATE_SET_MINUTES;  // Reset to set minutes
            }
            update_display();
        }

        

        last_button_state = button_state;

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));  // Update every second
    }
}

void motor_task(void *pvParameters)
{
    while (1) {
        if (timer_running && !motor_extended) {
            move_motor(STEPS_PER_REVOLUTION);  // Extend
            motor_extended = 1;
        } else if (!timer_running && motor_extended) {
            move_motor(-STEPS_PER_REVOLUTION);  // Retract
            motor_extended = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}

void update_display()
{
    char buffer[16];
    ssd1306_clear_display();

    snprintf(buffer, sizeof(buffer), "%02d:%02d", timer_minutes, timer_seconds);
    ssd1306_write_string(buffer, 0, 0);

    const char *state_str = "Unknown";
    switch (timer_state) {
        case STATE_SET_MINUTES:
            state_str = "Set Minutes";
            break;
        case STATE_SET_SECONDS:
            state_str = "Set Seconds";
            break;
        case STATE_RUNNING:
            state_str = timer_running ? "Running" : "Paused";
            break;
    }
    ssd1306_write_string(state_str, 2, 0);
}

// Initialize I2C
void init_i2c()
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_stretch_tick = 300,
    };
    i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER);
    i2c_param_config(I2C_MASTER_NUM, &conf);

}

// Initialize button
void init_button()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<BUTTON_PIN),
        .pull_up_en = 0,
        .pull_down_en = 1
    };
    gpio_config(&io_conf);
}

// Initialize ADC for potentiometer
void init_adc()
{
    adc_config_t adc_conf = {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 8,
    };
    ESP_ERROR_CHECK(adc_init(&adc_conf));
}

// Initialize motor
void init_motor()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<IN1_PIN) | (1ULL<<IN2_PIN) | (1ULL<<IN3_PIN) | (1ULL<<IN4_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
}

void set_motor_pins(int in1, int in2, int in3, int in4)
{
    gpio_set_level(IN1_PIN, in1);
    gpio_set_level(IN2_PIN, in2);
    gpio_set_level(IN3_PIN, in3);
    gpio_set_level(IN4_PIN, in4);
}

void move_motor(int steps)
{
    int direction = (steps > 0) ? 1 : -1;
    steps = abs(steps);

    for (int i = 0; i < steps; i++) {
        int step_index = (direction > 0) ? (i % 4) : (3 - (i % 4));
        set_motor_pins(step_sequence[step_index][0], step_sequence[step_index][1],
                       step_sequence[step_index][2], step_sequence[step_index][3]);
        vTaskDelay(pdMS_TO_TICKS(STEP_DELAY_MS));
    }

    // Turn off all coils
    set_motor_pins(0, 0, 0, 0);
}

// Include your existing SSD1306 functions here: ssd1306_init, ssd1306_write_command, ssd1306_write_data, ssd1306_clear_display, ssd1306_write_string


// ... (include your existing SSD1306 functions here: ssd1306_init, ssd1306_write_command, ssd1306_write_data, ssd1306_clear_display, ssd1306_write_string)

esp_err_t ssd1306_write_data(uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true);  // Co = 0, D/C# = 1
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t ssd1306_write_command(uint8_t command) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);  // Co = 0, D/C# = 0
    i2c_master_write_byte(cmd, command, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t ssd1306_init() {
    esp_err_t ret;

    // Turn off the display
    ret = ssd1306_write_command(0xAE); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; // Extra command


    // Set display clock divide ratio/oscillator frequency
    ret = ssd1306_write_command(0xD5); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x80); if (ret != ESP_OK) return ret;

    // Set multiplex ratio
    ret = ssd1306_write_command(0xA8); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x1F); if (ret != ESP_OK) return ret;

    // Set display offset
    ret = ssd1306_write_command(0xD3); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret;

    // Set start line
    ret = ssd1306_write_command(0x40); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; // extra command

    // Enable charge pump regulator
    ret = ssd1306_write_command(0x8D); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x14); if (ret != ESP_OK) return ret;

    // Set memory addressing mode (horizontal)
    ret = ssd1306_write_command(0x20); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret;

    // Set segment re-map
    ret = ssd1306_write_command(0xA0); if (ret != ESP_OK) return ret; //extra
    //ret = ssd1306_write_command(0xA1); if (ret != ESP_OK) return ret;
    //ret = ssd1306_write_command(0xA1); if (ret != ESP_OK) return ret;

    // Set COM output scan direction
    //ret = ssd1306_write_command(0xC8); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0xC0); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; //extra

    // Set COM pins hardware configuration
    ret = ssd1306_write_command(0xDA); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x02); if (ret != ESP_OK) return ret;

    // Set contrast control
    ret = ssd1306_write_command(0x81); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x8F); if (ret != ESP_OK) return ret;

    // Set pre-charge period
    ret = ssd1306_write_command(0xD9); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0xF1); if (ret != ESP_OK) return ret;

    // Set VCOMH deselect level
    ret = ssd1306_write_command(0xDB); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x40); if (ret != ESP_OK) return ret;

    // Disable entire display on
    ret = ssd1306_write_command(0xA4); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; //extra

    // Set normal display
    ret = ssd1306_write_command(0xA6); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; //extra

    // Clear the display
    //ret = ssd1306_clear_display(); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x2E); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; //extra

    // Turn on the display
    ret = ssd1306_write_command(0xAF); if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0x00); if (ret != ESP_OK) return ret; //extra


    return ESP_OK;
}

esp_err_t ssd1306_clear_display() {
    esp_err_t ret;
    
    // Set column address range
    ret = ssd1306_write_command(0x21); // Set column address command
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0);    // Start column
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(127);  // End column
    if (ret != ESP_OK) return ret;

    // Set page address range
    ret = ssd1306_write_command(0x22); // Set page address command
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(0);    // Start page
    if (ret != ESP_OK) return ret;
    ret = ssd1306_write_command(3);    // End page
    if (ret != ESP_OK) return ret;

    uint8_t data[1] = {0x00};  // Example data to turn on all pixels in a column
    // Write 0 to all pixels
    for (int i = 0; i < 1024; i++) {  // 128 * 64 / 8 = 1024 bytes
        ret = ssd1306_write_data(data,1);
        if (ret != ESP_OK) return ret;
    }

    return ESP_OK;
}

void ssd1306_write_char(char c, uint8_t page, uint8_t column) {
    ssd1306_write_command(SSD1306_DISPLAYOFF); 
    if (c < 0x20 || c > 0x7F) {
        c = 0x20; // Replace unsupported characters with a space
    }

    ssd1306_write_command(0xB0 + page);     // Set page address
    ssd1306_write_command(column & 0x0F);   // Set lower column address
    ssd1306_write_command(0x10 | (column >> 4)); // Set higher column address

    for (int i = 0; i < 5; i++) {
        uint8_t data[1] = {font5x7_basic[c - 0x20][i]};
        ssd1306_write_data(data,1); // Write character data
    }
    
    uint8_t data[1] = {0x00};
    ssd1306_write_data(data,1); // Add a space between characters (1 column)
    ssd1306_write_command(SSD1306_DISPLAYON);

}

void ssd1306_write_string(const char *str, uint8_t page, uint8_t start_column) {
    uint8_t column = start_column;
    while (*str) {
        ssd1306_write_char(*str++, page, column);
        column += 6; // Move to the next column position
        if (column >= SSD1306_WIDTH) {
            break; // Stop if we reach the end of the display width
        }
    }
}

void app_main()
{
    printf("Inside main");
    fflush(stdout);

    init_i2c();
    init_button();
    init_adc();
    init_motor();
    ssd1306_init();
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second
    xTaskCreate(timer_task, "timer_task", 2048, NULL, 5, NULL);
    xTaskCreate(motor_task, "motor_task", 2048, NULL, 5, NULL);
}