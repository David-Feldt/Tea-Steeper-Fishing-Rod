/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/i2c.h"

#define SSD1306_I2C_ADDRESS 0x3C  // SSD1306 I2C address

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

#define I2C_MASTER_SCL_IO           5    // GPIO number for I2C master clock (SCL)
#define I2C_MASTER_SDA_IO           4    // GPIO number for I2C master data (SDA)
#define I2C_MASTER_NUM              I2C_NUM_0  // I2C port number for master dev
#define I2C_MASTER_FREQ_HZ          100000     // I2C master clock frequency
#define I2C_MASTER_TX_BUF_DISABLE   0          // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 32

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

esp_err_t ssd1306_init(void);
esp_err_t ssd1306_write_command(uint8_t command);
esp_err_t ssd1306_write_data(uint8_t* data, size_t len);
esp_err_t ssd1306_clear_display(void);


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


// esp_err_t ssd1306_write_command(uint8_t command) {
//     uint8_t buffer[2] = {0x00, command};
//     esp_err_t ret;
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     ret = i2c_master_start(cmd); if (ret != ESP_OK) return ret;
//     ret = i2c_master_write_byte(cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true); if (ret != ESP_OK) return ret;
//     ret = i2c_master_write(cmd, buffer, sizeof(buffer), true); if (ret != ESP_OK) return ret;// Co = 0, D/C = 0 for command
//     ret = i2c_master_stop(cmd); if (ret != ESP_OK) return ret;
//     ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS); if (ret != ESP_OK) return ret;
//     i2c_cmd_link_delete(cmd);
//     return ESP_OK;
// }

// esp_err_t ssd1306_write_data(uint8_t data) {
//     uint8_t buffer[2] = {0x04, data};
//     esp_err_t ret;
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
//     ret = i2c_master_start(cmd);
//     ret = i2c_master_write_byte(cmd, (SSD1306_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true); if (ret != ESP_OK) return ret;
//     ret = i2c_master_write(cmd, buffer, sizeof(buffer), true); if (ret != ESP_OK) return ret;// Co = 0, D/C = 0 for command
//     ret = i2c_master_stop(cmd); if (ret != ESP_OK) return ret;
//     ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS); if (ret != ESP_OK) return ret;
//     i2c_cmd_link_delete(cmd);
//     return ESP_OK;
// }

// void ssd1306_init() {
//     ssd1306_write_command(0xAE); // Display off
//     ssd1306_write_command(0x20); // Set Memory Addressing Mode
//     ssd1306_write_command(0x10); // 00,Horizontal Addressing Mode; 01,Vertical Addressing Mode;
//     ssd1306_write_command(0xB0); // Set Page Start Address for Page Addressing Mode,0-7
//     ssd1306_write_command(0xC8); // Set COM Output Scan Direction
//     ssd1306_write_command(0x00); // Set low column address
//     ssd1306_write_command(0x10); // Set high column address
//     ssd1306_write_command(0x40); // Set start line address
//     ssd1306_write_command(0x81); // Set contrast control register
//     ssd1306_write_command(0xFF);
//     ssd1306_write_command(0xA1); // Set segment re-map 0 to 127
//     ssd1306_write_command(0xA6); // Set normal display
//     ssd1306_write_command(0xA8); // Set multiplex ratio(1 to 64)
//     ssd1306_write_command(0x3F); //
//     ssd1306_write_command(0xA4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content
//     ssd1306_write_command(0xD3); // Set display offset
//     ssd1306_write_command(0x00); // No offset
//     ssd1306_write_command(0xD5); // Set display clock divide ratio/oscillator frequency
//     ssd1306_write_command(0xF0); // Set divide ratio
//     ssd1306_write_command(0xD9); // Set pre-charge period
//     ssd1306_write_command(0x22);
//     ssd1306_write_command(0xDA); // Set com pins hardware configuration
//     ssd1306_write_command(0x12);
//     ssd1306_write_command(0xDB); // Set vcomh
//     ssd1306_write_command(0x20); // 0x20,0.77xVcc
//     ssd1306_write_command(0x8D); // Set DC-DC enable
//     ssd1306_write_command(0x14);
//     ssd1306_write_command(0xAF); // Turn on SSD1306 panel
// }

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
    ret = ssd1306_write_command(0xA1); if (ret != ESP_OK) return ret;

    // Set COM output scan direction
    ret = ssd1306_write_command(0xC8); if (ret != ESP_OK) return ret;
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


