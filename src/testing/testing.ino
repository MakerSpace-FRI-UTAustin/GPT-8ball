#include <Arduino.h>
#include <AudioTools.h>
#include <LittleFS.h>
#include <FS.h>
#include "secrets.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include "lcd.h"
#include "wifi.h"
#include "audio.h"

unsigned char shakenFlag = 0;
unsigned long timepoint = 0;

//Callback for Gravity: Shake Sensor
void IRAM_ATTR isr(){
  if (millis() - timepoint > 50U && shakenFlag == 0){
    shakenFlag = 1;   
  }
}

void setup(void) {
  Serial.begin(115200);

  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  
  //Mic setup
  auto cfg = adc.defaultConfig(RX_MODE);
  cfg.copyFrom(info);
  adc.begin(cfg);

  //FS setup; Can be substituted for SD card if needed
  if (!LittleFS.begin(true)) {
    // Serial.println("LittleFS Mount Failed");
    return;
  }

  //Interrupt handling for the shake sensor
  pinMode(4, INPUT_PULLUP);
  attachInterrupt(4, isr, FALLING);
  timepoint = millis();

  wifiSetup();

}

//Sanity check for API responses
bool checkChars(char* str){
  for (int i = 0; i < strlen(str); i++){
    if (!isAscii(str[i])){
      return false;      
    }
  }
  return true;
}

//Audio file -> text transcribtion
// Sadly manually printing the raw HTTP request to server
void sendAudio(char* prompt) {
  // Serial.println("Sending audio");
  audioFile = LittleFS.open(filename, FILE_READ);
  audioFile.seek(0);

  //information of the API to connect to
  int port = 443;
  char* host = "api.openai.com";
  char* endpoint = "/v1/audio/transcriptions";

  //Allows for TLS handshake to occur
  WiFiClientSecure client;
  client.setCACert(cert);
  client.connect(host, port);

  //Some HTTP specific values
  char* boundary = "------------------------e08f77c03373314f";
  char* twohyphens = "--";
  char* newline = "\r\n";

  int newlineLength = 2;
  int boundaryLength = strlen(twohyphens) + strlen(boundary) + newlineLength;

  //Fields of the multipart-form data
  char fileDisposition[100];
  sprintf(fileDisposition, "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"", filename);
  char* fileType = "Content-Type: audio/x-wav";
  char* modelDisposition = "Content-Disposition: form-data; name=\"model\"";
  char* model = "whisper-1";

  int contentLength = boundaryLength + strlen(fileDisposition) + newlineLength + strlen(fileType) + newlineLength + newlineLength + audioFile.size() + newlineLength
                      + boundaryLength + strlen(modelDisposition) + newlineLength + newlineLength + strlen(model) + boundaryLength + strlen(twohyphens) + newlineLength;

  client.print("POST ");
  client.print(endpoint);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("User-Agent: ESP32");
  client.println("Accept: */*");
  client.print("Authorization: Bearer ");
  client.println(APIKEY);
  client.print("Content-Length: ");
  client.println(contentLength);
  client.print("Content-Type: multipart/form-data; boundary=");
  client.println(boundary);
  client.println();

  //Writing body of request
  client.print(twohyphens);
  client.println(boundary);
  client.println(fileDisposition);
  client.println(fileType);
  client.println();

  int start = millis();
  
  uint8_t buffer[1600];
  while (audioFile.available()) {
    size_t readBytes = audioFile.readBytes((char*)buffer, 1600);
    client.write(buffer, readBytes);
  }
  client.flush();
  audioFile.close();
  // Serial.printf("raw sending audio bytes: %d s \n", (millis() - start)/1000);

  client.println();
  client.print(twohyphens);
  client.println(boundary);
  client.println(modelDisposition);
  client.println();
  client.println(model);
  client.print(twohyphens);
  client.print(boundary);
  client.println(twohyphens);
  client.println();

  //Parse response from server
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  char response[100];
  response[0] = '\0';
  while (client.available()) {
    char c = (char)client.read();
    strncat(response, &c, 1);
  }
  client.stop();

  StaticJsonDocument<16> doc;
  DeserializationError error = deserializeJson(doc, response, 100);

  if (error) {
    // Serial.println("deserialize error");
    strncpy(prompt, error.c_str(), strlen(error.c_str()) + 1);
    return;
  }

  const char* text = doc["text"];
  strncpy(prompt, text, strlen(text) + 1);
}

void sendPrompt(char* prompt, char* answer) {
  // Serial.println("Sending prompt");
    
  //information of the API to connect to
  int port = 443;
  char* host = "api.openai.com";
  char* endpoint = "/v1/chat/completions";

  //Allows for TLS handshake to occur
  WiFiClientSecure client;
  client.setCACert(cert);
  client.connect(host, port);

  char* payload1 = "{\"model\": \"gpt-3.5-turbo\", \"max_tokens\": 100, \"messages\": [{\"role\": \"system\", \"content\": \"You are an AI powered Magic 8 Ball, your answers should resemble a typical 8 Ball's response but can be adapted to fit the needs of the prompt.\"}, {\"role\": \"user\", \"content\": \"";
  char* payload2 = "\"}]}";
  int payloadLength = strlen(payload1) + strlen(payload2) + strlen(prompt);

  client.print("POST ");
  client.print(endpoint);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.println("User-Agent: ESP32");
  client.println("Accept: */*");
  client.print("Authorization: Bearer ");
  client.println(APIKEY);
  client.print("Content-Length: ");
  client.println(payloadLength);
  client.println("Content-Type: application/json");
  client.println();

  client.print(payload1);
  client.print(prompt);
  client.println(payload2);
  client.println();

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      // Serial.println("headers received");
      break;
    }
  }
  char response[800];
  response[0] = '\0';
  while (client.available()) {
    char c = (char)client.read();
    strncat(response, &c, 1);
  }
  client.stop();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    // Serial.println("deserialize error");
    strncpy(answer, error.c_str(), strlen(error.c_str()) + 1);
  }

  JsonObject choices = doc["choices"][0];
  const char* message = choices["message"]["content"];

  strncpy(answer, message, strlen(message) + 1);
}


void loop() {

  clearWrite("Shake to ask");
  if (shakenFlag){
    clearWrite("Recording audio");

    
    recordClip();

    char prompt[200];
    char answer[400];

    clearWrite("Transcribing");


    int start = millis();
    sendAudio(prompt);
    // Serial.println(prompt);
    if (strcmp(prompt, "InvalidInput") == 0 || strlen(prompt) == 0 || !checkChars(prompt)) {
      clearWrite("transcribe error");
      
    }
    // Serial.printf("Time taken for audio %d s \n", (millis() - start) / 1000);
    else{
      clearWrite("Generating");


      start = millis();
      sendPrompt(prompt, answer);

      marquee(answer);
    }
    
    Serial.println(answer);
    // Serial.printf("Time taken for response %d s \n", (millis() - start) / 1000);
    
    shakenFlag = 0;

    
  }
  delay(500);
}
