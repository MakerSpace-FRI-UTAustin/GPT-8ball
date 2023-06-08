#include "Arduino.h"
#include "AudioTools.h"
#include "FS.h"
#include "SPIFFS.h"
#include "WiFi.h"
#include "HTTPClient.h"

AnalogAudioStream adc;
// const int32_t max_buffer_len = 1024;
// uint8_t buffer[max_buffer_len];
ConverterScaler<int16_t> scaler(1.0, -26427, 32700 );
int sampleRate = 44100;
int channels = 1;
int samplingBits = 16;
int recordTime = 10;
int readSize = 2048;
int flashSize = sampleRate * channels * samplingBits / 8 * recordTime;

const char* ssid = "Rahul’s iPhone"; 
const char* password = "12345678";

AudioInfo info(sampleRate, channels, samplingBits);
const char *filename = "/speech.wav";                                                                        
File audioFile;                                                 
EncodedAudioStream out(&audioFile, new WAVEncoder());             
StreamCopy copier(out, adc);

void recordSetup(){
  SPIFFS.remove(filename);
  audioFile = SPIFFS.open(filename, FILE_WRITE);
  if(!audioFile){
   Serial.println("There was an error opening the file for writing");
   return;
  }
  audioFile.seek(0);

  //Maybe need to pass in a conf?
  auto cfg = out.defaultConfig();
  cfg.copyFrom(info);
  out.begin(cfg);
  
}
void setup(void) {
  Serial.begin(115200);

  wifiSetup();  
  Serial.println("starting I2S-ADC...");
  auto cfg = adc.defaultConfig(RX_MODE);
  cfg.copyFrom(info);
  // Serial.println("Buffer size: "+ (int) cfg.buffer_size);
  adc.begin(cfg);

  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  recordClip();

  sendAudio();

}

void recordClip(){
  recordSetup();
  Serial.println("starting record");
  
  unsigned long start = millis();
  //Prolly need to scale adc audio
  while (millis() - start < recordTime * 1000){
    copier.copy(scaler);
  }
  Serial.println(millis() - start);
  // Serial.println("length: "+ (millis() - start));

  audioFile.close();
}


//Testing
void sendAudio(){
  HTTPClient http;
  http.begin("https://webhook.site/1be50442-9523-4105-9867-c3cefa86a9f3");
  http.addHeader("Content-Type", "audio/wav");
  audioFile = SPIFFS.open(filename, FILE_READ);
  int httpCode = http.sendRequest("POST", &audioFile, audioFile.size());
  Serial.println(httpCode);
}


//Make into an AP to make configurations on
void wifiSetup(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());   
}

void loop() {
    
}   
