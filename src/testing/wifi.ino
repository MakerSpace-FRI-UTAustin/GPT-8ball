//Make into an AP to make configurations on
void wifiSetup() {  
  clearWrite("try with PEAP");

  WiFi.mode(WIFI_STA);


  // WiFi.begin(SSID, WPA2_AUTH_PEAP, EAP_IDENTITY, EAP_USERNAME, EAP_PASSWORD);
  // WiFi.begin(SSID,PSK);
  WiFi.begin("Rahul’s iPhone","12345678");


  for (int i = 0; i < 500 && WiFi.status() != WL_CONNECTED; i++) {
    delay(100);  
    Serial.println("Connecting to WiFi..");
  }
  //return from func if alrady connected to WIFI otherwise try AP method.
  if (WiFi.status() == WL_CONNECTED){
    Serial.println("CONNEDCTED");
    return;
  }
  else{
    // clearWrite("AP deployed");
    // WiFiManager wm;
    // // bool res;
    // // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
    // if(!wm.autoConnect("AutoConnectAP","password")) {
        // Serial.println("Failed to connect");
    ESP.restart();
    // } 
  }

  
  // clearWrite("AP deployed");
  // WiFiManager wm;
  // // bool res;
  // // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  // if(!wm.autoConnect("AutoConnectAP","12345")) {
  //     // Serial.println("Failed to connect");
  // } 
}