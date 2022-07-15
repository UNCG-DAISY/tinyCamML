#include "Arduino.h"
#include "BluetoothSerial.h"
#include "esp_camera.h"
#include "defines.h"
#include "time.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include <Preferences.h>       // alternative to EEPROM library
#include <WiFi.h>
// https://github.com/eloquentarduino/EloquentTinyML
#include <EloquentTinyML.h>    // supposedly easy library for running .tflite models
#include <eloquent_tinyml/tensorflow.h>  

// .h file that contains the array with model information
#include "modeldata.h"

#define N_INPUTS 150528
#define N_OUTPUTS 1
#define TENSOR_ARENA_SIZE 2*1024

Preferences preferences;

void userSetup(BluetoothSerial SerialBT, int* cameraCaptureFreq, int* pictureNumber, struct tm tm){

bool exitflag = false;
int btTemp_int = 0;
String btTemp_str;

  SerialBT.println("Please Enter a Camera Capture Frequency (in minutes): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= 0) {
      SerialBT.println("Please Enter a Valid Camera Frequency Value: ");
    }
    else {
      SerialBT.printf("Camera Freq Set To %d Minutes\n", btTemp_int);
      *cameraCaptureFreq = MICROMINUTECONST * btTemp_int - 5000;
      exitflag = true;
    }
  }
  exitflag = false;

  SerialBT.println("What is the picture index? (If input is 0, index continues counting from the last picture taken. If no picture has been taken, starts from index 1): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int < 0) {
      SerialBT.println("Please Enter a Valid Picture Index: ");
    }
    else if(btTemp_int == 0) {
      SerialBT.printf("Camera index starting from last picture taken: %d\n", preferences.getInt("pic-num", 0) + 1);
      exitflag = true;
    }
    else {
      SerialBT.printf("Camera index starting from: %d\n", btTemp_int);
      *pictureNumber = btTemp_int;
      preferences.putInt("pic-num", *pictureNumber);
      //EEPROM.commit();
      exitflag = true;
    }
  }
  exitflag = false;

  SerialBT.println("Please Enter the Current Year: ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= 0) {
      SerialBT.println("Please Enter a Valid Year: ");
    }
    else {
      tm.tm_year = btTemp_int - YEAROFFSET;
      exitflag = true;
    }
  }
  exitflag = false;

  SerialBT.println("Please Enter the Current Month (Value 1-12): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= 0 || btTemp_int >= 13) {
      SerialBT.println("Please Enter a Valid Month Value: ");
    }
    else {
      tm.tm_mon = --btTemp_int;
      exitflag = true;
    }
  }
  exitflag = false;

  SerialBT.println("Please Enter the Current Day (1-31): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= 0 || btTemp_int >= 32) {
      SerialBT.println("Please Enter a Valid Day Value: ");
    }
    else {
      tm.tm_mday = btTemp_int;
      exitflag = true;
    }
  }
  exitflag = false;

    SerialBT.println("Please Enter the Current Hour (1-24): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= 0 || btTemp_int >= 25) {
      SerialBT.println("Please Enter a Valid Hour: ");
    }
    else {
      tm.tm_hour = btTemp_int;
      exitflag = true;
    }
  }
  exitflag = false;

  SerialBT.println("Please Enter the Current Minute (0-59): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= -1 || btTemp_int >= 60) {
      SerialBT.println("Please Enter a Valid Minute: ");
    }
    else {
      tm.tm_min = btTemp_int;
      exitflag = true;
    }
  }
  exitflag = false;

    SerialBT.println("Please Enter the Current Second (0-59): ");
  while(!exitflag) {
  while(!SerialBT.available()) {
    delay(10);
  }
  btTemp_str = SerialBT.readString();
  btTemp_int = btTemp_str.toInt();
    if(btTemp_int <= -1 || btTemp_int >= 60) {
      SerialBT.println("Please Enter a Valid Second: ");
    }
    else {
      tm.tm_sec = btTemp_int;
      exitflag = true;
    }
  }
  exitflag = false; 

    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);

    SerialBT.printf("Date & Time Set To: %s",asctime(&tm));
    delay(2000);

    SerialBT.printf("Setup Complete!");
    delay(2000);

    SerialBT.end();   // End BT serial setup communications
}

// Enable bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

RTC_DATA_ATTR bool setup_en = true;
RTC_DATA_ATTR int cameraCaptureFreq;