void ssd1306_draw_pixel(uint8_t x, uint8_t y) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
        return; // Ignore out of bounds
    }
    
    ssd1306_write_command(0xB0 + (y / 8)); // Set page address
    ssd1306_write_command(x & 0x0F);       // Set low column address
    ssd1306_write_command(0x10 | (x >> 4));// Set high column address
    
    uint8_t data[1] = {1 << (y % 8)};  // Example data to turn on all pixels in a column
    ssd1306_write_data(data,1);      // Write pixel data
}

// void ssd1306_clear_display() {
//     for (uint16_t i = 0; i < (SSD1306_WIDTH * SSD1306_HEIGHT / 8); i++) {
//         ssd1306_write_data(0x00);
//     }
// }

// void ssd1306_clear_display() {
//     ssd1306_write_command(SSD1306_DISPLAYOFF); 
//     for (uint8_t page = 0; page < (SSD1306_HEIGHT / 8); page++) {
//         ssd1306_write_command(0xB0 + page);     // Set page address
//         ssd1306_write_command(0x00);            // Set lower column address
//         ssd1306_write_command(0x10);            // Set higher column address

//         // Write 0x00 to all columns in the current page to clear the display
//         for (uint8_t column = 0; column < SSD1306_WIDTH; column++) {
//             ssd1306_write_data(0x00);
//         }
//     }
//     ssd1306_write_command(SSD1306_DISPLAYON);
// }

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

void i2c_scanner() {
    printf("Scanning for I2C devices...\n");
    for (int address = 1; address < 127; address++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);
        
        if (ret == ESP_OK) {
            printf("Found I2C device at address: 0x%02X\n", address);
        }
    }
    printf("I2C scan complete.\n");
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


void ssd1306_test_pattern() {
    for (int i = 0; i < 8; i++) {
        ssd1306_write_command(0xB0 + i); // Set page address
        ssd1306_write_command(0x00);     // Set lower column address
        ssd1306_write_command(0x10);     // Set higher column address
        
        for (int j = 0; j < 128; j++) {
            uint8_t data[1] = {(i + j) % 2 ? 0xFF : 0x00};
            ssd1306_write_data(data,1);
        }
    }
}

void app_main()
{
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .clk_stretch_tick = 300,
        // .clk_stretch_tick = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t ret;
    ret = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER);
    if (ret != ESP_OK) {
        printf("Failed to install I2C driver: %s\n", esp_err_to_name(ret));
    } else {
        printf("I2C driver installed successfully\n");
    }

    ret = i2c_param_config(I2C_MASTER_NUM, &i2c_conf);
    if (ret != ESP_OK) {
        printf("Failed to configure I2C parameters: %s\n", esp_err_to_name(ret));
    } else {
        printf("I2C parameters configured successfully\n");
    }

    ret = ssd1306_init();
    if (ret != ESP_OK) {
        printf("Display initialization failed\n");
        return;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);  // Short delay after init

    for (int i = 0; i < 1024; i++) {
        uint8_t data[1] = {0xFF};  // Example data to turn on all pixels in a column
        ssd1306_write_data(data,1);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second

    // ssd1306_init();
    ssd1306_clear_display();

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Wait 1 second
    ssd1306_write_string("Harkirat",0,0);

    printf("Hello Board!\n");  /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");

    fflush(stdout);
    esp_restart();
}


