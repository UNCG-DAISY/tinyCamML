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
// light adjustment code from https://github.com/raduprv/esp32-cam_ov2640-timelapse/blob/main/ov2640_timelapse_github.ino
  camera_fb_t * fb = NULL;
sensor_t * s = esp_camera_sensor_get();
int light=0;
int day_switch_value=140;

  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
//  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
 // s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  //s->set_ae_level(s, 2);       // -2 to 2
  //s->set_aec_value(s, 1200);    // 0 to 1200
  s->set_gain_ctrl(s, 0);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)6);  // 0 to 6
  s->set_bpc(s, 1);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 0);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 0);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
 
    
   s->set_reg(s,0xff,0xff,0x01);//banksel

   light=s->get_reg(s,0x2f,0xff);
   Serial.print("First light is ");
   Serial.println(light);
   Serial.print("Old 0x0 value is");
   Serial.println(s->get_reg(s,0x0,0xff));

     //light=120+cur_pic*10;
     //light=0+cur_pic*5;

    if(light<day_switch_value)
    {
      //here we are in night mode
      if(light<45)s->set_reg(s,0x11,0xff,1);//frame rate (1 means longer exposure)
      s->set_reg(s,0x13,0xff,0);//manual everything
      s->set_reg(s,0x0c,0x6,0x8);//manual banding
           
      s->set_reg(s,0x45,0x3f,0x3f);//really long exposure (but it doesn't really work)
    }
    else
    {
      //here we are in daylight mode
      
      s->set_reg(s,0x2d,0xff,0x0);//extra lines
      s->set_reg(s,0x2e,0xff,0x0);//extra lines

      s->set_reg(s,0x47,0xff,0x0);//Frame Length Adjustment MSBs

    if(light<150)
    {
      s->set_reg(s,0x46,0xff,0xd0);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0xff);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0xff);//exposure (doesn't seem to work)
    }
    else if(light<160)
    {
      s->set_reg(s,0x46,0xff,0xc0);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0xb0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<170)
    {
      s->set_reg(s,0x46,0xff,0xb0);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<180)
    {
      s->set_reg(s,0x46,0xff,0xa8);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<190)
    {
      s->set_reg(s,0x46,0xff,0xa6);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x90);//exposure (doesn't seem to work)
    }
    else if(light<200)
    {
      s->set_reg(s,0x46,0xff,0xa4);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<210)
    {
      s->set_reg(s,0x46,0xff,0x98);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x60);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<220)
    {
      s->set_reg(s,0x46,0xff,0x80);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x20);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<230)
    {
      s->set_reg(s,0x46,0xff,0x70);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x20);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<240)
    {
      s->set_reg(s,0x46,0xff,0x60);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x20);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x80);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else if(light<253)
    {
      s->set_reg(s,0x46,0xff,0x10);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x40);//line adjust
      s->set_reg(s,0x45,0xff,0x10);//exposure (doesn't seem to work)
    }
    else
    {
      s->set_reg(s,0x46,0xff,0x0);//Frame Length Adjustment LSBs
      s->set_reg(s,0x2a,0xff,0x0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x0);//line adjust
      s->set_reg(s,0x45,0xff,0x0);//exposure (doesn't seem to work)
      s->set_reg(s,0x10,0xff,0x0);//exposure (doesn't seem to work)
    }
                                        
    s->set_reg(s,0x0f,0xff,0x4b);//no idea
    s->set_reg(s,0x03,0xff,0xcf);//no idea
    s->set_reg(s,0x3d,0xff,0x34);//changes the exposure somehow, has to do with frame rate

    s->set_reg(s,0x11,0xff,0x0);//frame rate
    s->set_reg(s,0x43,0xff,0x11);//11 is the default value
    }
    
   Serial.println("Getting first frame at");
   Serial.println(millis());
  
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Entering Sleep");
    esp_deep_sleep(cameraCaptureFreq - DEEPSLEEPOFFSET);
    return;
  }

   Serial.println("Got first frame at");
   Serial.println(millis());
   if(light==0)
    {
      s->set_reg(s,0x47,0xff,0x40);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xf0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==1)
    {
      s->set_reg(s,0x47,0xff,0x40);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xd0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==2)
    {
      s->set_reg(s,0x47,0xff,0x40);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xb0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==3)
    {
      s->set_reg(s,0x47,0xff,0x40);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x70);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==4)
    {
      s->set_reg(s,0x47,0xff,0x40);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x40);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==5)
    {
      s->set_reg(s,0x47,0xff,0x20);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==6)
    {
      s->set_reg(s,0x47,0xff,0x20);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x40);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==7)
    {
      s->set_reg(s,0x47,0xff,0x20);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x30);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==8)
    {
      s->set_reg(s,0x47,0xff,0x20);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x20);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==9)
    {
      s->set_reg(s,0x47,0xff,0x20);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x10);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light==10)
    {
      s->set_reg(s,0x47,0xff,0x10);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x70);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=12)
    {
      s->set_reg(s,0x47,0xff,0x10);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x60);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=14)
    {
      s->set_reg(s,0x47,0xff,0x10);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x40);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=18)
    {
      s->set_reg(s,0x47,0xff,0x08);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xb0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=20)
    {
      s->set_reg(s,0x47,0xff,0x08);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=23)
    {
      s->set_reg(s,0x47,0xff,0x08);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x60);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=27)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xd0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=31)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=35)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x60);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<=40)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x70);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<45)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x40);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    //after this the frame rate is higher, so we need to compensate
    else if(light<50)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0xa0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<55)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x70);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<65)
    {
      s->set_reg(s,0x47,0xff,0x04);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x30);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<75)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x80);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xff);//line adjust
    }
    else if(light<90)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x50);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0xbf);//line adjust
    }
    else if(light<100)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x20);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x8f);//line adjust
    }
    else if(light<110)
    {
      s->set_reg(s,0x47,0xff,0x02);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x10);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x7f);//line adjust
    }
    else if(light<120)
    {
      s->set_reg(s,0x47,0xff,0x01);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x10);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x5f);//line adjust
    }
    else if(light<130)
    {
      s->set_reg(s,0x47,0xff,0x00);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x2f);//line adjust
    }
    else if(light<140)
    {
      s->set_reg(s,0x47,0xff,0x00);//Frame Length Adjustment MSBs
      s->set_reg(s,0x2a,0xf0,0x0);//line adjust MSB
      s->set_reg(s,0x2b,0xff,0x0);//line adjust
    }
                   
    if(light<day_switch_value)s->set_reg(s,0x43,0xff,0x40);//magic value to give us the frame faster (bit 6 must be 1)

    //fb = esp_camera_fb_get();
   
    s->set_reg(s,0xff,0xff,0x00);//banksel
    s->set_reg(s,0xd3,0xff,0x8);//clock
    
    s->set_reg(s,0x42,0xff,0x2f);//image quality (lower is bad)
    s->set_reg(s,0x44,0xff,3);//quality
    
    //s->set_reg(s,0x96,0xff,0x10);//bit 4, disable saturation


    //s->set_reg(s,0xbc,0xff,0xff);//red channel adjustment, 0-0xff (the higher the brighter)
    //s->set_reg(s,0xbd,0xff,0xff);//green channel adjustment, 0-0xff (the higher the brighter)
    //s->set_reg(s,0xbe,0xff,0xff);//blue channel adjustment, 0-0xff (the higher the brighter)
    
    //s->set_reg(s,0xbf,0xff,128);//if the last bit is not set, the image is dim. All other bits don't seem to do anything but ocasionally crash the camera

    //s->set_reg(s,0xa5,0xff,0);//contrast 0 is none, 0xff is very high. Not really useful over 20 or so at most.

    //s->set_reg(s,0x8e,0xff,0x30);//bits 5 and 4, if set make the image darker, not very useful
    //s->set_reg(s,0x91,0xff,0x67);//really weird stuff in the last 4 bits, can also crash the camera

    //no sharpening
    s->set_reg(s,0x92,0xff,0x1);
    s->set_reg(s,0x93,0xff,0x0);
    
  time_t now = time(0);
  tm* localtm = localtime(&now);

    fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Entering Sleep");
    esp_deep_sleep(cameraCaptureFreq - DEEPSLEEPOFFSET);
    return;
  }

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

  //since we got the frame buffer, we reset the sensor and put it to sleep while saving the file
    s->set_reg(s,0xff,0xff,0x01);//banksel
    s->set_reg(s,0x12,0xff,0x80);//reset (we do this to clear the sensor registries, it seems to get more consistent images this way)
    delay(1);
    s->set_reg(s,0x09,0x10,0x10);//stand by

  digitalWrite(OBLED, HIGH);
  Serial.println("Entering Sleep");
  esp_deep_sleep(cameraCaptureFreq - DEEPSLEEPOFFSET); // Sleep for x seconds
}
