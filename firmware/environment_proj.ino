// environment_proj.ino
// ESP32-C6 — CO2, temp/humidity, sound (I2S), LDR, SD card (SDIO), MP3 playback
//
// Libraries needed (install via Arduino Library Manager):
//   - SensirionI2CScd4x   (CO2 sensor - SCD40)
//   - Adafruit_AHTX0      (temp/humidity - U6, change if your part differs)
//   - Adafruit_MCP23X17   (GPIO expander - MCP23017)
//   - ESP8266Audio        (MP3 playback)
//   - Arduino core for ESP32 (includes SD_MMC, driver/i2s)

#include <Wire.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_MCP23X17.h>
#include <SD_MMC.h>
#include <driver/i2s.h>
#include "AudioFileSourceSD_MMC.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

// ──────────────────────────────────────────────
// PIN DEFINITIONS
// ──────────────────────────────────────────────

// I2C (shared bus)
#define I2C_SDA   21
#define I2C_SCL   22

// I2S microphone (ICS-43434)
#define I2S_MIC_PORT    I2S_NUM_0
#define I2S_MIC_SCK     4   // A2
#define I2S_MIC_WS      3   // A1
#define I2S_MIC_SD_IN   5   // D3
// LR pin (A0) → tie to GND for left channel, not a GPIO

// SD card (SDIO 4-bit via SD_MMC)
// SD_MMC uses fixed pins on ESP32-C6:
//   CLK → GPIO19, CMD → GPIO18, D0 → GPIO20
//   D1  → GPIO21, D2  → GPIO22, D3  → GPIO23
// (No define needed — SD_MMC handles these internally)

// I2S speaker output (for MP3 playback via AudioOutputI2S)
// Uses I2S_NUM_1 — connect your speaker amp to these pins:
#define I2S_SPK_PORT    I2S_NUM_1
#define I2S_SPK_BCLK    6
#define I2S_SPK_LRCLK   7
#define I2S_SPK_DOUT    8

// LDR (light dependent resistor) — voltage divider output → ADC
#define LDR_PIN         A3   // adjust if wired differently

// ──────────────────────────────────────────────
// GLOBALS
// ──────────────────────────────────────────────

SensirionI2CScd4x scd4x;
Adafruit_AHTX0    aht;
Adafruit_MCP23X17 mcp;

AudioGeneratorMP3     *mp3      = nullptr;
AudioFileSourceSD_MMC *mp3File  = nullptr;
AudioOutputI2S        *mp3Out   = nullptr;

// I2S mic sample buffer
#define MIC_SAMPLE_COUNT 1024
int32_t micBuffer[MIC_SAMPLE_COUNT];

// ──────────────────────────────────────────────
// SETUP
// ──────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Booting...");

  // ── I2C bus ──
  Wire.begin(I2C_SDA, I2C_SCL);

  // ── CO2 sensor (SCD40) ──
  scd4x.begin(Wire);
  uint16_t scdErr;
  scdErr = scd4x.stopPeriodicMeasurement();   // stop if already running
  delay(500);
  scdErr = scd4x.startPeriodicMeasurement();
  if (scdErr) {
    Serial.print("SCD40 start error: ");
    Serial.println(scdErr);
  } else {
    Serial.println("SCD40 ready");
  }

  // ── Temp / humidity sensor (AHT — U6) ──
  if (!aht.begin()) {
    Serial.println("AHT sensor not found — check wiring");
  } else {
    Serial.println("AHT sensor ready");
  }

  // ── GPIO expander (MCP23017) ──
  if (!mcp.begin_I2C()) {   // default address 0x20
    Serial.println("MCP23017 not found — check wiring");
  } else {
    Serial.println("MCP23017 ready");
  }

  // ── LDR (ADC) ──
  analogReadResolution(12);  // 0–4095

  // ── I2S microphone ──
  setupMic();

  // ── SD card (SDIO 4-bit) ──
  // SD_MMC.begin() uses 4-bit by default on ESP32
  if (!SD_MMC.begin()) {
    Serial.println("SD card mount failed — check wiring");
  } else {
    Serial.printf("SD card ready — %llu MB\n",
                  SD_MMC.cardSize() / (1024 * 1024));
  }

  // ── I2S speaker output ──
  setupSpeaker();

  Serial.println("All systems go.\n");
}

// ──────────────────────────────────────────────
// LOOP
// ──────────────────────────────────────────────

