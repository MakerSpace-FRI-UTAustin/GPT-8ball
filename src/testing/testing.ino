#include <Arduino.h>
#include <AudioTools.h>
// #include <SPIFFS.h>
#include <FS.h>
#include <SD.h>

// #include "esp_wifi.h"

#include "secrets.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

// #include <esp_wpa2.h>
#include "lcd.h"
#include "wifi.h"
#include "audio.h"

unsigned char shakenFlag = 0;
unsigned long timepoint = 0;
const int sampleSize = 10;


const int xInput = 33;
const int yInput = 35;
const int zInput = 32;

// initialize minimum and maximum Raw Ranges for each axis
int xRawMin = 512;
int xRawMax = 4095;

int yRawMin = 512;
int yRawMax = 4095;

int zRawMin = 512;
int zRawMax = 4095;

WiFiClientSecure client;


void setup(void) {
  Serial.begin(115200);

  Serial.println("Starting");
  // initialize LCD
  lcd.init();
  // turn on LCD backlight                      
  lcd.backlight();
  
  //Mic setup
  
  //FS setup; Can be substituted for SD card if needed
  if(!SD.begin(5)){
    Serial.println("SD Mount Failed");
    return;
  
}

  
  Serial.println("Wifi Setup next");

  wifiSetup();
  Serial.println("Setup Complete");

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
  audioFile = SD.open(filename, FILE_READ);
  audioFile.seek(0);

  //information of the API to connect to
  int port = 443;
  char* host = "api.openai.com";
  char* endpoint = "/v1/audio/transcriptions";

  //Allows for TLS handshake to occur
  
  client.setCACert(cert);
  client.setInsecure();
  Serial.println(client.connect(host, port));
  // delay(1000);


  if (!client.connected()){
    Serial.println("Issues with establishing connection to API");
    char* empty = "";
    strncpy(prompt, empty, 1);
    return;
  }

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

  audioFile.seek(0);
  delay(500);

  // int test = 0;
  uint8_t buffer[1600];
  while (audioFile.available()) {
    size_t readBytes = audioFile.readBytes((char*)buffer, 1600);
    client.write(buffer, readBytes);
    // if (test == 0 ){
    //   for (int i = 0; i< readBytes; i++)
    //     Serial.print(buffer[i]);
    //     Serial.print(" ");
    // }
    // test++;
    client.flush();

  }
  client.flush();
  delay(500);
  audioFile.close();
  Serial.printf("raw sending audio bytes: %d s \n", (millis() - start)/1000);

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
  Serial.print("Response str: ");
  Serial.println(response);
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
  Serial.println("Sending prompt");
    
  //information of the API to connect to
  int port = 443;
  char* host = "api.openai.com";
  char* endpoint = "/v1/chat/completions";

  //Allows for TLS handshake to occur
  client.setCACert(cert);
  // client.setInsecure();
  client.connect(host, port);

  if (!client.connected()){
    Serial.println("Issues with establishing connection to API");
    char* empty = "";
    strncpy(answer, empty, 1);
    return;
  }

  char* payload1 = "{\"model\": \"gpt-3.5-turbo\", \"max_tokens\": 100, \"messages\": [{\"role\": \"system\", \"content\": \"You are an AI powered Magic 8 Ball, your answers should resemble a typical 8 Ball's response but can be adapted to fit the needs of the prompt. It should be really ridiculous, when acceptable. Make it around 50 characters\"}, {\"role\": \"user\", \"content\": \"";
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
      Serial.println("headers received");
      break;
    }
  }
  char response[1024];
  response[0] = '\0';
  // client.read();
  // client.read();
  // client.read();
  // client.read();

  int temp = 0;
  while (client.available()) {
    char c = (char)client.read();
    if (temp > 4)
      strncat(response, &c, 1);
    else
      temp++;
  }
  client.stop();


  //WEIRD FIX

  Serial.println("Response str: ");
  Serial.println(response);

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, response);


  if (error) {
    Serial.println("deserialize error");
    strncpy(answer, error.c_str(), strlen(error.c_str()));
  }
  else{

    JsonObject choices = doc["choices"][0];
    const char* message = choices["message"]["content"];

    strncpy(answer, message, strlen(message) + 1);
  }
}


int ReadAxis(int axisPin){
	long reading = 0;
	for (int i = 0; i < sampleSize; i++)
	{
	reading += analogRead(axisPin);

	}
	return reading/sampleSize;

}


int readAcc(){

  //Read raw values
	int xRaw = ReadAxis(xInput);
	int yRaw = ReadAxis(yInput);
	int zRaw = ReadAxis(zInput);


	 // Convert raw values to 'milli-Gs"
  long xScaled = map(xRaw, xRawMin, xRawMax, -1000, 1000);
  long yScaled = map(yRaw, yRawMin, yRawMax, -1000, 1000);
  long zScaled = map(zRaw, zRawMin, zRawMax, -1000, 1000);
  return sqrt(xScaled * xScaled + yScaled * yScaled +  zScaled * zScaled);



}

void loop() {
  clearWrite("Shake to ask");

  int acc = readAcc();
  Serial.print("Acc:");
  Serial.println(acc); 

  if (acc > 850){
    shakenFlag = 1;
  }


  if (shakenFlag){
    Serial.println("Triggered");
    // digitalWrite(15, HIGH);
    // delay(1000);
    // digitalWrite(15, LOW);
    clearWrite("Triggered");

    if (!recordClip()){
      clearWrite("Failed to record audio");
      goto fail;
    }


    char prompt[200];
    char answer[400];

    clearWrite("Transcribing");


    sendAudio(prompt);
    Serial.println(prompt);
    if (strcmp(prompt, "IncompleteInput") == 0 || strcmp(prompt, "InvalidInput") == 0 || strlen(prompt) == 0 || !checkChars(prompt)) {
      clearWrite("transcribe error");
      delay(5000);
      
    }
    // Serial.printf("Time taken for audio %d s \n", (millis() - start) / 1000);
    else{
      clearWrite("Generating");

      sendPrompt(prompt, answer);
      Serial.println("wroked");
      Serial.println(answer);


      marquee(answer,0);
    }
    
    // Serial.printf("Time taken for response %d s \n", (millis() - start) / 1000);
    
    fail:
    shakenFlag = 0;

    
  }
  delay(500);
}



