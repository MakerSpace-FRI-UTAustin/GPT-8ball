void marquee(char *answer)
{
  int length = strlen(answer);

  char text[length + 16] = "";
  strcat(text, answer);
  strcat(text, "                ");
  
  if(length < lcdColumns)
    lcd.print(answer);
  else
  {
    int pos;
    while (pos + 16 < strlen(text)){
      // lcd.clear();
      lcd.setCursor(0,0);
      for (int i = 0; i < 16; i ++){
        lcd.print(text[pos + i]);
      }
      delay(500);
      pos += 3;
    }
    
  }  
}