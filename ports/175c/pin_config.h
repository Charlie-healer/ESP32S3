#pragma once

// Waveshare ESP32-S3-Touch-AMOLED-1.75C (SKU 33691 / 33692)
// 与上游 boards/175/MotoNav/pin_config.h 的差异：
//   LCD_RESET: 39 -> 1
//   TP_RESET : 40 -> 2
// 1.75C 没有 microSD 卡槽，原 SDMMC_* 定义已删除（GPIO 1/2 被复位占用）。

#define XPOWERS_CHIP_AXP2101

#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 1
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// TOUCH
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 2

// ES8311 / ES7210
#define I2S_MCK_IO 16
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8

#define MCLKPIN             42
#define BCLKPIN              9
#define WSPIN               45
#define DOPIN               10
#define DIPIN                8
#define PA                  46
