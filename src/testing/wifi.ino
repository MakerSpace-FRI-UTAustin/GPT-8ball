//Make into an AP to make configurations on
void wifiSetup() {
  WiFi.begin(SSID, PASS);
  lcd.clear();  

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    
    lcd.setCursor(0, 0);
    lcd.print("Setting up WiFi");    
    Serial.println("Connecting to WiFi..");
  }
  lcd.clear();
  Serial.println(WiFi.localIP());
}