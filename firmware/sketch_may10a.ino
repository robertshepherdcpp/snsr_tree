#include <ArduinoSound.h>
#include <Wire.h>
#include <Audio.h>
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <SensirionI2cScd4x.h>
#include <SD_MCC.h>
#include <SD.h>
#include <Adafruit_MCP23X17.h>

// microphone stuff
#include "I2SSampler.h"

i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = 16000,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

// i2s pins
i2s_pin_config_t i2s_pins = {
  .bck_io_num = GPIO_NUM_5,
  .ws_io_num = GPIO_NUM_19,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = GPIO_NUM_18
};

I2SSampler *i2s_sampler = NULL;

// for getting the time via wifi:
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// replace with your network credentials!! or else the project wont work... needed to get the time.
const char *ssid = "REPLACE_WITH_YOUR_SSID";
const char *password = "REPLACE_WITH_YOUR_PASSWORD";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

// on the esp32

#define LDR_B 6

// however need to check with these that i can just wire these like that
#define SDA_BUS 22
#define SDL_BUS 23

// on the mc23017, bank A: 1-8, bank b: 21-28

// #define SCK_MC TODO
// #define SCA_MC TODO

#define LED1 3
#define LED2 2
#define LED3 1
#define LED4 27

Adafruit_MCP23X08 mcp;

// i2c addresses
#define SCD40 0x62
// #define BME280 0x76  // could be 0x77 if SDO is connected to VCC

// humidity and temperature notes:
/*
humidity: 0-100% (recommended: 40-60)
temperature: -40-85 (recommended: 20-22)
*/

// humidity + temperature sensor
float humidity_percentage;
float temperature;  // in degrees

// microphone
int decibels;

// co2 levels ppm
uint16_t co2_levels;

// light levels in lux
int light_level;

// for the co2 sensor
SensirionI2cScd4x sensor;

static int16_t error;

/*
VALUES SORTED:

CO2 levels
temperature
humidity_percentage

light

sound input
*/

auto read_values() {
  // updating the time:
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

  // get the hour.
  int splitT = formattedDate.indexOf("T");
  timeStamp = formattedDate.substring(splitT + 1, splitT + 3);
  int hour = timeStamp.toInt();
  if (hour <= 7 && hour >= 22) {
    dayTime = true;
  } else {
    dayTime = false;
  }

  // reading from the co2 sensor
  bool dataReady = false;
  uint16_t co2Concentration = 0;  // in ppm
  float temp = 0.0;               // in degrees
  float relativeHumidity = 0.0;   // percentage
  delay(5000);
  error = sensor.getDataReadyStatus(dataReady);
  if (error != NO_ERROR) {
    return;
  }
  while (!dataReady) {
    delay(100);
    error = sensor.getDataReadyStatus(dataReady);
    if (error != NO_ERROR)
      return;
  }
}

error = sensor.readMeasurement(co2Concentration, temp, relativeHumidity);
if (error != NO_ERROR) {
  return;
}

humidity_percentage = relativeHumidity;
co2_levels = co2Concentration;
temperature = temp;

// read light intensity
int ldrValue = 0;
ldrValue = mcp.analogRead(LDR_B);  // well say that the threshhold is 600 and if it is less than that we will put the lights on.
light_level = ldrValue;


// read the sound input:
if (amplitudeAnalyzer.available()) {
  // read the new amplitude
  int amplitude = amplitudeAnalyzer.read();
  // then turn it into decibels
  int decibels = 20 * log10(abs(amplitude));  // greater than 80, we tell user to put it down.
}
}

bool lightsOn = false;

// files needed: humidity_up, humidity_down, temp_up, temp_down all .mp3 files in sd card