void loop() {
  readCO2();
  readTempHumidity();
  readLDR();
  readMic();

  // Play a file if mp3 playback is active
  if (mp3 && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("MP3 playback finished");
    }
  }

  delay(5000);  // read sensors every 5 s (SCD40 updates every 5 s)
}

// ──────────────────────────────────────────────
// SENSOR FUNCTIONS
// ──────────────────────────────────────────────

void readCO2() {
  bool dataReady = false;
  uint16_t err = scd4x.getDataReadyFlag(dataReady);
  if (err || !dataReady) return;

  uint16_t co2;
  float    temperature, humidity;
  err = scd4x.readMeasurement(co2, temperature, humidity);
  if (err) {
    Serial.print("SCD40 read error: ");
    Serial.println(err);
    return;
  }
  Serial.printf("[CO2]  %u ppm  |  %.1f °C  |  %.1f %%RH\n",
                co2, temperature, humidity);
}

void readTempHumidity() {
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp);
  Serial.printf("[AHT]  %.1f °C  |  %.1f %%RH\n",
                temp.temperature, hum.relative_humidity);
}

void readLDR() {
  int raw = analogRead(LDR_PIN);
  // Higher raw value = more resistance = darker
  // Lower raw value  = less resistance = brighter
  float voltage    = raw * (3.3f / 4095.0f);
  float brightness = 100.0f - (raw / 4095.0f * 100.0f); // 0=dark, 100=bright
  Serial.printf("[LDR]  raw=%d  voltage=%.2fV  brightness=%.0f%%\n",
                raw, voltage, brightness);
}

void readMic() {
  size_t bytesRead = 0;
  esp_err_t err = i2s_read(I2S_MIC_PORT,
                            micBuffer,
                            sizeof(micBuffer),
                            &bytesRead,
                            pdMS_TO_TICKS(100));
  if (err != ESP_OK) {
    Serial.printf("I2S mic read error: %d\n", err);
    return;
  }

  int samplesRead = bytesRead / sizeof(int32_t);

  // Compute RMS amplitude as a simple sound level indicator
  int64_t sum = 0;
  for (int i = 0; i < samplesRead; i++) {
    // ICS-43434 outputs 24-bit data left-justified in 32-bit word
    int32_t sample = micBuffer[i] >> 8;
    sum += (int64_t)sample * sample;
  }
  float rms = sqrt((float)sum / samplesRead);
  Serial.printf("[MIC]  samples=%d  RMS=%.0f\n", samplesRead, rms);
}

// ──────────────────────────────────────────────
// MP3 PLAYBACK
// Call playMP3("/yourfile.mp3") from anywhere
// ──────────────────────────────────────────────

void playMP3(const char* path) {
  // Stop any existing playback
  if (mp3 && mp3->isRunning()) {
    mp3->stop();
  }
  delete mp3;
  delete mp3File;
  // Note: don't delete mp3Out — reuse the output

  mp3File = new AudioFileSourceSD_MMC(path);
  if (!mp3File->isOpen()) {
    Serial.printf("Could not open %s\n", path);
    return;
  }

  if (!mp3Out) {
    mp3Out = new AudioOutputI2S(I2S_SPK_PORT);
    mp3Out->SetPinout(I2S_SPK_BCLK, I2S_SPK_LRCLK, I2S_SPK_DOUT);
    mp3Out->SetGain(0.5);  // 0.0–1.0 — adjust volume here
  }

  mp3 = new AudioGeneratorMP3();
  if (mp3->begin(mp3File, mp3Out)) {
    Serial.printf("Playing %s\n", path);
  } else {
    Serial.println("MP3 begin failed");
  }
}

// ──────────────────────────────────────────────
// I2S SETUP HELPERS
// ──────────────────────────────────────────────

void setupMic() {
  i2s_config_t mic_cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 256,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t mic_pins = {
    .bck_io_num   = I2S_MIC_SCK,
    .ws_io_num    = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_MIC_SD_IN
  };
  esp_err_t err = i2s_driver_install(I2S_MIC_PORT, &mic_cfg, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Mic I2S driver install failed: %d\n", err);
    return;
  }
  i2s_set_pin(I2S_MIC_PORT, &mic_pins);
  Serial.println("I2S mic ready");
}

void setupSpeaker() {
  // AudioOutputI2S initialises the I2S peripheral itself when
  // playMP3() is first called, so nothing extra needed here.
  // This function is a placeholder if you need custom config later.
  Serial.println("I2S speaker ready (initialised on first play)");
}
