// ES7210 microphone I2S using the new i2s_std driver.
//
// The firmware's speaker path uses ESP_I2S (i2s_std). ESP-IDF refuses to run
// the legacy driver/i2s.h in the same image ("CONFLICT! The new i2s driver
// can't work along with the legacy i2s driver"), so the official Waveshare
// example's legacy config cannot be used here. This file configures the same
// ES7210 as a slave ADC and the ESP32-S3 as an i2s_std master receiver.
#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s_std.h>
#include <driver/i2s_common.h>
#include "es7210.h"
#include "../../pin_config.h"

static bool s_ready = false;
static i2s_chan_handle_t s_rx = nullptr;

bool micBegin() {
  if (s_ready) return true;

  audio_hal_codec_config_t cfg = {
      .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
      .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
      .i2s_iface = {
          .mode = AUDIO_HAL_MODE_SLAVE,
          .fmt = AUDIO_HAL_I2S_NORMAL,
          .samples = AUDIO_HAL_16K_SAMPLES,
          .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
      },
  };
  esp_err_t err = es7210_adc_init(&Wire, &cfg);
  err |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
  err |= es7210_adc_set_gain_all(GAIN_37_5DB);
  err |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
  es7210_set_mute(false);
  if (err != ESP_OK) {
    Serial.printf("MIC: ES7210 init 0x%X\n", (unsigned)err);
    return false;
  }

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chan_cfg, nullptr, &s_rx) != ESP_OK) {
    Serial.println("MIC: i2s_new_channel failed");
    es7210_adc_deinit();
    return false;
  }

  i2s_std_config_t std_cfg = {};
  std_cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000);
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  std_cfg.gpio_cfg.mclk = (gpio_num_t)I2S_MCK_IO;
  std_cfg.gpio_cfg.bclk = (gpio_num_t)I2S_BCK_IO;
  std_cfg.gpio_cfg.ws = (gpio_num_t)I2S_WS_IO;
  std_cfg.gpio_cfg.dout = (gpio_num_t)-1;
  std_cfg.gpio_cfg.din = (gpio_num_t)I2S_DI_IO;

  if (i2s_channel_init_std_mode(s_rx, &std_cfg) != ESP_OK ||
      i2s_channel_enable(s_rx) != ESP_OK) {
    Serial.println("MIC: i2s std RX init failed");
    i2s_del_channel(s_rx);
    s_rx = nullptr;
    es7210_adc_deinit();
    return false;
  }
  s_ready = true;
  Serial.println("MIC: ES7210 ready (i2s_std)");
  return true;
}

// Returns mono 16 kHz samples (takes the left slot of each stereo frame).
int micReadMono(int16_t *dst, int maxSamples, uint32_t timeoutMs) {
  if (!s_ready || !s_rx || maxSamples <= 0) return 0;
  static int16_t buf[512 * 2];
  int wantFrames = maxSamples < 512 ? maxSamples : 512;
  size_t bytes = 0;
  if (i2s_channel_read(s_rx, buf, (size_t)wantFrames * 4, &bytes,
                       pdMS_TO_TICKS(timeoutMs)) != ESP_OK) return 0;
  int frames = (int)(bytes / 4);
  for (int i = 0; i < frames; i++) dst[i] = (int16_t)((buf[i * 2] + buf[i * 2 + 1]) / 2);
  return frames;
}

void micEnd() {
  if (!s_ready) return;
  if (s_rx) {
    i2s_channel_disable(s_rx);
    i2s_del_channel(s_rx);
    s_rx = nullptr;
  }
  es7210_adc_ctrl_state(AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_STOP);
  es7210_adc_deinit();
  s_ready = false;
}
