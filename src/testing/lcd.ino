void marquee(char* msg, int row) {
  int msgLength = strlen(msg);
  char displayText[lcdColumns + 1];  // +1 for the null terminator

  // Scroll from pos = 0 to pos = msgLength - 1 for one complete cycle.
  for (int pos = 0; pos < msgLength - lcdColumns + 1; pos++) {
    // Build a string of exactly displayWidth characters
    for (int i = 0; i < lcdColumns; i++) {
      int index = (pos + i) % msgLength;  // Wrap-around using modulo arithmetic
      displayText[i] = msg[index];
    }
    displayText[lcdColumns] = '\0';  // Terminate the string

    lcd.setCursor(0, row);
    lcd.print(displayText);
    delay(400);
  }
}


void clearWrite(char *text){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);

}