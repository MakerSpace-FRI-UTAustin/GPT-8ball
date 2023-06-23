#include "Arduino.h"
#include "AudioTools.h"
#include "LittleFS.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "secrets.h"

AnalogAudioStream adc;
ConverterScaler<int16_t> scaler(1.0, -26427, 32700);

//Audio Information. May vary.
int sampleRate = 43000;
int channels = 1;
int samplingBits = 16;
int duration = 5;
int fileSize = (samplingBits * sampleRate * channels * duration) / 8;
int blockSize = 1024;
AudioInfo info(sampleRate, channels, samplingBits);

File audioFile;
const char *filename = "/speech.wav";

//TODO: Change to a method where a variable can store the current Filesystem
bool SDMODE = false;

EncodedAudioStream out(&audioFile, new WAVEncoder());
StreamCopy copier(out, adc);

void setup(void) {
  Serial.begin(115200);

  Serial.println("starting I2S-ADC...");
  auto cfg = adc.defaultConfig(RX_MODE);
  cfg.copyFrom(info);
  adc.begin(cfg);
  
  if (SDMODE){
    if (!SD.begin(true)) {
      Serial.println("SD Mount Failed");
      return;
    }
  }
  else{
    if (!LittleFS.begin(true)) {      
      Serial.println("LittleFS Mount Failed");
      return;
    }
    
  }
  recordClip();
  wifiSetup();
  sendAudio();
}


void recordSetup() {
  if (SDMODE){
    SD.remove(filename);
    audioFile = SD.open(filename, FILE_WRITE);
  }
  else{
    LittleFS.remove(filename);
    audioFile = LittleFS.open(filename, FILE_WRITE);    
  }

  if (!audioFile) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  audioFile.seek(0);
  auto cfg = out.defaultConfig();
  cfg.copyFrom(info);
  out.begin(cfg);
}

void recordClip() {
  recordSetup();
  Serial.println("starting record");

  unsigned long start = millis();
  for (int i = 0; i <= fileSize; i += blockSize) {
    copier.copy(scaler);
  }

  Serial.print("time taken: ");
  Serial.print((millis() - start)/1000);
  Serial.println("s");

  audioFile.close();
}


void sendAudio() {
  HTTPClient http;
  http.begin(URL);
  http.addHeader("Content-Type", "audio/x-wav");
  if (SDMODE){
    audioFile = SD.open(filename, FILE_READ);
  }
  else{
    audioFile = LittleFS.open(filename, FILE_READ);
  }

  int fileLen = audioFile.size();
  int httpCode = http.sendRequest("POST", &audioFile, fileLen);
  Serial.println(httpCode);
  http.end();
}


//Make into an AP to make configurations on
void wifiSetup() {
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
}

void loop() {
}