int pictureNumber = 0;
camera_fb_t * fb = NULL;
const char *mountpoint = "/picture";

BluetoothSerial SerialBT;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  pinMode(OBLED, OUTPUT);
  digitalWrite(OBLED, HIGH);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  preferences.begin("pic-num", false);

  bool nosd_flag = false;
  bool sdfail_flag = false;
  bool nocam_flag = false;

  struct tm thyme;

  // initialize model and check if it is okay
  tf.begin(modeldata);
  if(!tf.isOk()){
        Serial.print("ERROR: ");
        Serial.println(tf.getErrorMessage());
        
        while (true) delay(1000);
  }

  if(setup_en){
    SerialBT.begin("SDF_TEMP");
    while(!SerialBT.connected()) {
      delay(10);
    }
  }
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 5;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    SerialBT.printf("Camera init failed with error 0x%x", err);
    Serial.printf("Camera init failed with error 0x%x", err);
    nocam_flag = true;
  }
  if (nocam_flag) {
    digitalWrite(OBLED, LOW);
    delay(30000);  // delay for 12 second
    digitalWrite(OBLED, HIGH);
    nocam_flag = false;  
    return;
  }

    sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, 0); // lower the saturation
    s->set_whitebal(s,1); // enable white balance (1 - enable, 0 - disable)
    s->set_awb_gain(s,1); // enable auto white balance gain (1 - enable, 0 - disable)
    s->set_wb_mode(s,1); // enable auto white balance mode (0 - enable, 1 - disable)
    s->set_ae_level(s, 2); //
    s->set_gain_ctrl(s, 0);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 5);       // 0 to 30
    //s->set_gainceiling(s, (gainceiling_t)6);  // 0 to 6
    s->set_bpc(s, 1);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 0);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 0);            // 0 = disable , 1 = enable

  //Serial.println("Starting SD Card");
  if(!SD_MMC.begin(mountpoint,true,true)){
    SerialBT.println("SD Card Mount Failed");
    Serial.println("SD Card Mount Failed");
    sdfail_flag = true;
  }
  if (sdfail_flag) {
    for(int i=30; i>0; i--) {
      digitalWrite(OBLED, LOW);
      delay(500); // delay 0.5 seconds
      digitalWrite(OBLED, HIGH);
      delay(500); // delay 0.5 seconds
    }
    sdfail_flag = false;
    return;
  }

  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    SerialBT.println("No SD Card attached");
    Serial.println("No SD Card attached");
    nosd_flag = true;
  }
  if(nosd_flag) {
    for (int i=10; i>0; i--) {
    digitalWrite(OBLED, LOW);
    delay(1000);   // delay 10 seconds
    digitalWrite(OBLED, HIGH);
    delay(1000);
    digitalWrite(OBLED, LOW);
    delay(500);
    digitalWrite(OBLED, HIGH);
    delay(500);
    }
    nosd_flag = false;
    return;
  }

  // initialize EEPROM with predefined size
  //EEPROM.begin(EEPROM_SIZE);
  pictureNumber = preferences.getInt("pic-num", 0) + 1;

  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);

  if (setup_en) {
    userSetup(SerialBT, &cameraCaptureFreq, &pictureNumber, thyme);
    setup_en = false;
  }
}

void loop() {
  // Take Picture with Camera
  digitalWrite(OBLED, LOW);
  delay(1000); // allow time for AWB and AE
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Entering Sleep");
    esp_deep_sleep(cameraCaptureFreq - DEEPSLEEPOFFSET);
    return;
  }

  float predicted = tf.predict(fb);
  Serial.print("predicted: ");
  Serial.println(predicted);

  time_t now = time(0);
  tm* localtm = localtime(&now);

  Serial.printf("The local date and time is: %s", asctime(localtm));
  // Path where new picture will be saved in SD Card
  String path = mountpoint + String(pictureNumber) +".jpg";

  fs::FS &fs = SD_MMC; 
  Serial.printf("Picture file name: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    preferences.putInt("pic-num", pictureNumber);
    //EEPROM.writeInt(0, pictureNumber);
    //EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);

  // Convert now to tm struct for local timezone

  digitalWrite(OBLED, HIGH);
  Serial.println("Entering Sleep");
  esp_deep_sleep(cameraCaptureFreq - DEEPSLEEPOFFSET); // Sleep for x seconds
}
