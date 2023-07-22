//Setup file and encoder
void recordSetup() {
  //Initialize the file
  LittleFS.remove(filename);
  audioFile = LittleFS.open(filename, FILE_WRITE);

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

  Serial.printf("time taken: %d seconds\n", (millis() - start) / 1000);

  audioFile.close();
}
