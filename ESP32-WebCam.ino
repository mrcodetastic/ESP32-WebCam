/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-video-streaming-web-server-camera-home-assistant/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "config.h"

// https://github.com/bitbank2/JPEGDEC
// Used to decode the captured JPEG in PSRAM and do an assessment 
// on a pixel by pixel level based on a scaled image.
#include "JPEGDEC.h"


#define PART_BOUNDARY "123456789000000000000987654321"

// This project was tested with the AI Thinker Model, M5STACK PSRAM Model and M5STACK WITHOUT PSRAM
#define CAMERA_MODEL_AI_THINKER
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WITHOUT_PSRAM

// Not tested with this model
//#define CAMERA_MODEL_WROVER_KIT

#if defined(CAMERA_MODEL_WROVER_KIT)
  #define PWDN_GPIO_NUM    -1
  #define RESET_GPIO_NUM   -1
  #define XCLK_GPIO_NUM    21
  #define SIOD_GPIO_NUM    26
  #define SIOC_GPIO_NUM    27
  
  #define Y9_GPIO_NUM      35
  #define Y8_GPIO_NUM      34
  #define Y7_GPIO_NUM      39
  #define Y6_GPIO_NUM      36
  #define Y5_GPIO_NUM      19
  #define Y4_GPIO_NUM      18
  #define Y3_GPIO_NUM       5
  #define Y2_GPIO_NUM       4
  #define VSYNC_GPIO_NUM   25
  #define HREF_GPIO_NUM    23
  #define PCLK_GPIO_NUM    22

#elif defined(CAMERA_MODEL_M5STACK_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       32
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_M5STACK_WITHOUT_PSRAM)
  #define PWDN_GPIO_NUM     -1
  #define RESET_GPIO_NUM    15
  #define XCLK_GPIO_NUM     27
  #define SIOD_GPIO_NUM     25
  #define SIOC_GPIO_NUM     23
  
  #define Y9_GPIO_NUM       19
  #define Y8_GPIO_NUM       36
  #define Y7_GPIO_NUM       18
  #define Y6_GPIO_NUM       39
  #define Y5_GPIO_NUM        5
  #define Y4_GPIO_NUM       34
  #define Y3_GPIO_NUM       35
  #define Y2_GPIO_NUM       17
  #define VSYNC_GPIO_NUM    22
  #define HREF_GPIO_NUM     26
  #define PCLK_GPIO_NUM     21

#elif defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not selected"
#endif

#define RED_LED 33
#define FLASH_LED 4

httpd_handle_t stream_httpd = NULL;
unsigned long lastCaptureMillis = 0;   // last time image was sent
unsigned long lastStreamMillis = 0;   // last time image was sent

volatile bool pause_stream = false;

 // dont_send is used for capturing without uploading to remote server, used for calibration of camera
void uploadPhoto();

WiFiClient client;

#include "ESPTelnet.h"
#include "telnet_handlers.h"


// Static instance of the JPEGDEC structure. It requires about
// 17.5K of RAM. You can allocate it dynamically too. Internally it
// does not allocate or free any memory; all memory management decisions
// are left to you
JPEGDEC jpeg;
unsigned long photoLightAverage = 0;

// Indicator
void blinkRedLED_WiFiConnected(void * parameter){
  while(true) {

    // Flash when connected
    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(RED_LED, LOW);
      delay(200);
      digitalWrite(RED_LED, HIGH);    
    }
    delay(3000);   
    
  }
}
// HTTP Server
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";


static esp_err_t stream_handler(httpd_req_t *req)
{

  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true)
  {   

    if (pause_stream == true)
    {
      delay(10);
      continue;
    }

    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){

      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    lastStreamMillis = millis();
    Serial.printf("MJPG frame: %u bytes\n",(uint32_t)(_jpg_buf_len));

  }
  return res;
}

void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };
  
  //Serial.printf("Starting web server on port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &index_uri);
  }
}

int drawMCUs(JPEGDRAW *pDraw)
{
  //int x = pDraw->x;
  //int y = pDraw->y;
  int w = pDraw->iWidth;
  int h = pDraw->iHeight;

  //Serial.println(w, DEC);  

  for(int i = 0; i < w * h; i++)
  {
    uint8_t px_greyscale_value = pDraw->pPixels[i] & 0xFF;

    photoLightAverage += px_greyscale_value; // divide the sum total once we have completed decoding

    //Serial.print(value, DEC);
    //Serial.print(", ");
  }
  //Serial.println("");


  return 1; // returning true (1) tells JPEGDEC to continue decoding. Returning false (0) would quit decoding immediately.
} /* drawMCUs() */


