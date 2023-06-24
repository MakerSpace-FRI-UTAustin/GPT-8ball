#include "Arduino.h"
#include "AudioTools.h"
#include "LittleFS.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "WiFiClientSecure.h"
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

char *cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\n" \
  "RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\n" \
  "VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\n" \
  "DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\n" \
  "ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\n" \
  "VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\n" \
  "mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\n" \
  "IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\n" \
  "mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\n" \
  "XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\n" \
  "dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\n" \
  "jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\n" \
  "BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\n" \
  "DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\n" \
  "9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\n" \
  "jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\n" \
  "Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\n" \
  "ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\n" \
  "R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\n" \
  "-----END CERTIFICATE-----\n";


void setup(void) {
  Serial.begin(115200);

  Serial.println("starting I2S-ADC...");
  auto cfg = adc.defaultConfig(RX_MODE);
  cfg.copyFrom(info);
  adc.begin(cfg);
  

  //TODO: Make FS be stored into global variable and then controlled
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
  //Initialize the file
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
  //Prompts encoder to write metadata to file
  audioFile.seek(0);
  auto cfg = out.defaultConfig();
  cfg.copyFrom(info);
  out.begin(cfg);
}

void recordClip() {
  recordSetup();
  Serial.println("starting record");

  unsigned long start = millis();
  //Records a set duration of audio by determining total file size of audio
  for (int i = 0; i <= fileSize; i += blockSize) {
    copier.copy(scaler);
  }

  Serial.println(String("time taken: ") + ((millis() - start)/1000) + String("s"));

  audioFile.close();
}


void sendAudio() {
  if (SDMODE){
    audioFile = SD.open(filename, FILE_READ);
  }
  else{
    audioFile = LittleFS.open(filename, FILE_READ);
  }
  audioFile.seek(0);

  //information of the API to connect to
  int port = 443;
  String host = "api.openai.com";
  String endpoint = "/v1/audio/transcriptions";

  //Allows for TLS handshake to occur
  WiFiClientSecure client;
  client.setCACert(cert);
  client.connect(host.c_str(), port);

  //Some HTTP specific values
  String boundary = "------------------------e08f77c03373314f";
  String twohyphens = "--";
  String newline = "\r\n";

  int newlineLength = 2;
  int boundaryLength = twohyphens.length() + boundary.length() + newlineLength;
  
  //Fields of the multipart-form data
  String fileDisposition = String("Content-Disposition: form-data; name=\"file\"; filename=\"") + filename + String("\"");
  String fileType = "Content-Type: audio/x-wav";
  String modelDisposition = "Content-Disposition: form-data; name=\"model\"";
  String model = "whisper-1";

  int contentLength = boundaryLength + fileDisposition.length() + newlineLength + fileType.length() + newlineLength + newlineLength + audioFile.size() + newlineLength
                      + boundaryLength + modelDisposition.length() + newlineLength + newlineLength + model.length() + boundaryLength + twohyphens.length() + newlineLength;


  client.println("POST "+ endpoint +" HTTP/1.1");
  client.println("Host: "+ host);
  client.println("User-Agent: ESP32");
  client.println("Accept: */*");
  client.println("Authorization: Bearer " + String(APIKEY));
  client.println("Content-Length: " + String(contentLength));
  client.println("Content-Type: multipart/form-data; boundary=" + String(boundary));
  client.println();  

  //Writing body of request
  client.println(twohyphens + boundary);
  client.println(fileDisposition);
  client.println(fileType);
  client.println();
  
  uint8_t buffer[1024];
  while (audioFile.available()){
    size_t readBytes = audioFile.readBytes((char*)buffer, 1024);    
    client.write(buffer, readBytes);
  }
  client.flush();
  audioFile.close();
  client.println();
  client.println(twohyphens + boundary);
  client.println(modelDisposition);
  client.println();
  client.println(model);
  client.println(twohyphens + boundary + twohyphens);  
  client.println();

  //Parse response from server
  Serial.println("============");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String response = "";
  while (client.available()) { 
    response += (char) client.read();
  }

  Serial.println(response);

  client.stop();
  

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