auto handle_values() {

  // handling the humidifier
  if (humidity_percentage < 40) {
    // audio saying: "its a bit dry in here, but your humidifier on!"
    played = false;
    if (!played) {
      audio.connecttoFS(SD, "/humidity_up.mp3");
      played = true;
    }
    delay(3000);
  } else if (humidity_percentage > 60) {
    // audio saying: "its a bit humid in here! maybe put your dehumidifier on!"
    played = false;
    if (!played) {
      audio.connecttoFS(SD, "/humidity_down.mp3");
      played = true;
    }
    delay(3000);
  }

  // handling temperature
  if (temperature < 19) {
    // audio saying, put the temperature up!
    played = false;
    if (!played) {
      audio.connecttoFS(SD, "/temp_up.mp3");
      played = true;
    }
    delay(3000);
  }
  if (temperature > 23) {
    // audio saying: its a bit warm in here, try cooling down!
    played = false;
    if (!played) {
      audio.connecttoFS(SD, "/temp_down.mp3");
      played = true;
    }
    delay(3000);
  }

  // handling the sound.
  if (decibels > 80) {
    // flash the leds 3 times (because playing audio would be kinda silly)
    for (int i = 0; i < 3; i++) {
      mcp.digitalWrite(LED1, HIGH);
      mcp.digitalWrite(LED2, HIGH);
      mcp.digitalWrite(LED3, HIGH);
      mcp.digitalWrite(LED4, HIGH);
      delay(500);
      mcp.digitalWrite(LED1, LOW);
      mcp.digitalWrite(LED2, LOW);
      mcp.digitalWrite(LED3, LOW);
      mcp.digitalWrite(LED4, LOW);
      delay(500);
    }
  }

  // handling the co2
  if (co2_levels > 5000) {
    // audio saying: "co2 levels too high, open your windows!"
    played = false;
    if (!played) {
      audio.connecttoFS(SD, "/co2_lower.mp3");
      played = true;
    }
    delay(3000);
  }

  if (light_levels < 600 && !nightTime) {
    // put on LED lights.
    lightsOn = true;
    mcp.digitalWrite(LED1, HIGH);
    mcp.digitalWrite(LED2, HIGH);
    mcp.digitalWrite(LED3, HIGH);
    mcp.digitalWrite(LED4, HIGH);
  } else if (light_levels >= 600 && lightsOn) {
    mcp.digitalWrite(LED1, LOW);
    mcp.digitalWrite(LED2, LOW);
    mcp.digitalWrite(LED3, LOW);
    mcp.digitalWrite(LED4, LOW);
  }
}

// MICROPHONE STUFF

I2SSampler *i2s_sampler = NULL;

uint8_t *audio_buffer = NULL;
int buffersize_bytes = NULL;



void i2sWriterTask(void *param) {
  const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
  while (true) {
    uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    if (ulNotificationValue > 0) {
      audio_buffer = (uint8_t *)sampler->getCapturedAudioBuffer()
                       buffersize_bytes = sampler->getBufferSizeInBytes();
      float x_Pascal = (float)raw_sample / (float)(1 << 23) * 20.0f;  // Convert RMS in Pascals to dBSPL
      float Lz = 20.0 * log10(rms / (20e-6));                         // sound in decibels
      decibels = Lz;
    }
  }
}

#define I2S_DOUT 1
#define I2S_BCLK 2
#define I2S_LRC 16

#define SD_SCK 19
#define SD_MISO 20
#define SD_MOSI 18
#define SD_CS 21

bool played = false;


void setup() {
  // sorting out the microphone:
  // i2s_sampler = new I2SSampler();
  // i2s_sampler->start(I2S_NUM_1, i2s_pins, i2s_config, 32768, writer_task_handle);
  Serial.begin(115200);

  // sorting out loading sounds from the sd card and playing them on the speaker.
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS)) {
    return;  // Halt setup sequence
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(12);

  // sorting out the microphone:
  i2s_sampler = new I2SSampler();
  TaskHandle_t writer_task_handle;
  xTaskCreate(i2sWriterTask, "I2S Writer Task", 4096, i2s_sampler, 1, &writer_task_handle);
  i2s_sampler->start(I2S_NUM_1, i2s_pins, i2s_config, 32768, writer_task_handle);

  // for the wifi:
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  timeClient.begin();
  timeClient.setTimeOffset(0);  // GMT time (here in Britain, change for if ur in a different location.)
  // setting up the LEDS
  mcp.pinMode(LED1, OUTPUT);
  mcp.pinMode(LED2, OUTPUT);
  mcp.pinMode(LED3, OUTPUT);
  mcp.pinMode(LED4, OUTPUT);

  // setting up the i2c channel
  Wire.begin();
  wire.begin(SCD40);  // setting up the co2 sensor as a slave.
  Serial.begin(115200);
  sensor.begin(Wire, SCD40_I2C_ADDR_62);

  // the microphone
  if (!AudioInI2S.begin(44100, 32)) {
    Serial.println("Failed to initialize I2S input!");
    while (1)
      ;  // do nothing
  }

  if (!amplitudeAnalyzer.input(AudioInI2S)) {
    Serial.println("Failed to set amplitude analyzer input!");
    while (1)
      ;

    if (!SD.begin(SD_card_select)) {
      Serial.println("cant connect to the SD card....");
      return 0;
    }

    // Initialize file system
    SPIFFS.begin();

    // Create audio components
    file = new AudioFileSourceSD("/music.mp3");
    out = new AudioOutputI2S();
    out.setPinout(24);
    mp3 = new AudioGeneratorMP3();

    out->SetGain(0.16);  // setting the volume.

    // Start playback
    mp3->begin(file, out);
  }

  void loop() {
    read_values();
    handle_values();

    // Keep calling loop while playing
    if (mp3->isRunning()) {
      if (!mp3->loop()) mp3->stop();
    } else {
      // Playback has stopped
    }
    audio.loop();
  }