/* Need to give time to the camera to calibrate. */
static esp_err_t camera_warmer()
{
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  Serial.println("Warming up camera for photo shoot...");

  // Give the camera four seconds or so.
  delay(100);  
  fb = esp_camera_fb_get();  
  if (!fb) {
    Serial.println("Camera capture failed");
    res = ESP_FAIL;
  } else {
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  delay(100);

  return res;
}


void uploadPhoto() {

  pause_stream = true;

  String getAll;
  String getBody;
  long lTime;
  char szTemp[64];

  camera_warmer();
  delay(1000);

  //telnet.println("Capturing timed photo...");  

  // Capture image and save to framebuffer
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart(); // Pretty punative here - reboot the entire devi
  }

  // Assess the image
  // Open a large JPEG image stored in FLASH memory (included as thumb_test.h)
  // This image is 12 megapixels, but has a 320x240 embedded thumbnail in it
  photoLightAverage = 0;
  if (jpeg.openRAM((uint8_t *) fb->buf, fb->len, drawMCUs))
  {
    Serial.println("Successfully opened JPEG image from ESP32 Cam PSRAM Buffer");
    Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(),
      jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());

    if (jpeg.hasThumb())
       Serial.printf("Thumbnail present: %d x %d\n", jpeg.getThumbWidth(), jpeg.getThumbHeight());

    jpeg.setPixelType(EIGHT_BIT_GRAYSCALE); // IMPORTANT, this is assumed in drawMCUs
    lTime = micros();
    
    if (jpeg.decode(0,0,JPEG_SCALE_EIGHTH))
    {
      lTime = micros() - lTime;
      sprintf(szTemp, "Successfully decoded image in %d us", (int)lTime);
      Serial.println(szTemp);

      // Calculate photo light average
      photoLightAverage = photoLightAverage / ((jpeg.getWidth()/8)*(jpeg.getHeight()/8));      

      Serial.print("Calculated photo's light average is: "); Serial.println(photoLightAverage, DEC);
    }

    jpeg.close();
  }

  if (photoLightAverage < photoLightThreshold  )
  {
      Serial.println("Captured photo below brightness threshold. Skipping.");
      pause_stream = false;
      return;
  }
 
  
  Serial.println("Connecting to server: " + serverName);
  client.setTimeout(25); // 15 seconds
  if (client.connect(serverName.c_str(), serverPort)) {
    Serial.println("Connection successful!");    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
 
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);
    Serial.println("Sent data to server.");
    
    esp_camera_fb_return(fb);
    pause_stream = false;

    int timoutTimer = 10000;
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + timoutTimer) > millis()) {
      Serial.print(".");
      delay(100);      
      while (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (getAll.length()==0) { state=true; }
          getAll = "";
        }
        else if (c != '\r') { getAll += String(c); }
        if (state==true) { getBody += String(c); }
        startTimer = millis();
      }
      if (getBody.length()>0) { break; }
    }
    Serial.println();
    client.stop();
    Serial.println(getBody);
    telnet.println(getBody);     

    digitalWrite(RED_LED, LOW);
    delay(100);
    digitalWrite(RED_LED, HIGH);

    telnet.println("Photo uploaded.");      


  }
  else {
    getBody = "Connection to " + serverName +  " failed.";
    Serial.println(getBody);
    telnet.println(getBody); 

  }

    pause_stream = false;

}



void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  Serial.begin(115200);
  Serial.setDebugOutput(false);

  // Led
  // https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/
  pinMode(RED_LED, OUTPUT);
  pinMode(FLASH_LED, OUTPUT); // bright flashlight

  digitalWrite(RED_LED, LOW);

  // Allocate PSRAM for image processing
  //rgb = (uint8_t *) ps_malloc(1600/8 * 1200/8); // one eigth decode and 8 bit greyscale!
  
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    Serial.println("PSRAM Found.");
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
  // Wi-Fi connection
// This part of code will try create static IP address
#ifndef USE_DHCP
 if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
#endif  

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Wifi Connected checker
  TaskHandle_t Task1;

  setupTelnet();  
  
  xTaskCreatePinnedToCore(
  blinkRedLED_WiFiConnected,            /* Task function. */
  "blinkRedLED",                 /* name of task. */
  1000,                    /* Stack size of task */
  NULL,                     /* parameter of the task */
  1,                        /* priority of the task */
  &Task1,                   /* Task handle to keep track of created task */
  0);                       /* Core */ 
  
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());

  digitalWrite(RED_LED, HIGH);

  // Start streaming web server
  startCameraServer();  
    
  delay(2000);
  uploadPhoto();

  digitalWrite(FLASH_LED, HIGH);
  delay(500);
  digitalWrite(FLASH_LED, LOW);  
}



void loop() {

  telnet.loop();
  unsigned long currentMillis = millis();
  if (currentMillis - lastCaptureMillis >= (60*1000*timerInterval)) {
    digitalWrite(RED_LED, LOW);    
    delay(2000);
  
    uploadPhoto();
  
    digitalWrite(RED_LED, HIGH);    
    lastCaptureMillis = currentMillis;
  }
}