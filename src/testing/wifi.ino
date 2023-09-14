//Make into an AP to make configurations on
void wifiSetup() {  
  clearWrite("try with PEAP");
  WiFi.begin(SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_IDENTITY, EAP_PASSWORD);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(1000);  
    // Serial.println("Connecting to WiFi..");
  }
  //return from func if alrady connected to WIFI otherwise try AP method.
  if (WiFi.status() == WL_CONNECTED){
    return;
  }
  
  clearWrite("AP deployed");
  WiFiManager wm;
  // bool res;
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  if(!wm.autoConnect("AutoConnectAP","password")) {
      // Serial.println("Failed to connect");
      ESP.restart();
  } 
}