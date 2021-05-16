#include <Arduino.h>
#include "esp_camera.h"
// #include <WiFi.h>

//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM
#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 1
#include <NimBLEDevice.h>
#include "camera_pins.h"
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define STORE_SIZE 100
const char *ssid = "Ancin 09 bdg";
const char *password = "ahmadfarisfs";

BLECharacteristic *pCharacteristicRx, *pCharacteristicTx;
BLEDescriptor *pDescriptor;
BLEServer *pServer;
BLEService *pService;
bool deviceConnected = false;
uint16_t peerMTU;
uint16_t connID;
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc)
  {
    connID = desc->conn_handle;

    deviceConnected = true;
    Serial.println("device connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    Serial.println("device disconnected");
  }
};

/* 
 *  BLE Characteristic Callbacks
 */
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {

    std::string rxValue = pCharacteristic->getValue();
    String strValue;
    if (rxValue.length() > 0)
    {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
        // strValue += rxValue[i];
      }
      // Serial.print(" ");
      // Serial.print(strValue.substring(0, 5));

      // Serial.println();
      // if (strValue.substring(0, 5) == "value")
      // {
      //   // storeValue(strValue.substring(5));
      // }

      Serial.println("*********");
    }
  }
};

/* 
 *  BLE Descriptor Callbacks
 */
class MyDisCallbacks : public BLEDescriptorCallbacks
{
  void onWrite(BLEDescriptor *pDescriptor)
  {
    uint8_t *rxValue = pDescriptor->getValue();

    if (pDescriptor->getLength() > 0)
    {
      if (rxValue[0] == 1)
      {
        //deviceNotifying=true;
      }
      else
      {
        // deviceNotifying=false;
      }
      Serial.println("*********");
      Serial.print("Received Descriptor Value: ");
      for (int i = 0; i < pDescriptor->getLength(); i++)
        Serial.print(rxValue[i]);

      Serial.println();
      Serial.println("*********");
    }
  }
};

void Prepare_BLE()
{
  Serial.println("INIT BL");
  // Create the BLE Device
  BLEDevice::init("UART Service");
  BLEDevice::setMTU(BLE_ATT_MTU_MAX);
  Serial.println("INIT BL done");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  Serial.println("create service");
  // Create the BLE Service
  pService = pServer->createService(SERVICE_UUID);
  Serial.println("create service done");
  // Create a BLE Characteristic
  pCharacteristicTx = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      NIMBLE_PROPERTY::NOTIFY);
  Serial.println("create service rx");
  pCharacteristicRx = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE);
  Serial.println("create service rx done");
  pCharacteristicRx->setCallbacks(new MyCallbacks());
  Serial.println("create setcb");

  // pDescriptor->setCallbacks(new MyDisCallbacks()); this makes crazy
  Serial.println("Waiting a client connection to notify...");
  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound())
  {
    Serial.println("FOUND");
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    Serial.println("NOT FOUND");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID)
  {
    s->set_vflip(s, 1);       // flip it back
    s->set_brightness(s, 1);  // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_UXGA);

  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");

  // startCameraServer();

  // Serial.print("Camera Ready! Use 'http://");
  // Serial.print(WiFi.localIP());
  // Serial.println("' to connect");
  // digitalWrite(4, LOW);
  Prepare_BLE();
}
camera_fb_t *fb = NULL;
uint8_t txValue = 0;
void loop()
{
  // if (deviceConnected)
  // {
  //   Serial.printf("*** Sent Value: %d ***\n",
  //                 txValue);
  //                 //max 512 byte, must use chunking
  //   pCharacteristicTx->setValue(&txValue, 1);
  //   pCharacteristicTx->notify();
  //   txValue++;
  // }

  //camera section

  delay(10000);

  // esp_err_t res = ESP_OK;
  // int64_t fr_start = esp_timer_get_time();
  Serial.println("Camera capture start");
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
  }
  size_t fb_len = 0;
  if (fb->format == PIXFORMAT_JPEG)
  {
    fb_len = fb->len;
    // for (int i = 0; i < fb_len; i++)
    // {
    //   Serial.print(fb->buf[i] < 16 ? "0" : "");
    //   Serial.print(fb->buf[i],HEX);
    // }

    // Serial.println("");
    // Serial.println("FINISH");
    // Serial.println(fb_len);

    //chunk
    if (deviceConnected)
    {
      uint16_t negotiatedMTU = pServer->getPeerMTU(connID);
      ; //negotiated MTU is always peer MTU, because esp32 is set to always request MAX
      if (negotiatedMTU > BLE_ATT_ATTR_MAX_LEN)
      {
        negotiatedMTU = BLE_ATT_ATTR_MAX_LEN;
      }
      Serial.print("Negotiated MTU : ");
      Serial.print(negotiatedMTU);
      Serial.print(" Local MTU : ");
      Serial.println(BLEDevice::getMTU());
      uint16_t chunkLen = (uint16_t)ceil((float)fb_len / (float)negotiatedMTU);
      Serial.print("Transfer image via BLE to android in : ");
      Serial.print(chunkLen);
      Serial.print(" chunks, ");
      Serial.print(fb_len);
      Serial.println(" bytes of data");
      for (uint16_t i = 0; i < chunkLen; i++)
      {
        // Serial.println(i);
        if (deviceConnected)
        {
          size_t toSend;
          // if(fb_len > negotiatedMTU){
          //   toSend = negotiatedMTU-(fb_len%negotiatedMTU);
          // }else{
          //   toSend = fb_len;
          // }
          // toSend=    fb_len-((i-1)*negotiatedMTU);
          //88
          toSend = fb_len - i * negotiatedMTU;
          if (toSend > negotiatedMTU)
          {
            toSend = negotiatedMTU;
          }
          pCharacteristicTx->setValue(&fb->buf[i * negotiatedMTU], toSend);
          pCharacteristicTx->notify();
        }
      }

      Serial.println("Transfer image to complete ");
    }else{
      Serial.println("No device connected ");
    }
  }
  else
  {
    Serial.println("Camera capture failed2");
  }
  esp_camera_fb_return(fb);
  Serial.println("Camera capture end");
}
