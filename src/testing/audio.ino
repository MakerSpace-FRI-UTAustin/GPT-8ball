//Setup file and encoder
bool recordSetup() {
  //Initialize the file
  Serial.println("Entering record setup");
  
  auto cfg = adc.defaultConfig(RX_MODE);
  cfg.copyFrom(info);


  if (!SD.exists(filename))
    SD.remove(filename);
  Serial.println("Delete file");

  audioFile = SD.open(filename, FILE_WRITE);
  Serial.println("open file");

  if (!audioFile) {
    Serial.println("There was an error opening the file for writing");
    return false;
  }
  //Prompts encoder to write metadata to file
  audioFile.seek(0);
  // out.setEncoder(new WAVEncoder());
  // auto cfg = out.audioInfo();
  // cfg.copyFrom(info);
  out.begin(info);
  adc.begin(cfg);



  return true;
}

bool recordClip() {
  if (!recordSetup()){
    return false;
  }
  Serial.println("starting record");

   clearWrite("Recording");

  unsigned long start = millis();
  //Records a set duration of audio by determining total file size of audio
  // for (int i = 0; i <= fileSize; i += copier.copy()) {
  // }
  copier.copyMs(duration * 1000,  info);

  Serial.printf("time taken: %d seconds\n", (millis() - start) / 1000);
  out.end();
  adc.end();

  audioFile.close();

  return true;
}



